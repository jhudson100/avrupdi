#pragma once

#include "util.h"

typedef enum DataSize_ {
    SIZE_BYTE = 0b00,
    SIZE_WORD = 0b01,
    SIZE_3BYTE = 0b10        //3 bytes
} DataSize;

typedef enum PointerType_ {
    PTR_DEREF_POSTINC = 0b01,
    PTR_ITSELF = 0b10
} PointerType;

extern uint16_t updiReceiveTimeout;
void updiInit();
void sendBreak();
void ST(PointerType ptrType, DataSize dataSize, uint32_t data);
uint32_t LD(PointerType ptrType, DataSize dataSize);
uint8_t LDCS(uint8_t addr);
void STCS(uint8_t addr,uint8_t data);
void enableUpdi();
void ST_REPEAT(uint8_t* data, uint8_t lengthMinusOne);
void LD_REPEAT(uint8_t* data, uint8_t lengthMinusOne);
uint16_t LD_REPEAT_VERIFY(uint8_t* data, uint8_t lengthMinusOne, uint8_t* badValue);
void STS(uint32_t addr, uint8_t value);
uint8_t LDS(uint32_t addr);
void KEY(const uint8_t key[8]);
void SIB(uint8_t sib[16]);
void setReceiveTimeout(uint16_t msec);
BOOL updiSetBitRate(uint32_t bitRate);
