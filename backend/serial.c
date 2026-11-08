
#include "conf.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/atomic.h>
#include <avr/sleep.h>
#include <stdio.h>
#include "util.h"
#include "serial.h"

#define BUFFER_SIZE 512
uint8_t buffer[BUFFER_SIZE];
volatile unsigned takeIndex=0;
volatile unsigned count=0;
volatile unsigned putIndex=0;

#define USARTn SERIAL_USART
#define PORTn SERIAL_PORT

uint8_t errors=0;

uint8_t serialGetError(){
    uint8_t tmp;
    ATOMIC_BLOCK( ATOMIC_RESTORESTATE ){
        tmp = errors;
        errors=0;
    }
    return tmp;
}

void serialInit();
uint8_t serialGetError();

int serialPutc(char c, FILE* x)
{
    while( 0 == (USARTn.STATUS & (1<<5)) ){
        //data remains to be sent out to serial port
    }
    USARTn.TXDATAL = c;
    return 0;
}

int serialGetc(FILE* x)
{
    //wait until count > 0
    //Since count is updated inside interrupt routine,
    //we need to pay attention to atomicity
    //However, we don't want to continuously enable and disable interrupts
    //when we check because the AVR will suppress an interrupt for
    //one instruction after sei(). Thus, in a tight loop, a significant
    //fraction of time is spent with interrupts disabled, and we
    //might get a serial buffer overrun. Thus, we do
    //a check without interrupts and if that shows that
    //the count has become nonzero, we lock and do our operation.

    unsigned char c;
    BOOL keepLooping=TRUE;
    while(keepLooping){
        //interrupts are active here, so we only read the count
        if( count == 0 )
            continue;

        //barrier
        __sync_synchronize();
        ATOMIC_BLOCK(ATOMIC_RESTORESTATE){
            if( count != 0 ){
                c = buffer[takeIndex];
                takeIndex++;
                if( takeIndex >= BUFFER_SIZE )
                    takeIndex=0;
                count--;
                keepLooping=FALSE;
            }
        }
        //barrier
        __sync_synchronize();
    }

    return c;

}


void serialInit(){

    USARTn.BAUD = (64 * F_CPU) / (16*115200);

    //set transmit pin to output
    PORTn.DIRSET = SERIAL_TRANSMIT_PIN;

    //enable receiver and transmitter
    USARTn.CTRLB |= (1<<7) | (1<<6);

    //enable interrupts on receive
    USARTn.CTRLA |= (1<<7);

    static FILE xstdout = FDEV_SETUP_STREAM(serialPutc, NULL, _FDEV_SETUP_WRITE);
    stdout = &xstdout;
    static FILE xstdin = FDEV_SETUP_STREAM(NULL, serialGetc, _FDEV_SETUP_READ);
    stdin = &xstdin;

}

ISR(USARTn_RXC_vect){
    uint8_t flags = USARTn.RXDATAH;
    uint8_t c = USARTn.RXDATAL;

    if( flags & (1<<6) )
        errors |= ERROR_BUFFER_OVERRUN;
    if( flags & (1<<2) )
        errors |= ERROR_FRAME_ERROR;

    if( count < BUFFER_SIZE ){
        buffer[putIndex] = c;
        putIndex++;
        if( putIndex >= BUFFER_SIZE )
            putIndex = 0;
        count++;
    }
}
