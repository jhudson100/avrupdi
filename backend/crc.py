
poly = 0x04c11db7
poly0 = (poly     ) & 0xffffffff
poly1 = (poly << 1) & 0xffffffff
poly2 = (poly << 2) & 0xffffffff
poly3 = (poly << 3) & 0xffffffff
poly4 = (poly << 4) & 0xffffffff

#top 4: 0000
#bottom 4: 0111
msg=b"123456789"


def nibble(n):
    global reg
    top = (reg >> 28)&0xf
    reg <<= 4
    reg &= 0xffffffff
    reg |= n
    if top & 0x8:
        reg ^= (poly0<<3)
    if top & 0x4:
        reg ^= (poly0<<2)
        #reg ^= 0b0100
    if top & 0x2:
        reg ^= (poly0<<1)
        #reg ^= 0b0110
    if top & 0x1:
        reg ^= (poly0)

reg = 0xffffffff
for b in msg:
    nibble((b>>4)&0xf)
    nibble(b&0xf)

for i in range(8):
    nibble(0)
    nibble(0)

print(hex(reg),hex(reg^0xffffffff))

    


