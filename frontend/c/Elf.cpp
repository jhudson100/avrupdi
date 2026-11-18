#include "Elf.h"
#include <cstdint>

using std::uint8_t;
using std::uint16_t;
using std::uint32_t;


typedef uint32_t uintN_t;

#pragma pack(push,1)
struct ElfHeader{
    char magic[3];      //0x7f,E,L,F
    uint8_t klass;      //ELFCLASS32=32 bit, ELFCLASS64=64 bit
    uint8_t endian;     //ELFDATA2{L,M}SB
    uint8_t version;    //EV_CURRENT
    uint8_t abi;        ??
    uint8_t abiVersion; 
    uint8_t padding[8];
    uint16_t type;
    uint16_t machine;
    uint32_t version;
    uintN_t entry;
    uintN_t programHeaderOffset;
    uintN_t sectionHeaderOffset;
    uint32_t flags;
    uint16_t elfHeaderSize;
    uint16_t programHeaderEntrySize;
    uint16_t programHeaderCount;
    uint16_t sectionHeaderEntrySize;
    uint16_t sectionHeaderCount;
    uint16_t sectionHeaderStringIndex;
};

//32 bit format
struct ProgramHeader{
    uint32_t type;      //PT_NULL (skip), PT_LOAD (loadable), etc.
    uint32_t offset;
    uint32_t virtualAddr;
    uint32_t physicalAddr;
    uint32_t sizeOnDisk;
    uint32_t sizeInMemory;
    uint32_t flags;
    uint32_t alignment;
};

#pragma pack(pop)
ElfParser::ElfParser(std::fstream&& in_) : in(std::move(in_))
{
    ...read elf header

    if( elfHeader.magic != "\x7fELF"){
        error
    }
    
    if( elfHeader.type != ET_EXEC ){
        error
    }
    if( elfHeader.machine != ??AVR?? ){
        error
    }
    if( elfHeader.version != CURRENT ){
        error
    }
    if( elfHeader.programHeaderEntryCount == 0xffff ){
        //we don't support XNUM
        error
    }
    if( elfHeader.sectionHeaderEntryCount == 0xffff ){
        //we don't support XNUM
        error
    }
    

}

