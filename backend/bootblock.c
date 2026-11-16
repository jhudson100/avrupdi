#include <avr/io.h>
#include "conf.h"
#include <util/crc16.h>
#include "scratchspace.h"

#if SCRATCH_SPACE_SIZE < 512
    #error
#endif

#define USARTn SERIAL_USART

//jump over all code in section 7. We will have the flash update
//code in section 7 and we must not let the startup sequence execute it.
//Sections 7 and 8 are not used otherwise by avr-libc or gcc.
//This must be the first thing in init7
__attribute__(used,unused,naked,(section(".init7")))
static static void init7(){
    //GCC requires this function to only use inline assembly
    asm(
        "call earlyInit\n")
        "jmp init8\n"
    );
    /*must not end with "return"! Just fall through*/
}

//all code that will be active when actually flashing memory must
//be in section init7



__attribute__(used,unused,(section(".init7")))
static void serial_send_char(char x){
    while( 0 == (USARTn.STATUS & (1<<6)) ){
    }
    USARTn.TXDATAL = x;
}

__attribute__(used,unused,(section(".init7")))
static void serial_send(const char* x){
    while(*x){
        serial_send_char(*x++);
    }
}

__attribute__(used,unused,(section(".init7")))
static uint8_t serial_recv(){
    while(0 == (USARTn.STATUS & (1<<7)){
    }
    return USARTn.RXDATAL;
}



__attribute__(used,unused,(section(".init7")))
void earlyInit(){
    wdt_disable();

    //set CPU clock to 24MHz
    #if F_CPU != 24'000'000
        #error Wrong speed
    #endif
    _PROTECTED_WRITE( CLKCTRL.OSCHFCTRLA, 0x9<<2 );

    serialInit();

    _delay_ms(500);
    
    //check to see if we need to do a flash update
    serial_getLine(scratchspace)
    if( strings_equal(scratchspace,"UPDATE") ){
        do_update_flash();
    } else {
        serial_send("{ \"ok\": true, \"boot\": true }\n");
    }
}

__attribute__(used,unused,(section(".init7")))
void earlySerialInit(){

    USARTn.BAUD = (64 * F_CPU) / (16*115200);

    //set transmit pin to output
    PORTn.DIRSET = SERIAL_TRANSMIT_PIN;

    //enable receiver and transmitter
    USARTn.CTRLB |= (1<<7) | (1<<6);

    //enable interrupts on receive
    USARTn.CTRLA |= (1<<7);

}

__attribute__(used,unused,(section(".init7")))
void do_update_flash(){
    uint8_t tmp;

    //flush the receive buffer
    while( USARTn.RXCIF ){
        tmp = USARTn.RXDATAL;
    }

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
