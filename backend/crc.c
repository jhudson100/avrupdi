#include <stdint.h>
#include <stdio.h>

static const uint32_t poly = 0x11021;
static const uint16_t poly16 = 0x1021;

uint16_t crcbyte(uint16_t reg, uint8_t n){
    uint8_t mask=0x80;
    for(uint8_t i=0;i<8;++i){
        uint16_t top = reg & 0x8000;
        reg <<= 1;
        if( n & mask )
            reg |= 1;
        if(top)
            reg ^= poly16;
        mask>>=1;
    }
    return reg;
}


uint16_t crcbyte2(uint32_t reg, uint8_t n){
    reg <<= 8;
    reg |= n;
    uint32_t mask = 0x800000;
    uint32_t p = poly << 7;
    for(uint8_t i=0;i<8;++i){
        if(reg & mask)
            reg ^= p;
        p>>=1;
        mask>>=1;
    }
    return reg;
}
int main(int argc, char* argv[])
{
    uint32_t reg = 0;
    const char* msg = "ABCDEFGHIJKL";
    for(int i=0;msg[i];i++){
        reg = crcbyte(reg,msg[i]);
    }
    reg = crcbyte(reg,0);
    reg = crcbyte(reg,0);
    printf("CRC: %04x (expected 0x56fa)\n",reg);
    return 0;
}
