#include <avr/io.h>
#include "conf.h"
#include <util/crc16.h>
#include "scratchspace.h"

#if SCRATCH_SPACE_SIZE < 512
    #error
#endif

#define USARTn SERIAL_USART

#define BOOTFUNC(ret,nam)  __attribute__(used,unused,(section(".init7"))) static ret nam


//jump over all code in section 7. We will have the flash update
//code in section 7 and we must not let the startup sequence execute it.
//Sections 7 and 8 are not used otherwise by avr-libc or gcc.
//This must be the first thing in init7
__attribute__(used,unused,naked,(section(".init7")))
static static void init7(){
    //GCC requires this function to only use inline assembly
    asm(
        "call boot_earlyInit\n")
        "jmp init8\n"
    );
    /*must not end with "return"! Just fall through*/
}

//all code that will be active when actually flashing memory must
//be in section init7



__attribute__(used,unused,(section(".init7")))
static void boot_serial_send_char(char x){
    while( 0 == (USARTn.STATUS & (1<<6)) ){
    }
    USARTn.TXDATAL = x;
}

BOOTFUNC(void, boot_serial_send)(const char* x){
    while(*x){
        serial_send_char(*x++);
    }
}

BOOTFUNC(uint8_t,boot_serial_recv)(){
    while(0 == (USARTn.STATUS & (1<<7)){
    }
    return USARTn.RXDATAL;
}
BOOTFUNC(void, boot_serial_getLine)(char* buffer, uint8_t maxlen){
    uint8_t i=0;
    while(1){
        char c = (char) serial_recv();
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

BOOTFUNC(uint8_t, boot_strings_equal)(const char* a, const char* b){
    while(1){
        if( *a != *b )
            return 0;
        if( !*a && !&b )
            return 1;
        if( !*a || !*b )
            return 0;
        ++a;
        ++b;
    }
}


BOOTFUNC(void, boot_set_led)(uint8_t on){
    if( on )
        UPDILED_PORT.OUTCLR = UPDILED_PIN;
    else
        UPDILED_PORT.OUTSET = UPDILED_PIN;
}

BOOTFUNC(void, boot_earlyInit)(){
    wdt_disable();

    //set CPU clock to 24MHz
    #if F_CPU != 24'000'000
        #error Wrong speed
    #endif
    _PROTECTED_WRITE( CLKCTRL.OSCHFCTRLA, 0x9<<2 );

    boot_serial_init();

    UPDILED_PORT.DIRSET = UPDILED_PIN;
    boot_set_led(0);
    boot_set_led(1);
    _delay_ms(100);
    boot_set_led(0);
    
    //check to see if we need to do a flash update
    boot_serial_getLine(scratchspace)
    if( boot_strings_equal(scratchspace,"UPDATE") ){
        boot_update_flash();
        //fall through and use the new updated flash
    } else {
        boot_serial_send("{ \"ok\": true, \"boot\": true }\n");
        //use the existing flash
    }
}

BOOTFUNC(void, boot_serial_init)(){

    USARTn.BAUD = (64 * F_CPU) / (16*115200);

    //set transmit pin to output
    PORTn.DIRSET = SERIAL_TRANSMIT_PIN;

    //enable receiver and transmitter
    USARTn.CTRLB |= (1<<7) | (1<<6);

    //enable interrupts on receive
    USARTn.CTRLA |= (1<<7);

}

BOOTFUNC(void, boot_update_flash)(){
    uint8_t tmp;

    //flush the receive buffer
    while( USARTn.RXCIF ){
        tmp = USARTn.RXDATAL;
    }

    serial_send("OK\n");
    
    //each block is 512 bytes; we have a max of 32KB = 64 blocks
    uint8_t numBlocks = boot_serial_recv();
    if( numBlocks == 0 ){
        boot_panic(0x1111);
    }
    while(numBlocks){
        uint8_t* p = scratchspace;
        for(uint16_t i=0;i<512;++i){
            *p = boot_serial_recv();
            p++;
        }
        uint16_t expected_csum = boot_serial_recv_u32();
        
    
        
??get number of blocks
??get each block and verify crc
??write block
??repeat until done
??return

    while( numBlocks > 0 ){
        serial_send("FLRD");

    uint8_t* p = buffer;
    for(uint16_t i=0;i<512;++i){
        *p++ = serial_recv();
    }
    
}    

//start of init8
__attribute__(used,unused,naked,(section(".init8")))
static void init8(void){

    /* This function must only use inline assembly
    */
    asm( "nop");
    /*must not end with "return"! Just fall through*/
}




}
