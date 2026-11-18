import zlib
import binascii

poly = 0x11021
polysize = 16

poly1 = (poly << 1) & 0xffff
poly2 = (poly << 2) & 0xffff
poly3 = (poly << 3) & 0xffff

def nibble(n):
    global reg,reg2
    top = (reg >> (polysize-4) )&0xf
    reg <<= 4
    reg |= n
    if top & 0x8:
        reg ^= (poly<<3)
    if top & 0x4:
        reg ^= (poly<<2)
    if top & 0x2:
        reg ^= (poly<<1)
    if top & 0x1:
        reg ^= (poly   )

    for i in range(4):
        bit = 1 if (n & (0x8>>i)) else 0
        reg2 <<= 1
        reg2 |= bit
        if reg2 & (1<<(polysize)):
            reg2 ^= poly

def bytex(reg,n):

    #           abcdefgh ijklmnop
    # abcdefgh  ijklmnop 00000000
    # abcdefgh  ijklmnop ABCDEFGH
    #           hijklmno pABCDEFG
    #         h ijklmnop ABCDEFGH
    #        1  pppppppp pppppppp
    # 1ppppppp  pppppppp p       .

    #1pppppppppppppppp
    #         mmmmmmmm
    reg <<= 8
    reg |= n

    mask = 0x800000
    p = poly << 7
    for i in range(8):
        if reg & mask:
            reg ^= p
        p>>=1
        mask>>=1
        
    return reg
    
def byte(reg3,b,reflin):
    for i in range(8):
        if reflin:
            bit = 1 if (b & (0x01<<i)) else 0
        else:
            bit = 1 if (b & (0x80>>i)) else 0
        reg3 <<= 1
        reg3 |= bit
        if reg3 & (1<<(polysize)):
            reg3 ^= poly
        assert reg3 < ( (1<<polysize+1))
    return reg3

M=b"ABCDEFGHIJKL"
reg = 0x0
reg2 = reg
reg3 = reg
reg4 = reg
reg5 = reg

print("binascii:",hex( binascii.crc_hqx(M,reg) ))

for b in M:
    n1 = (b>>4)&0xf
    n2 = b&0xf
    nibble(n1)
    nibble(n2)
    reg4 = byte(reg4,b,False)
    reg5 = bytex(reg5,b)
    
for i in range(polysize>>2):
    nibble(0)
for i in range(polysize>>3):
    reg4=byte(reg4,0,False)
    reg5=bytex(reg5,0)
    
print(hex(reg))
print(hex(reg2))
print(hex(reg4))
print(hex(reg5))


