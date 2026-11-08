//The backend to handle low-level communication

#include "conf.h"
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdio.h>
#include <string.h>
#include <util/delay.h>
#include <stdint.h>
#include <setjmp.h>
#include <ctype.h>
#include <stdlib.h>
#include "serial.h"
#include "updi.h"
#include "report.h"
#include "util.h"


jmp_buf errorJumpBuffer;

#define BACKEND_VERSION 0x2658
//~ uint16_t TIMEOUT_MS=2000;

BOOL connected=FALSE;

#define MAX_LINE_LENGTH 64

char* nextToken(char* toScan){
    static char empty[1] = {0};
    static char* p=empty;

    if( toScan ){
        p=toScan;
    }

    if(!p){
        p = empty;
    }

    while( *p && isspace(*p) )
        ++p;

    char* ret = p;

    while( *p && !isspace(*p) )
        ++p;

    if( *p == '\0' ){
        //at end of string
    } else {
        //not at end of string
        *p='\0';
        p++;
    }
    return ret;
}

//read line into p and terminate with null (newline
//is not stored). maxLength = total length of buffer,
//including null byte. If newline is not encountered
//before buffer is exhausted, return false.
BOOL readLine(char* p, unsigned maxLength){
    while(1){
        if( maxLength == 0 )
            return FALSE;
        char c = fgetc(stdin);

        //~ #warning
        //~ static const char* H="0123456789abcdef";
        //~ fputs("RL(",stdout);
        //~ fputc(H[(c>>4)&0xf],stdout);
        //~ fputc(H[c&0xf],stdout);
        //~ fputc(')',stdout);

        if( c == '\r' || c == '\n' ){
            *p='\0';
            return TRUE;
        } else {
            *p = c;
            p++;
            maxLength--;
        }
    }
}

void sendBlockOfDataBack( const uint8_t* block, uint16_t size ){
    outputStart(TRUE);
    outputListStart("data");
    const uint8_t* B = block;
    while(size){
        outputListItem(*B);
        B++;
        size--;
    }
    outputListEnd();
    outputEnd();
}

static uint8_t block[256];

//Read size bytes (binary data) from stdin
//and then use REPEAT ST to store it.
//Note: This function uses the global variable 'block'
//for temporary storage.
void do_st_repeat(uint16_t size){
    uint8_t* B = block;
    uint16_t ctr=size;
    while(ctr>0){
        uint8_t rv = fread(B,1,1,stdin);
        if( 1 != rv ){
            ProtocolError("fread",rv);
        }
        B++;
        ctr--;
    }
    DEBUG("ST_REPEAT");
    ST_REPEAT(block,size-1);
}


void CMD_ST_RV(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No addr",0);
    uint32_t addr = atol(p);
    p = nextToken(0);
    if(!p)
        ProtocolError("No size",0);
    uint16_t size = atol(p);
    if( size == 0 || size > 256)
        ProtocolError("Bad size",size);
    p = nextToken(0);
    if(!p)
        ProtocolError("No flags",0);
    uint8_t flags = atol(p);

    BOOL verifyAfterWrite = (flags&1);

    //100ms delay works here
    //20ms delay works here
    //10ms delay works here
    //5ms delay works here
    //2ms reliably results in the
    //remote avr losing sync or throwing
    //a bogus parity error
    _delay_ms(10);

    DEBUGU16("ST PTR_ITSELF",addr);

    if( addr <= 0xffff ){
        ST(PTR_ITSELF, SIZE_WORD, addr );
    } else {
        ST(PTR_ITSELF, SIZE_3BYTE, addr );
    }

    uint8_t* B = block;
    uint16_t ctr=size;
    while(ctr>0){
        uint8_t rv = fread(B,1,1,stdin);
        if( 1 != rv ){
            ProtocolError("fread",rv);
        }
        B++;
        ctr--;
    }
    DEBUG("ST_REPEAT");
    ST_REPEAT(block,size-1);

    if( verifyAfterWrite ){
        DEBUG("ST PTR_ITSELF AGAIN");
        if( addr <= 0xffff ){
            ST(PTR_ITSELF, SIZE_WORD, addr  );
        } else {
            ST(PTR_ITSELF, SIZE_3BYTE, addr );
        }
        DEBUG("LD_REPEAT_VERIFY");
        uint8_t badValue;
        uint16_t idx = LD_REPEAT_VERIFY(block,size-1,&badValue);
        if( idx == size ){
            success0();
        } else {
            VerificationError("Mismatch",idx,badValue,block[idx]);
        }
    } else {
        success0();
    }
}

void CMD_LD(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No size",0);
    uint8_t size = atoi(p);
    switch(size){
        case SIZE_BYTE:
        case SIZE_WORD:
        case SIZE_3BYTE:
            break;
        default:
            ProtocolError("Bad size",size);
    }
    p = nextToken(0);
    if(!p)
    ProtocolError("No target",0);
    uint8_t target = atoi(p);
    switch(target){
        case PTR_ITSELF:
        case PTR_DEREF_POSTINC:
            break;
        default:
            ProtocolError("Bad target",target);
    }
    uint32_t v = LD( size,target);
    success1("value",v);
}

void CMD_STS(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No addr",0);
    uint32_t addr = atol(p);
    p = nextToken(0);
    if(!p)
        ProtocolError("No value",0);
    uint8_t value = atol(p);
    STS(addr,value);
    success0();
}

void CMD_ST_REPEAT(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No size",0);
    uint16_t size = atol(p);
    if( size == 0 || size > 256)
        ProtocolError("Bad size",size);
    do_st_repeat(size);
    success0();
}

void CMD_STS_M(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No count",0);
    uint8_t count = atoi(p);
    for(uint8_t i=0;i<count;++i){
        p = nextToken(0);
        if(!p)
            ProtocolError("No value",0);
        uint32_t addr = atol(p);
        p = nextToken(0);
        if(!p)
            ProtocolError("No value",0);
        uint8_t value = atol(p);
        STS(addr,value);
    }
    success0();
}

void CMD_VERSION(){
    success1("version",BACKEND_VERSION);
}

void CMD_CONNECT_UPDI(){
    if( connected ){
        success1("already_connected",1);
    } else {
        enableUpdi();
        connected=TRUE;
        success1("already_connected",0);
    }
}

void CMD_BREAK(){
    if( !connected){
        ProtocolError("Not connected",0);
    }
    sendBreak();
    success0();
}

void CMD_LDCS(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No address",0);
    uint8_t v = LDCS(atol(p));
    success1("value",v);
}

void CMD_ST(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No pointer type",0);
    unsigned pointerType = atol(p);
    switch(pointerType){
        case PTR_DEREF_POSTINC:
        case PTR_ITSELF:
            break;
        default:
            ProtocolError("Bad pointer type",pointerType);
    }
    p = nextToken(0);
    if(!p)
        ProtocolError("No data size",0);
    unsigned dataSize = atol(p);
    switch(dataSize){
        case SIZE_BYTE:
        case SIZE_WORD:
        case SIZE_3BYTE:
            break;
        default:
            ProtocolError("Bad data size",dataSize);
    }
    p = nextToken(0);
    if(!p)
        ProtocolError("No data",0);
    uint32_t data = atol(p);    //use atol, not atol
    ST(pointerType,dataSize,data);
    success0();
}

void CMD_STCS(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No address",0);
    uint16_t addr = atol(p);
    p = nextToken(0);
    if(!p)
        ProtocolError("No value",0);
    uint8_t value = atol(p);
    STCS(addr,value);
    success0();
}

void CMD_LD_REPEAT(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No size",0);
    uint16_t size = atol(p);
    if( size == 0 || size > 256)
        ProtocolError("Bad size",size);
    LD_REPEAT(block,size-1);
    sendBlockOfDataBack( block, size );
    //sendBlockOfDataBack already wrote success/failure
}

void CMD_KEY(){
    for(uint8_t i=0;i<8;++i){
        char* p = nextToken(0);
        if(!p)
            ProtocolError("Bad key",0);
        block[i] = (uint8_t) atol(p);
    }
    KEY( block );
    success0();
}

void CMD_SIB(){
    SIB(block);
    sendBlockOfDataBack(block,16);
    //sendBlockOfDataBack already wrote success/failure
}


void CMD_LDS(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No addr",0);
    uint32_t addr = atol(p);
    success1( "value", LDS(addr) );
}


void CMD_TIMEOUT(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No timeout",0);
    uint16_t timeout = atol(p);
    setReceiveTimeout(timeout);
    success0();
}

void CMD_UPDISPEED(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No bit rate",0);
    uint32_t rate= atol(p);
    BOOL success = updiSetBitRate(rate);
    if( success )
        success0();
    else{
        outputStart(FALSE);
        outputEnd();
    }
}

void CMD_STS_W(){
    char* p = nextToken(0);
    if(!p)
        ProtocolError("No write count",0);
    uint8_t toWriteCount = atol(p);
    while(toWriteCount > 0 ){
        p = nextToken(0);
        if(!p)
            ProtocolError("No addr",0);
        uint16_t addr = atol(p);
        p = nextToken(0);
        if(!p)
            ProtocolError("No value",0);
        uint8_t val = atol(p);
        STS(addr,val);
        toWriteCount--;
    }
    p = nextToken(0);
    if(!p)
        ProtocolError("No poll addr",0);
    uint16_t toPollAddr = atol(p);
    p = nextToken(0);
    if(!p)
        ProtocolError("No mask",0);

    uint8_t pollMask = atol(p);
    uint16_t now = RTC.CNT;
    uint8_t pollValue = LDS(toPollAddr);
    while( pollValue & pollMask ){
        if( RTC.CNT-now > updiReceiveTimeout ){
            TimeoutError("Polling", pollValue);
        }
        pollValue = LDS(toPollAddr);
    }
    success0();
}

void processLine()
{
    static char line[MAX_LINE_LENGTH];

    if(!readLine(line,MAX_LINE_LENGTH)){
        ProtocolError("Line is too long",0);
    }

    char* action = nextToken(line);
    if(!action){
        ProtocolError("Bad action",0);
    }

    #define ELSEIF( tok )                                              \
        else if( 0 == strcmp( #tok, action ) ) { CMD_ ## tok (); }     \

    if( line[0] == '\0' || !action || action[0] == '\0' ){
        //empty; ignore
    }
    ELSEIF( ST_REPEAT )
    ELSEIF( LD_REPEAT )
    ELSEIF( ST_RV )
    ELSEIF( LDS )
    ELSEIF( STS )
    ELSEIF( STS_M )
    ELSEIF( STS_W )
    ELSEIF( LDCS )
    ELSEIF( STCS )
    ELSEIF( LD )
    ELSEIF( ST )
    ELSEIF( VERSION )
    ELSEIF( CONNECT_UPDI )
    ELSEIF( BREAK )
    ELSEIF( KEY )
    ELSEIF( SIB )
    ELSEIF( TIMEOUT )
    ELSEIF( UPDISPEED )
    else {
        ProtocolError("Bad command",0);
    }
}


void setConfLED(BOOL on){
    if( on )
        USBCONFLED_PORT.OUTCLR = USBCONFLED_PIN;
    else
        USBCONFLED_PORT.OUTSET = USBCONFLED_PIN;
}

int main(){

    wdt_disable();

    //set CPU clock to 24MHz
    _PROTECTED_WRITE( CLKCTRL.OSCHFCTRLA, 0x9<<2 );

    serialInit();

    //use internal 1k osc
    RTC.CLKSEL = 0x01;

    //wait until rtc is ready for config
    while(RTC.STATUS & 1 ){
    }
    //set up the RTC so we can implement timeouts
    //1k interrupts, not prescaled
    //enabled
    RTC.CTRLA = 1;


    USBCONFLED_PORT.DIRSET = USBCONFLED_PIN;
    setConfLED(FALSE);

    UPDILED_PORT.DIRSET = UPDILED_PIN;
    setUPDILED(FALSE);

    setUPDILED(TRUE);
    sleep(250);
    setUPDILED(FALSE);
    sleep(250);
    setUPDILED(TRUE);
    sleep(250);
    setUPDILED(FALSE);

    //bring MCP2221 out of reset
    USBRESET_PORT.DIRSET = USBRESET_PIN;
    USBRESET_PORT.OUTSET = USBRESET_PIN;

    updiInit();

    #warning add interrupt handler to dynamically change this
    //wait for USB to be connected
    while( USBCONF_PORT.IN & USBCONF_PIN ){
    }

    setConfLED(TRUE);

    fputs("\r\nAVR UPDI BACKEND\r\n",stdout);

    sei();

    while(1){
        fputs("\r\n@@>",stdout);

        resetError();
        int rv = setjmp(errorJumpBuffer);
        if( rv != 0 ){
            //we are here because of an error.
            reportError();
        } else {
            processLine();
        }
    }
    return 0;   //bogus
}
