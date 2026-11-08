#make bit rate table


bitrates = [300,1200,2400,4800,9600,14400,19200,38400,57600,
    64000, 75000, 115200,230400,460800,500000,750000,921600]
delays=set()


#with __builtin_avr_delay_cycles(), max delay on 24MHz
# CPU approx = 4,200,000 / 24 = 175000ms
def makeDelayFunc(delay):
    if delay not in delays:
        print(f"static inline void delay{delay}ns(){{",file=fp)
        print(f"    _delay_us({delay/1000});",file=fp)
        print("}",file=fp)
        delays.add(delay)

with open("delays.h","w") as fp:

    print("#pragma once",file=fp)
    print("//Automatically generated file; do not edit",file=fp)
    print("#include <util/delay.h>",file=fp)
    print("typedef void (*BitDelayFunc)(void);" ,file=fp)
    print("struct BitRate{"                     ,file=fp)
    print("    uint32_t bitsPerSecond;"         ,file=fp)
    print("    BitDelayFunc bitTimeDelayFunc;"  ,file=fp)
    print("    BitDelayFunc quarterBitTimeDelayFunc;",file=fp)
    print("};",file=fp)

    for bitrate in bitrates:
        nanosecondsPerBit = int(round(1_000_000_000/bitrate))
        nanosecondsPerQBit = int(round(1_000_000_000/bitrate/4))
        makeDelayFunc(nanosecondsPerBit)
        makeDelayFunc(nanosecondsPerQBit)

    print("struct BitRate bitRates[] = {",file=fp)
    for bitrate in bitrates:
        nanosecondsPerBit = int(round(1_000_000_000/bitrate))
        nanosecondsPerQBit = int(round(1_000_000_000/bitrate/4))
        print(f"    {{ {bitrate} , delay{nanosecondsPerBit}ns, delay{nanosecondsPerQBit}ns }},",file=fp)
    print("    {0,0,0}",file=fp)
    print("};",file=fp)
