//default speed is 3.3MHz for EB,  4MHz for Dx
//3.5MHz is a reasonable approximation for both
#define F_CPU 3'500'000ULL

#include <avr/io.h>
#include <util/delay.h>


int main(){
    LED_PORT.DIRSET = LED_PIN;
    while(1){
        LED_PORT.OUTSET = LED_PIN;
        _delay_ms(250);
        LED_PORT.OUTCLR = LED_PIN;
        _delay_ms(250);
    }
    return 0;
}
