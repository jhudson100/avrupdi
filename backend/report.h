#pragma once

#include "util.h"

//routines for reporting success or failure

//call this to report the error set with one of the XXXError() functions
void reportError();

//Call these to report success
void success0(void);
void success1(const char* key, uint32_t value);
void success2(const char* key1, uint32_t value1, const char* key2, uint32_t value2);

//these set the error data for the specified error and
//transfer to the error handler via longjmp
void ParityError(const char* msg, uint8_t value, uint8_t parity, uint8_t stops);
void FrameError(const char* msg, uint8_t value, uint8_t parity, uint8_t stops);
void ProtocolError(const char* msg, uint8_t value);
void TimeoutError(const char* msg, uint8_t value);
void VerificationError(const char* msg, uint16_t index, uint8_t value, uint8_t expected);

void resetError();

//begin json output; add the "ok" key/value pair
void outputStart(BOOL ok);

//end the json output
void outputEnd();

//output a string with the given key. Note: No escaping
//is done for characters in the string.
void outputString(const char* key, const char* value);

//output an integer value
void outputInt(const char* key, uint32_t value);

//begin outputting a list of numbers
void outputListStart(const char* key);

//output another item in the list of numbers; outputListStart must
//have been called already.
void outputListItem(uint32_t value);

//end the list of numbers
void outputListEnd();
