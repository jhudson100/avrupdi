//routines for reporting success or failure

#include <stdint.h>
#include "util.h"
#include <stdio.h>
#include <setjmp.h>
#include <stdlib.h>
#include <string.h>
#include "report.h"

void outputStart(BOOL ok){
    fputs("{ \"ok\": ",stdout);
    if( ok )
        fputs("true",stdout);
    else
        fputs("false",stdout);
}

void outputEnd(){
    fputs("}\n",stdout);
}

void outputString(const char* key, const char* value){
    fputs(", \"",stdout);
    fputs(key,stdout);
    fputs("\": \"",stdout);
    fputs(value,stdout);
    fputs("\"",stdout);
}

void outputInt(const char* key, uint32_t value){
    fputs(", \"",stdout);
    fputs(key,stdout);
    fputs("\": ",stdout);
    char buff[10];      //4 billion = 10 characters with trailing null.
    itoa(value, buff, 10);
    fputs(buff,stdout);
}

static BOOL listHasData;
void outputListStart(const char* key){
    fputs(", \"",stdout);
    fputs(key,stdout);
    fputs("\": [",stdout);
    listHasData=FALSE;
}

void outputListItem(uint32_t value){
    if(listHasData){
        fputc(',',stdout);
    }
    listHasData=TRUE;
    char buff[10];      //4 billion = 10 characters with trailing null.
    itoa(value, buff, 10);
    fputs(buff,stdout);
}


void outputListEnd(){
    fputs("]",stdout);
}

void success0(void){
    outputStart(TRUE);
    outputEnd();
}

void success1(const char* key, uint32_t value){
    outputStart(TRUE);
    outputInt(key,value);
    outputEnd();
}
void success2(const char* key1, uint32_t value1, const char* key2, uint32_t value2){
    outputStart(TRUE);
    outputInt(key1,value1);
    outputInt(key2,value2);
    outputEnd();
}

#define MAX_ERROR_MESSAGE_LENGTH 32

//for error handling in lieu of exceptions:
//If an error occurs, we set these variables and
//do a longjmp back into main
enum ErrorType {
    NO_ERROR=0, PARITY_ERROR=1, FRAME_ERROR=2,
    PROTOCOL_ERROR=3, TIMEOUT_ERROR=4, VERIFICATION_ERROR=5
};

struct ParityError{
    uint8_t value;
    uint8_t parity;
    uint8_t stops;
};

struct FrameError{
    uint8_t value;
    uint8_t parity;
    uint8_t stops;
};

struct VerificationError{
    uint16_t index;
    uint8_t value;
    uint8_t expected;
};

struct TimeoutError{
    uint8_t value;
};

struct ProtocolError{
    uint8_t value;
};

union ErrorInfoData {
    struct ParityError parityError;
    struct FrameError frameError;
    struct VerificationError verificationError;
    struct ProtocolError protocolError;
    struct TimeoutError timeoutError;
};

struct ErrorInfo{
    enum ErrorType type;
    char msg[MAX_ERROR_MESSAGE_LENGTH];
    union ErrorInfoData info;
};

struct ErrorInfo errorInfo;
extern jmp_buf errorJumpBuffer;

void copyErrorMessage(char* dest, const char* src){
    strncpy(dest,src,MAX_ERROR_MESSAGE_LENGTH);
    errorInfo.msg[MAX_ERROR_MESSAGE_LENGTH-1] = '\0';
}

void ParityError(const char* msg, uint8_t value, uint8_t parity, uint8_t stops){
    errorInfo.type = PARITY_ERROR;
    copyErrorMessage(errorInfo.msg,msg);
    errorInfo.info.parityError.value=value;
    errorInfo.info.parityError.parity = parity;
    errorInfo.info.parityError.stops = stops;
    longjmp(errorJumpBuffer,1);
}

void FrameError(const char* msg, uint8_t value, uint8_t parity, uint8_t stops){
    errorInfo.type = FRAME_ERROR;
    copyErrorMessage(errorInfo.msg,msg);
    errorInfo.info.frameError.value=value;
    errorInfo.info.frameError.parity = parity;
    errorInfo.info.frameError.stops = stops;
    longjmp(errorJumpBuffer,1);
}

void ProtocolError(const char* msg, uint8_t value){
    errorInfo.type = PROTOCOL_ERROR;
    copyErrorMessage(errorInfo.msg,msg);
    errorInfo.info.protocolError.value=value;
    longjmp(errorJumpBuffer,1);
}

void TimeoutError(const char* msg, uint8_t value){
    errorInfo.type = TIMEOUT_ERROR;
    copyErrorMessage(errorInfo.msg,msg);
    errorInfo.info.timeoutError.value = value;
    longjmp(errorJumpBuffer,1);
}

void VerificationError(const char* msg, uint16_t index, uint8_t value, uint8_t expected){
    errorInfo.type = VERIFICATION_ERROR;
    copyErrorMessage(errorInfo.msg,msg);
    errorInfo.info.verificationError.index = index;
    errorInfo.info.verificationError.value = value;
    errorInfo.info.verificationError.expected = expected;
    longjmp(errorJumpBuffer,1);
}

void reportError(){
    outputStart(FALSE);
    outputInt("type",errorInfo.type);
    outputString("msg", errorInfo.msg);

    switch(errorInfo.type){
        case PARITY_ERROR:
            outputInt("parity", errorInfo.info.parityError.parity);
            outputInt("stops",  errorInfo.info.parityError.stops);
            outputInt("value",  errorInfo.info.parityError.value);
            break;
        case FRAME_ERROR:
            outputInt("parity", errorInfo.info.frameError.parity);
            outputInt("stops",  errorInfo.info.frameError.stops);
            outputInt("value",  errorInfo.info.frameError.value);
            break;
        case PROTOCOL_ERROR:
            outputInt("value",  errorInfo.info.protocolError.value);
            break;
        case TIMEOUT_ERROR:
            outputInt("value",  errorInfo.info.timeoutError.value);
            break;
       case VERIFICATION_ERROR:
            outputInt("index",errorInfo.info.verificationError.index);
            outputInt("value",errorInfo.info.verificationError.value);
            outputInt("expected",errorInfo.info.verificationError.expected);
            break;
        default:
            break;
    }

    outputEnd();

}


void resetError(){
    errorInfo.type = NO_ERROR;
}
