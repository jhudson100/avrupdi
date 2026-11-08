#pragma once
//Automatically generated file; do not edit
#include <util/delay.h>
typedef void (*BitDelayFunc)(void);
struct BitRate{
    uint32_t bitsPerSecond;
    BitDelayFunc bitTimeDelayFunc;
    BitDelayFunc quarterBitTimeDelayFunc;
};
static inline void delay3333333ns(){
    _delay_us(3333.333);
}
static inline void delay833333ns(){
    _delay_us(833.333);
}
static inline void delay208333ns(){
    _delay_us(208.333);
}
static inline void delay416667ns(){
    _delay_us(416.667);
}
static inline void delay104167ns(){
    _delay_us(104.167);
}
static inline void delay52083ns(){
    _delay_us(52.083);
}
static inline void delay26042ns(){
    _delay_us(26.042);
}
static inline void delay69444ns(){
    _delay_us(69.444);
}
static inline void delay17361ns(){
    _delay_us(17.361);
}
static inline void delay13021ns(){
    _delay_us(13.021);
}
static inline void delay6510ns(){
    _delay_us(6.51);
}
static inline void delay4340ns(){
    _delay_us(4.34);
}
static inline void delay15625ns(){
    _delay_us(15.625);
}
static inline void delay3906ns(){
    _delay_us(3.906);
}
static inline void delay13333ns(){
    _delay_us(13.333);
}
static inline void delay3333ns(){
    _delay_us(3.333);
}
static inline void delay8681ns(){
    _delay_us(8.681);
}
static inline void delay2170ns(){
    _delay_us(2.17);
}
static inline void delay1085ns(){
    _delay_us(1.085);
}
static inline void delay543ns(){
    _delay_us(0.543);
}
static inline void delay2000ns(){
    _delay_us(2.0);
}
static inline void delay500ns(){
    _delay_us(0.5);
}
static inline void delay1333ns(){
    _delay_us(1.333);
}
static inline void delay333ns(){
    _delay_us(0.333);
}
static inline void delay271ns(){
    _delay_us(0.271);
}
struct BitRate bitRates[] = {
    { 300 , delay3333333ns, delay833333ns },
    { 1200 , delay833333ns, delay208333ns },
    { 2400 , delay416667ns, delay104167ns },
    { 4800 , delay208333ns, delay52083ns },
    { 9600 , delay104167ns, delay26042ns },
    { 14400 , delay69444ns, delay17361ns },
    { 19200 , delay52083ns, delay13021ns },
    { 38400 , delay26042ns, delay6510ns },
    { 57600 , delay17361ns, delay4340ns },
    { 64000 , delay15625ns, delay3906ns },
    { 75000 , delay13333ns, delay3333ns },
    { 115200 , delay8681ns, delay2170ns },
    { 230400 , delay4340ns, delay1085ns },
    { 460800 , delay2170ns, delay543ns },
    { 500000 , delay2000ns, delay500ns },
    { 750000 , delay1333ns, delay333ns },
    { 921600 , delay1085ns, delay271ns },
    {0,0,0}
};
