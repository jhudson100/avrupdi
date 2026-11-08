#include "conf.h"
#include <avr/io.h>
#include <util/delay.h>
#include <stdio.h>
#include "updi.h"
#include "util.h"
#include "report.h"
#include "delays.h"

//constants for manual send (bit banging)
//at default 4MHz clock on target AVR, baud rate should
//be between 75bps and 225kbps
//BITS_PER_MICROSECOND = BITS_PER_SECOND / 1_000_000
//So MICROSECONDS_PER_BIT = 1/BITS_PER_MICROSECOND
//Thus, MICROSECONDS_PER_BIT = 1_000_000 / BITS_PER_SECOND
// 75bps: 13_333 us/bit
// 64kbps: 15.625 us/bit
// 225kbps: 4us/bit
//~ #define BITS_PER_SECOND  115200
//~ #define MICROSECONDS_PER_BIT (1'000'000 / BITS_PER_SECOND)
//~ #define BIT_TIME_US MICROSECONDS_PER_BIT
//~ #define QUARTER_BIT_TIME_US  (BIT_TIME_US >> 2)

#define DEFAULT_BAUD_RATE 38400
BitDelayFunc bitTimeDelayFunc;
BitDelayFunc quarterBitTimeDelayFunc;

//guard time when we switch line from input to output
#warning could make this shorter?
static inline
void guardTime(){
    bitTimeDelayFunc();
    bitTimeDelayFunc();
    bitTimeDelayFunc();
    bitTimeDelayFunc();
}

static inline
void setOutput(BOOL high){
    if(high)
        UPDI_PORT.OUTSET = UPDI_PIN;
    else
        UPDI_PORT.OUTCLR = UPDI_PIN;
    UPDI_PORT.DIRSET = UPDI_PIN;
}

static inline
void setInput(){
    //Set the updi pin to input mode
    UPDI_PORT.DIRCLR = UPDI_PIN;
}

static inline
void setLow(){
    UPDI_PORT.OUTCLR = UPDI_PIN;
}

static inline
void setHigh(){
    UPDI_PORT.OUTSET = UPDI_PIN;
}

void sendBreak(){
    //send break to remote. Datasheet recommends we
    //pull line low for 25 msec and do it twice.
    setOutput(0);
    _delay_ms(25);
    setOutput(1);
    _delay_ms(1);
    setOutput(0);
    _delay_ms(25);
    setOutput(1);
    _delay_ms(1);
    setInput();
    _delay_ms(1);
}

static inline
void sendBit(uint8_t b){
    setOutput(b&1);
    bitTimeDelayFunc();
}

static const uint8_t parityTable[16] = {
    0,1,1,0,1,0,0,1,
    1,0,0,1,0,1,1,0
};

static
void sendByte(uint8_t b){
    //Send start bit, 8 data bits, parity bit, two stop bits

    guardTime();

    setOutput(TRUE);

    uint8_t parity=parityTable[b&0xf] ^ parityTable[b>>4];

    sendBit(0);  //start bit
    for(uint8_t i=0;i<8;++i){
        sendBit(b);
        b>>=1;
    }
    sendBit(parity);
    sendBit(1);      //stop bit

    //for the second stop bit, just let the line stay high
    //under the influence of the target's internal pull-up resistor.
    setInput();
    bitTimeDelayFunc();

}

static inline
void sendSync(){
    //Send a sync byte
    sendByte(0x55);
}

static inline
uint8_t recvBit(){
    return ((UPDI_PORT.IN & UPDI_PIN ) ? 1:0);
}

//milliseconds
uint16_t updiReceiveTimeout=4000;

void setReceiveTimeout(uint16_t msec){
    updiReceiveTimeout = msec;
}


static inline
uint8_t recvByte(){
    setUPDILED(TRUE);

    setInput();

    //Two possibilities, depending on whether
    //counter wraps:

    //Possibility 1: Deadline did not wrap (but cnt
    // might wrap by the time we test it)
    // Timed out: cnt>=deadline or cnt < now
    //  +--------------------+-----------+----------+
    //  0                   now         deadline  65535

    //Possibility 2: Deadline does wrap
    // Timed out: cnt >= deadline and cnt < now
    //  +----+----------------+----------+
    //  0   deadline        now        65535

    //Alternate strategy: Scale down by -now before doing any tests.
    //This will wrap if negative values are computed.

    uint16_t now = RTC.CNT;

    while( recvBit() && RTC.CNT-now <= updiReceiveTimeout ){
    }

    if( recvBit() ){
        //timed out
        sendBreak();
        uint8_t err = LDCS(0x01); //read error register
        setUPDILED(FALSE);
        TimeoutError("Timedout",err);
    }

    //has gone low, so we are looking at start bit
    //delay for one bit period + one quarter of bit time
    //to ensure we're not right on the edge
    //~ _delay_us(BIT_TIME_US+QUARTER_BIT_TIME_US);
    bitTimeDelayFunc();
    quarterBitTimeDelayFunc();


    uint8_t v=0;
    uint8_t exppar = 0;     //expected parity
    for(uint8_t i=0;i<8;++i){
        uint8_t b = recvBit();
        v >>= 1;
        if(b)
            v |= 0x80;
        exppar ^= b;
        bitTimeDelayFunc();
        //_delay_us(BIT_TIME_US);
    }

    uint8_t par = recvBit();
    bitTimeDelayFunc();
    //~ _delay_us(BIT_TIME_US);

    uint8_t stop1 =  recvBit();
    bitTimeDelayFunc();
    //~ _delay_us(BIT_TIME_US);

    uint8_t stop2 = recvBit();

    //_delay_us(BIT_TIME_US);
    //must sleep after second stop bit
    //or else (if we're quick to send a bit to the remote),
    //the remote updi may report bus contention

    setUPDILED(FALSE);

    if(stop1 != 1 || stop2 != 1)
        FrameError("Stops", v,par, (stop1<<1) | stop2);

    if(exppar != par)
        ParityError("Parity error", v,par,(stop1<<1)|stop2);

    return v;
}


static inline
void waitAck(){
    //Wait for an ack byte

    setInput();

    //wait for a quarter of a bit time (guard bit).
    //This also lets the pull-up
    //bring the line high (although it should
    //have been high from the stop bit)
    //and avoids bus contention and spurious start bit
    //detection
    quarterBitTimeDelayFunc();

    uint8_t b = recvByte();
    if(b != 0x40){
        ProtocolError("WaitAck",b);
    }
    return;
}

uint32_t LD(PointerType ptrType, DataSize dataSize){
    DEBUG("LD");
    sendSync();
    uint8_t cmd = 0b0010'0000 | (ptrType<<2) | dataSize;
    sendByte(cmd);
    uint32_t v = 0;
    for(uint8_t i=0;i<=dataSize;++i){
        uint32_t tmp = recvByte();
        tmp <<= (i*8);
        v |= tmp;
    }
    return v;
}


void ST(PointerType ptrType, DataSize dataSize, uint32_t data){
    //Store to memory location.
    //If data is two or three bytes, send lsByte first
    //~ DEBUG("ST");

    sendSync();
    uint8_t cmd = 0b0110'0000 | (ptrType<<2) | (dataSize) ;
    //~ DEBUGU8("cmd",cmd);
    sendByte(cmd);

    //dataSize is 00 (for one byte), 01 (for two bytes), or 10 (for three bytes)
    //Thus, we add one for the loop count
    for(uint8_t i=0;i<=dataSize;++i){
        //~ DEBUGU8("data",data&0xff);
        sendByte( data & 0xff );
        data >>= 8;
    }
    //~ DEBUG("STack?");
    waitAck();
    //~ DEBUG("ST!");
}


uint8_t LDCS(uint8_t addr){
    //load (read) control/status register
    sendSync();
    sendByte( 0b1000'0000 | addr );
    uint8_t d = recvByte();
    return d;
}

void STCS(uint8_t addr,uint8_t data){
    //store (write) control/status register
    sendSync();
    sendByte( 0b1100'0000 | addr );
    sendByte(data);
    setInput();
}

void REPEAT_1(uint8_t count){
    //NOTE: Actual repeat value is count+1
    //(so repeat of 256 is denoted by count==255)
    sendSync();
    sendByte(0b1010'0000 | SIZE_BYTE);
    sendByte( count );
}

void enableUpdi(){
    //initialize updi system for communication

    sendBreak();

    setOutput(1);
    _delay_ms(100); //let line settle and sync with avr

    //line must be low between 10-200us
    setLow();
    _delay_us(100);
    //~ setHigh();

    //Release line. UPDI will hold it low until it's ready for comm,
    //but this is usually pretty fast.
    setInput();
    while( 0 == recvBit() ){
    }

    //Then UPDI releases line and pull-up brings it high
    //Within 200us - 13.5ms, send a sync character to initiate transmission
    sendSync();

    setInput();

    //let everything stabilize
    _delay_ms(10);

    //ensure avr is not stuck in some previous transaction
    sendBreak();


    return;
}


void ST_REPEAT(uint8_t* data, uint8_t lengthMinusOne){
    //lengthMinusOne=the length of the block minus one
    //  ex: If the block is 256 bytes, lengthMinusOne is 255.

    //if updi's pointer has not been initialized,
    //the caller must call:
    //   ST(PTR_ITSELF, SIZE_WORD, addr )
    //before calling this function. addr is the
    //destination address for the transfer. Since this
    //function uses ST(PTR_DEREF_POSTINC) to start the
    //transfer, sendBlock() may be called repeatedly
    //without updating ptr in the meantime.

    //~ DEBUGU8("ST_REPEAT",lengthMinusOne);

    if( lengthMinusOne > 0 ){
        //~ DEBUG("REPEAT_1");
        REPEAT_1(lengthMinusOne);
    }

    //~ DEBUGU8("*(ptr++)",*data);
    ST(PTR_DEREF_POSTINC, SIZE_BYTE, *data++ );

    //if length is 1, lengthMinusOne is zero and this loop never executes.
    //if length is 2, lengthMinusOne is 1 and this loop executes once.
    //etc.
    while(lengthMinusOne > 0 ){
        //~ DEBUGU8("data",*data);
        sendByte(*data++);
        lengthMinusOne--;
        //response from UPDI.
        //recvByte checks parity and stop bits
        uint8_t v = recvByte();
        //~ DEBUGU8("ack",v);
        if( v != 0x40 )
            ProtocolError("Send block ack",v);
    }
    //~ DEBUG("ST_REPEAT done");
}

void LD_REPEAT(uint8_t* data, uint8_t lengthMinusOne){

    //receive data and stream to stdout.
    //Size = lengthMinusOne + 1
    // (ex: For a count of 256, set lengthMinusOne to 255).

    //if updi's pointer has not been initialized,
    //the caller must call:
    //   ST(PTR_ITSELF, SIZE_WORD, addr )
    //before calling this function. addr is the
    //source address for the transfer. Since this
    //function uses LD(PTR_DEREF_POSTINC) to start the
    //transfer, this function may be called repeatedly
    //without updating ptr in the meantime.
    // We do byte receives here since there are no ack's;
    // it's not expected that doing word receives would
    // improve the speed significantly.

    if(lengthMinusOne > 0 )
        REPEAT_1(lengthMinusOne);

    sendSync();

    //LD command, no ack
    sendByte(0b0010'0000 | (PTR_DEREF_POSTINC<<2) | SIZE_BYTE );
    *data++ = recvByte();
    //if count is 1, lengthMinusOne is zero and the loop does not execute.
    //if count is 2, lengthMinusOne is 1 and the loop executes once
    while(lengthMinusOne > 0 ){
        *data++ = recvByte();
        lengthMinusOne--;
    }
}


uint16_t LD_REPEAT_VERIFY(uint8_t* data, uint8_t lengthMinusOne, uint8_t* badValue){
    //like LD_REPEAT but instead of copying data
    //into the 'data' pointer, verify the contents match.
    //Return the index of the first mismatch or length
    //(i.e., lengthMinusOne+1) if it all matches

    //lengthMinusOne is the number of bytes to verify minus one
    // (ex: For a count of 256, set lengthMinusOne to 255).

    //This assumes ST(PTR_ITSELF, ... ) has already been called.

    if(lengthMinusOne > 0 )
        REPEAT_1(lengthMinusOne);

    uint16_t idx=0;
    uint16_t badIndex=lengthMinusOne+1;
    BOOL ok=TRUE;

    sendSync();

    //LD command, *(ptr++), byte size
    sendByte(0b0010'0100 );

    uint8_t v = recvByte();
    if( *data != v ){
        ok=FALSE;
        *badValue = v;
    } else {
        data++;
        idx++;
    }

    //we can't do an early exit since
    //the remote will send all the data we requested.

    //if count is 1, lengthMinusOne is zero and the loop does not execute.
    //if count is 2, lengthMinusOne is 1 and the loop executes once
    while(lengthMinusOne > 0 ){
        v = recvByte();

        if(ok && *data != v ){
            ok=FALSE;
            *badValue=v;
            badIndex=idx;
        }
        data++;
        idx++;
        lengthMinusOne--;
    }
    return badIndex;
}


void KEY(const uint8_t key[8]){

    sendSync();
    sendByte( 0b1110'0000 );
    for(uint8_t i=0;i<8;++i){
        //~ DEBUGU8("KEY",key[i]);
        sendByte(key[i]);
    }
}

void SIB(uint8_t sib[16]){

    sendSync();
    sendByte( 0b1110'0101 );
    for(uint8_t i=0;i<16;++i){
        sib[i] = recvByte();
    }
}

void STS(uint32_t addr, uint8_t value){
    //~ DEBUG("STS");


    sendSync();
    //~ if( addr < 256 ){
        //~ DEBUG("BYTE ADDR");
        //~ sendByte(0b0100'0000);
        //~ sendByte(addr&0xff);
        //~ waitAck();
        //~ sendByte(value);
        //~ waitAck();
    //~ } else
    if( addr <= 0xffff ){
        //~ DEBUG("WORD ADDR");
        sendByte(0b0100'0100);
        sendByte(addr&0xff);
        sendByte((addr>>8)&0xff);
        //~ DEBUG("WAITACK");
        waitAck();
        //~ DEBUG("SENDVAL");
        sendByte(value);
        //~ DEBUG("WAITACK");
        waitAck();
        //~ DEBUG("DONE");
    } else {
        //~ DEBUG("LONG ADDR");
        sendByte(0b0100'1000);
        sendByte(addr&0xff);
        sendByte((addr>>8)&0xff);
        sendByte((addr>>16)&0xff);
        waitAck();
        sendByte(value);
        waitAck();
    }
}

uint8_t LDS(uint32_t addr){

    sendSync();
    //~ if( addr < 256 ){
        //~ sendByte(0b0000'0000);
        //~ sendByte(addr&0xff);
        //~ return recvByte();
    //~ } else
    if( addr <= 0xffff ){
        //~ DEBUG("LDS");
        sendByte(0b0000'0100);
        sendByte(addr&0xff);
        sendByte((addr>>8)&0xff);
        return recvByte();
    } else {
        sendByte(0b0000'1000);
        sendByte(addr&0xff);
        sendByte((addr>>8)&0xff);
        sendByte((addr>>16)&0xff);
        return recvByte();
    }
}

BOOL updiSetBitRate(uint32_t bitRate){
    for(uint8_t i=0; ; i++){
        if( bitRates[i].bitsPerSecond == 0 )
            return FALSE;
        if( bitRates[i].bitsPerSecond == bitRate ){
            bitTimeDelayFunc = bitRates[i].bitTimeDelayFunc;
            quarterBitTimeDelayFunc = bitRates[i].quarterBitTimeDelayFunc;
            return TRUE;
        }
    }
}

void updiInit(){
    //don't enable pullup: UPDI target has
    //its own 20k pullup, and we should
    //not have a second one
    setInput();
    bitTimeDelayFunc = bitRates[0].bitTimeDelayFunc;
    quarterBitTimeDelayFunc = bitRates[0].quarterBitTimeDelayFunc;
    updiSetBitRate(DEFAULT_BAUD_RATE);
}
