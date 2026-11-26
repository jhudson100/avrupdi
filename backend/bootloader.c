//the bootloader must be self-contained
//and not use any library functions
//It is compiled separately and written to the start of flash
//No interrupts can be used in this code
//since the interrupt table starts at the end of the bootloader

#define BOOTLOADER_VERSION_STRING "1000"

#define F_CPU 24'000'000ULL

#ifndef __AVR_AVR32DD14__
#error Assuming AVR32DD14
#endif

#include <avr/io.h>

#define UPDILED_PORT PORTD
#define UPDILED_PIN (1<<5)

//the serial port we're using
#define SERIAL_USART USART0
#define SERIAL_TRANSMIT_PORT PORTA
#define SERIAL_TRANSMIT_PIN (1<<0)


#define PAGE_SIZE 512

//this array is only used when we are in the bootloader
//once we exit to app code, this memory is reclaimed

#define SCRATCH_SPACE_SIZE 512
char scratchspace[SCRATCH_SPACE_SIZE];

static void boot_delay_100_ms(){
    //at 24MHz: 24 cycles = 1 us
    //          24000 cycles = 1ms
    //          240000 cycles = 10ms
    //          2400000 cycles = 100ms
    __builtin_avr_delay_cycles( F_CPU / 10 );
}

static void boot_serial_send_char(char x){
    while( 0 == (SERIAL_USART.STATUS & (1<<6)) ){
    }
    SERIAL_USART.TXDATAL = x;
}

static void boot_serial_send(const char* x){
    while(*x){
        boot_serial_send_char(*x++);
    }
}

static uint8_t boot_serial_recv(){
    while(0 == (SERIAL_USART.STATUS & (1<<7))){
    }
    return SERIAL_USART.RXDATAL;
}

static void boot_serial_getLine(char* buffer, uint16_t maxlen){
    uint8_t i=0;
    while(1){
        char c = (char) boot_serial_recv();
        if( c == '\r' ){
            //ignore
        } else if( c == '\n' ){
            buffer[i] = 0;
            return;
        } else{
            if( i == maxlen-1 ){
                //invalid message; truncate to 0 length
                buffer[0]=0;
                return;
            } else {
                buffer[i++] = c;
            }
        }
    }
}

static uint8_t boot_strings_equal(const char* a, const char* b){
    while(1){
        if( *a != *b )
            return 0;
        if( !*a && !&*b )
            return 1;
        if( !*a || !*b )
            return 0;
        ++a;
        ++b;
    }
}


static void boot_set_led(uint8_t on){
    if( on )
        UPDILED_PORT.OUTCLR = UPDILED_PIN;
    else
        UPDILED_PORT.OUTSET = UPDILED_PIN;
}


static void writePage(uint8_t pageNumber, void* data){

    _PROTECTED_WRITE( NVMCTRL.CTRLA, 0x00 );        //nocmd
    _PROTECTED_WRITE( NVMCTRL.CTRLA, 0x08 );        //page erase

    uint8_t* mappedAddress = (uint8_t*) 0x8000;
    mappedAddress += pageNumber * PAGE_SIZE;

    *mappedAddress = 0; //trigger the erase

    while( NVMCTRL.STATUS & 1 ){
        //flash busy
    }

    _PROTECTED_WRITE( NVMCTRL.CTRLA, 0x00 );        //nocmd

    _PROTECTED_WRITE( NVMCTRL.CTRLA, 0x02 );        //flash write enable

    char* p = (char*) data;
    for(uint16_t i=0;i<PAGE_SIZE;i++,p++){
        *mappedAddress = *p;
        ++mappedAddress;
        ++p;
    }

    while( NVMCTRL.STATUS & 1 ){
        //flash busy
    }

    _PROTECTED_WRITE( NVMCTRL.CTRLA, 0x00 );        //nocmd

}


static void boot_serial_init(){

    SERIAL_USART.BAUD = (64 * F_CPU) / (16*115200);

    //set transmit pin to output
    SERIAL_TRANSMIT_PORT.DIRSET = SERIAL_TRANSMIT_PIN;

    //enable receiver and transmitter
    SERIAL_USART.CTRLB |= (1<<7) | (1<<6);

    //enable interrupts on receive
    SERIAL_USART.CTRLA |= (1<<7);

}

static const char* hexTable="0123456789abcdef";
static void hex(uint8_t v, char* c){
    c[0] = hexTable[v>>4];
    c[1] = hexTable[v&15];
}

static uint8_t hexNibble(char c){
    if( c >= '0' && c <= '9' )
        return c-'0';
    if( c >= 'A' && c <= 'F' )
        return c - 'A' + 10;
    if( c >= 'a' && c <= 'f' )
        return c - 'a' + 10;
    return 0;
}

static uint8_t parseHexByte(const char* c){
    uint8_t b = hexNibble(c[0]) << 4;
    b |= hexNibble(c[1]);
    return b;
}

void boot_print(uint8_t ok, const char* msg){
    const char* p;
    if(ok)
        p="true";
    else
        p="false";

    boot_serial_send("{\"ok\": ");
    boot_serial_send(p);
    boot_serial_send(", \"msg\": \"");
    boot_serial_send(msg);
    boot_serial_send("\"}\n");
}

int main(){

    //disable watchdog timer
    _PROTECTED_WRITE(WDT.CTRLA, 0);

    //brownout detector enabled
    _PROTECTED_WRITE(BOD.CTRLA,1<<2);

    //set CPU clock to 24MHz
    #if F_CPU != 24'000'000
        #error Wrong speed
    #endif
    _PROTECTED_WRITE( CLKCTRL.OSCHFCTRLA, 0x9<<2 );

    boot_serial_init();

    UPDILED_PORT.DIRSET = UPDILED_PIN;
    boot_set_led(0);
    boot_set_led(1);
    boot_delay_100_ms();
    boot_set_led(0);

    uint16_t firstAppPage = FUSE.BOOTSIZE;

    while(1){
        boot_serial_send("BOOT>");
        boot_serial_getLine(scratchspace,SCRATCH_SPACE_SIZE);
        if( boot_strings_equal(scratchspace,"WRITE") ){
            //note that this is two ascii hexadecimal characters!
            //No leading 0x
            boot_serial_getLine(scratchspace,SCRATCH_SPACE_SIZE);
            uint8_t pageNumber = parseHexByte(scratchspace);
            if( pageNumber < firstAppPage ){
                boot_print(0,"Bad page");
            }
            boot_print(1,"Ready");
            char* p = scratchspace;
            for(uint16_t i=0;i<PAGE_SIZE;++i){
                *p = boot_serial_recv();
                p++;
            }
            writePage(pageNumber,scratchspace);
            boot_print(1,"Wrote");
        } else if( boot_strings_equal(scratchspace,"READ")){
            boot_serial_getLine(scratchspace,SCRATCH_SPACE_SIZE);
            uint8_t pageNumber = parseHexByte(scratchspace);
            //flash is mapped at 0x8000
            boot_serial_send("\"ok\": true, \"data\": \"");
            char* p = (char*) 0x8000 + pageNumber * PAGE_SIZE;
            char hb[2];
            for(uint16_t i=0;i<PAGE_SIZE;++i){
                hex(*p,hb);
                boot_serial_send_char(hb[0]);
                boot_serial_send_char(hb[1]);
                p++;
            }
            boot_serial_send("\"}\n");
        } else if( boot_strings_equal(scratchspace, "BLVERSION") ){
              boot_print(1,BOOTLOADER_VERSION_STRING);
        } else if( boot_strings_equal(scratchspace,"BOOT")){
            boot_print(1,"Exiting");
            break;
        } else if( boot_strings_equal(scratchspace,"SIZE")){
            boot_serial_send( "{\"ok\": true, \"size\": 0x");
            char hb[2];
            hex(firstAppPage,hb);
            boot_serial_send_char(hb[0]);
            boot_serial_send_char(hb[1]);
            boot_serial_send("}\n");
        } else {
            boot_print(0,"Unknown command");
        }
    }

    //jump to start of app code
    asm("ijmp" : : "z" (firstAppPage * PAGE_SIZE) );

}
