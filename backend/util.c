#include "util.h"

#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>
#include "conf.h"

//we use the RTC to implement sleeps
//RTC ticks at approx 1 msec rate,
void sleep(uint8_t msec)
{
    while( RTC.STATUS & (1<<1) ){
    }
    //reset so we don't have to worry about wrapping
    RTC.CNT=0;
    while( RTC.CNT < msec ){
    }
}


void resetAVR(){
    //Microchip recommends using the watchdog
    //to reset the AVR
    fputs("RESET",stdout);
    wdt_enable(WDTO_120MS);
    while(1){}
}


void setUPDILED(BOOL on){
    if( on )
        UPDILED_PORT.OUTCLR = UPDILED_PIN;
    else
        UPDILED_PORT.OUTSET = UPDILED_PIN;
}
