#pragma once

#include <stdio.h>
#include <avr/io.h>
#include <avr/wdt.h>

typedef uint8_t BOOL;
#define TRUE 1
#define FALSE 0

void sleep(uint8_t msec);
void resetAVR();
void setUPDILED(BOOL on);


#if 0
static const char* hx = "0123456789abcdef";
static void OUTU8(uint8_t v){
    fputs(" 0x",stdout);
    fputc(hx[v>>4],stdout);
    fputc(hx[v&0xf],stdout);
}
static void OUTU16(uint16_t v1){
    fputs(" 0x",stdout);
    fputc(hx[(v1>>12)&0xf],stdout);
    fputc(hx[(v1>>8)&0xf],stdout);
    fputc(hx[(v1>>4)&0xf],stdout);
    fputc(hx[v1&0xf],stdout);
}

static inline void DEBUG(const char* s){
    fputs("DBG>",stdout);
    fputs(s,stdout);
    fputc('\n',stdout);
}
static inline void DEBUGU8(const char* s, uint8_t v){
    fputs("DBG>",stdout);
    fputs(s,stdout);
    OUTU8(v);
    fputc('\n',stdout);
}
static inline void DEBUGU8U8(const char* s, uint8_t v, uint8_t v2){
    fputs("DBG>",stdout);
    fputs(s,stdout);
    OUTU8(v);
    fputc(' ',stdout);
    OUTU8(v2);
    fputc('\n',stdout);
}
static inline void DEBUGU16(const char* s, uint16_t v1){
    fputs("DBG>",stdout);
    fputs(s,stdout);
    OUTU16(v1);
    fputc('\n',stdout);
}
#else
#define DEBUG(x)
#define DEBUGU8(x,y)
#define DEBUGU8U8(x,y,z)
#define DEBUGU16(x,y)
#endif
