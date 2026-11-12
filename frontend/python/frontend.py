#!/usr/bin/env python

# pip install --user --upgrade pyserial

# python3 frontend.py --gpio 12,13 --speed 200000  write test.bin


import serial                       #type: ignore
import serial.tools.list_ports      #type: ignore
import argparse
import sys
import time
import json
import struct
import io
from typing import Any,Callable
from typing import NamedTuple
from Exceptions import ProtocolError,Timeout
import UpdiLink
import profiling

# ~ from UpdiLink.UpdiLink import UpdiLink.UpdiLink, PTR_ITSELF, SIZE_WORD
# ~ from UpdiLink.UpdiLink import debugTimes as updiLinkDebugTimes
# ~ from UpdiLink.UpdiLink import STS_CALLS as updiLinkSTS_CALLS

from SerialPort import SerialPort
from log import debug,error

import chips
# ~ import ATmega4808
import AVR16EB14
import AVR32DD20
import AVR32DB32
import AVR64DB32
import AVR128DA48

EXPECTED_BACKEND_VERSION = 0x2658

def getErrorStatus(avrLink: UpdiLink.UpdiLink) -> int:
    #status B register
    v = avrLink.LDCS(0x01)
    return v

def getSignature(avrLink: UpdiLink.UpdiLink) -> tuple[int,int,int]:
    #signature row: pg 45: First three bytes are signature
    #signature row is at 0x1080
    srow0 = avrLink.LDS(0x1080)
    srow1 = avrLink.LDS(0x1081)
    srow2 = avrLink.LDS(0x1082)
    return (srow0,srow1,srow2)

def enableInterbyteDelay(avrLink: UpdiLink.UpdiLink) -> None :
    v = avrLink.LDCS(0x02)
    v |= (1<<7)
    avrLink.STCS(0x02, v )

def disableInterbyteDelay(avrLink: UpdiLink.UpdiLink) -> None :
    v = avrLink.LDCS(0x02)
    v &= ~(1<<7)
    avrLink.STCS(0x02, v )

def beginFlashWrite(avrLink: UpdiLink.UpdiLink) -> None:
    assert avrLink.chip != None
    # ~ debug("beginFlashWrite")
    avrLink.enableNVMProgramming()
    avrLink.chip.beginFlashWrite(avrLink)

def storeToFlash(avrLink: UpdiLink.UpdiLink, offset:int, value:int) -> None:
    assert avrLink.chip != None
    avrLink.chip.beginFlashWrite(avrLink)
    avrLink.chip.clearPageBuffer(avrLink)
    #don't erase since we only change one byte
    avrLink.chip.beginFlashPage(avrLink,False,offset)
    avrLink.chip.storeToFlash(avrLink,offset,value)
    #don't erase since we only want to change one byte
    avrLink.chip.endFlashPage(avrLink,False)
    avrLink.chip.finishFlashWrite(avrLink)



def eraseFlash(avrLink: UpdiLink.UpdiLink) -> None:
    assert avrLink.chip != None
    # ~ avrLink.enableErase()
    avrLink.enableNVMProgramming()
    avrLink.chip.eraseFlash(avrLink)

def readFlash(avrLink: UpdiLink.UpdiLink, start: int, count: int) -> list[int]:
    assert avrLink.chip != None
    tmp=[]
    for i in range(count):
        tmp.append(avrLink.LDS(avrLink.chip.getFlashStart()+start+i))
    return tmp

#documented in UPDI CONTROLA register
#0x0 = 128 cycles of guard time
#0x1 = 64 cycles of guard time
#...
#0x6 = 2 cycles of guard time
#0x7 = No guard time
def setGuardTime(avrLink: UpdiLink.UpdiLink,g:int) -> None:
    c = avrLink.LDCS(0x02)
    c &= ~0b111
    c |= g
    avrLink.STCS(0x02, c)

def readCompletely( fp: io.BufferedReader, count: int, notPast: int ) -> bytearray:
    """Read up to count bytes from fp, but not past file offset notPast"""
    result = bytearray()
    endOffset = min([ notPast, fp.tell() + count ] )
    count = endOffset - fp.tell()
    while count > 0:
        tmp = fp.read(count)
        if len(tmp) == 0:
            break
        result += tmp
        count -= len(tmp)
    return result


def sendFile(avrLink: UpdiLink.UpdiLink,filename: str,
    destinationOffset:int,
    skipUnchanged: bool,
    eraseBeforeWrite:bool,
    sizeLimit:int,
    sourceSkip:int,
    chipHasBeenErased: bool,
    verify: bool) -> None:

    assert avrLink.chip != None

    debug(f"<><>sendFile: Begin writing at most {sizeLimit} bytes at offset {destinationOffset}")

    numWritten = 0

    if chipHasBeenErased:
        debug("Chip was erased, so forcing eraseBeforeWrite to False")
        eraseBeforeWrite = False

    chip = avrLink.chip

    if 0 != destinationOffset % chip.getFlashPageSize():
        error("Destination offset must be a multiple of page size (",chip.getFlashPageSize()," bytes)")

    beginFlashWrite(avrLink)

    startTime = time.time()
    with open(filename,"rb") as fp:
        fp.seek(0,2)
        fsize = fp.tell() - sourceSkip
        if fsize < 0:
            fsize = 0
        fp.seek(sourceSkip)

        while True:
            debug("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~")
            numRead = fp.tell() - sourceSkip

            if numRead >= sizeLimit:
                print("Reached size limit; stopping")
                break

            if skipUnchanged:
                if chipHasBeenErased:
                    #we know flash is all 0xff's
                    debug("Chip was erased; assuming flash is all 0xff")
                    currentFlash = bytearray([0xff]*chip.getFlashPageSize())
                else:
                    debug("Getting current flash")
                    currentFlash = streamRead( avrLink,
                        chip.getFlashPageSize(),
                        destinationOffset + numRead
                    )
            else:
                currentFlash = bytearray()


            originalFileData=readCompletely( fp, chip.getFlashPageSize(), sourceSkip + sizeLimit )

            if len(originalFileData) == 0:
                #no more data to read
                break

            targetAddress = destinationOffset + numRead
            assert targetAddress % chip.getFlashPageSize() == 0
            assert len(originalFileData) <= chip.getFlashPageSize()

            if skipUnchanged and currentFlash[:len(originalFileData)] == originalFileData:
                changed="[Unchanged]"
            else:

                #if chip has >32KB of flash, we must map the section we want to write
                #into the chip's writeable window of addresses. This will give us a different
                #destination address than the physical address.
                addrToWrite = chip.mapFlashAddress( avrLink, targetAddress )

                debug("send_file: sendBlock will ST to",hex(addrToWrite),"...",hex(addrToWrite+len(originalFileData)-1))

                #strange issue with AVR64DBxx: Writing close to RAM address 0xffff
                #(which is either flash address 0x7fff or 32K+0x7fff)
                #causes NVM controller to start giving back bogus values
                #from then on for address and status registers.
                # Workaround: stop early and use STS to do the last few bytes
                RED_ZONE_START = 0xfffc
                numInRedZone = addrToWrite + len(originalFileData) - RED_ZONE_START
                if numInRedZone > 0:
                    debug("WILL DO SOME LATER")
                    doLater = originalFileData[-numInRedZone:]
                    fileData = originalFileData[:-numInRedZone]
                else:
                    doLater = bytearray()
                    fileData = originalFileData

                chip.beginFlashPage(avrLink,eraseBeforeWrite,targetAddress)

                st=time.time_ns()

                USE_REMOTE_VERIFY=False

                #a page might be > 256 bytes, so we
                #write 256 bytes at a time. We already limited
                #the size of fileData to be at most one page.
                for i in range(0,len(fileData),256):
                    if 0:
                        avrLink.REP_ST( addrToWrite+i, fileData[i:i+256] )
                        profiling.count("sendFile:REP_ST",time.time_ns()-st)
                    elif not USE_REMOTE_VERIFY:
                        avrLink.ST(UpdiLink.PTR_ITSELF, UpdiLink.SIZE_WORD, addrToWrite+i )
                        profiling.count("sendFile:ST",time.time_ns()-st)
                        avrLink.ST_REPEAT(fileData[i:i+256])
                        profiling.count("sendFile:ST_REPEAT+ST",time.time_ns()-st)
                    elif USE_REMOTE_VERIFY:
                        avrLink.ST_RV(addrToWrite+i,verify,fileData[i:i+256])
                    else:
                        avrLink.STS_REPEAT(addrToWrite+i,fileData[i:i+256])

                for dl in range(len(doLater)):
                    val = doLater[dl]
                    debug(f"STS 0x{doLater[dl]} to 0x{RED_ZONE_START+dl:x}")
                    avrLink.STS(RED_ZONE_START+dl, doLater[dl])

                chip.endFlashPage(avrLink,eraseBeforeWrite)

                numWritten += len(fileData)
                debug("send_file: endPage done")

                if verify and not USE_REMOTE_VERIFY:
                    readbackData = streamRead(avrLink, len(originalFileData), targetAddress)
                    if readbackData != originalFileData:
                        ok=True
                        for e in range(len(originalFileData)):
                            if readbackData[e] != originalFileData[e]:
                                error(f"At target address 0x{targetAddress+e:x}: Data does not match: Read back 0x{readbackData[e]:02x} but expected 0x{originalFileData[e]:02x}")
                    else:
                        debug("verify: Data matches")
                        changed="[Verified]"
                else:
                    debug("Not verifying")
                    changed=""


            pct = round(100*fp.tell()/fsize)
            print(pct,"% (",fp.tell(),"of",fsize,"bytes )",changed)

    chip.finishFlashWrite(avrLink)
    endTime = time.time()
    elapsed=endTime-startTime

    print(f"Wrote {numWritten} bytes in {endTime-startTime:.2f} seconds = "
          f"{round(numWritten*8/elapsed)} bits per second = "
          f"{round(numWritten/elapsed)} bytes per second")


def recvFile(avrLink: UpdiLink.UpdiLink,count: int, offset:int, fp: io.BufferedWriter) -> None:

    assert avrLink.chip != None

    # ~ debug("/"*20,"recvFile","/"*20)
    #write flash using st

    # ~ debug("Begin recvFile loop")
    startTime = time.time()
    i=0
    while i < count:
        addr = avrLink.chip.mapFlashAddress(avrLink,i+offset)
        avrLink.ST(UpdiLink.PTR_ITSELF, UpdiLink.SIZE_WORD, addr )
        toRead = min( [256, count-i] )
        rdata: bytes = avrLink.LD_REPEAT(count=toRead)
        fp.write(rdata)
        pct = round(100*fp.tell()/count)
        print(pct,"% (",fp.tell(),"of",count,"bytes )")
        i += toRead

    endTime = time.time()
    elapsed=endTime-startTime
    print(f"Read {count} bytes in {endTime-startTime:.2f} seconds = {round(count*8/elapsed)} bits per second")

def streamRead( avrLink: UpdiLink.UpdiLink,count:int, offset: int) -> bytearray:
    debug("streamRead: Read up to",count,"bytes starting at offset",offset)

    #bug: if we let the read get to 0xffff, the nvm controller state becomes
    #corrupted
    result = bytearray()
    while count > 0:
        tmp = streamReadOneBlock(avrLink,count,offset)
        result += tmp
        count -= len(tmp)
        offset += len(tmp)

    return result

def streamReadOneBlock(avrLink: UpdiLink.UpdiLink,count:int, offset: int) -> bytes:
    debug("streamReadOneBlock: Read up to",count,"bytes starting at offset",offset)

    toRead = min( [256, count] )

    assert avrLink.chip != None

    addr = avrLink.chip.mapFlashAddress(avrLink,offset)

    endAddr = addr + toRead - 1
    assert endAddr <= 0xffff,f"addr=0x{addr:x}, count={count}, endAddr=0x{endAddr:x}"
    if endAddr == 0xffff:
        doExtra=True
        toRead -= 1
    else:
        doExtra = False

    if addr != endAddr:
        avrLink.ST(UpdiLink.PTR_ITSELF, UpdiLink.SIZE_WORD, addr )
        rdata = avrLink.LD_REPEAT(count=toRead)
        #rdata = avrLink.LDS_REPEAT(addr,toRead)

    if doExtra:
        rdata = rdata + bytes([avrLink.LDS(endAddr)])

    debug(f"streamReadOneBlock: Got {[hex(q) for q in rdata[0:4]]}...")
    return rdata[:count]

#true if we know the chip has all 0xff's; used to
#optimize writing, especially for larger devices
chipHasBeenErased=False

def process_command(port:SerialPort, cmdlist: list[str]) -> None:

    global chipHasBeenErased

    #remove arguments from list as they are processed.
    #ignore extra arguments; leave them in the list
    port.connect()

    avrLink = port.avrLink
    chip = port.avrLink.chip
    assert chip != None
    assert port.avrLink.chip != None


    cmd = cmdlist.pop(0).lower()
    # ~ debug("="*20,"command:",cmd,"="*20)
    if cmd == "break":
        port.avrLink.BREAK()
    elif cmd == "write":
        destinationOffset=0
        sourceSkip=0
        skipUnchanged=False
        eraseBeforeWrite=True
        sizeLimit=0xffffffff
        verify=True
        while True:
            filename = cmdlist.pop(0)
            if filename == ":o":
                destinationOffset = int(cmdlist.pop(0),0)
            elif filename == ":u":
                skipUnchanged = True
            elif filename == ":e":
                eraseBeforeWrite=False
            elif filename == ":z":
                sizeLimit = int(cmdlist.pop(0),0)
            elif filename == ":s":
                sourceSkip = int(cmdlist.pop(0),0)
            elif filename == ":v":
                verify=False
            else:
                break
        sendFile(port.avrLink,filename=filename,
            #noAck=False, noAckDelay=0,
            skipUnchanged=skipUnchanged,eraseBeforeWrite=eraseBeforeWrite,
            destinationOffset=destinationOffset,sizeLimit=sizeLimit,
            sourceSkip=sourceSkip,
            chipHasBeenErased=chipHasBeenErased,
            verify=verify)
        chipHasBeenErased = False
    elif cmd == "getsib":
        sib = port.avrLink.SIB()
        familyID,nvmVersion0,nvmVersion1,nvmVersion2,ocdVersion0,ocdVersion1,ocdVersion2,dbgOscFreq = struct.unpack_from("7s1x3b3b1x1b", bytes(sib) )
        print("Family:",familyID)
        print("nvmVersion:",hex(nvmVersion0),hex(nvmVersion1),hex(nvmVersion2))
        print("ocdVersion:",hex(ocdVersion0),hex(ocdVersion1),hex(ocdVersion2))
        print("dbgOscFreq:",dbgOscFreq)
    elif cmd == "reset":
        port.avrLink.requestReset()
    elif cmd == "erase":
        eraseFlash(port.avrLink)
        chipHasBeenErased = True
    elif cmd == "geterrorstatus":
        print(bin(getErrorStatus(port.avrLink)))
    elif cmd == "enableprogramming":
        port.avrLink.enableNVMProgramming()
    elif cmd == "getsignature":
        print( [hex(q) for q in getSignature(port.avrLink)] )
    elif cmd == "store":
        offset = int(cmdlist.pop(0),0)
        value = int(cmdlist.pop(0),0)
        storeToFlash(port.avrLink,offset,value)
        if value != 0xff:
            chipHasBeenErased = False
    elif cmd == "beginflashwrite":
        beginFlashWrite(port.avrLink)
    elif cmd == "finishflashwrite":
        chip.finishFlashWrite(port.avrLink)
    elif cmd == "setguardtime":
        g = int(cmdlist.pop(0),0)
        setGuardTime(port.avrLink,g)
    elif cmd == "print":
        start=int(cmdlist.pop(0),0)
        count=int(cmdlist.pop(0),0)
        v = readFlash(port.avrLink,start,count)
        linecount=16
        for i in range(0,len(v),linecount):
            sublist: list[int] = v[i:i+linecount]
            tl: list[str] = [f"{q:02x}" for q in sublist]
            ts = " ".join(tl)
            print(f"{start+i:04x} | {ts}")
    elif cmd == "enabledelay":
        enableInterbyteDelay(port.avrLink)
    elif cmd == "disabledelay":
        disableInterbyteDelay(port.avrLink)
    elif cmd == "read":
        filename = cmdlist.pop(0)
        count = int(cmdlist.pop(0),0)
        offset = int(cmdlist.pop(0),0)
        with open(filename,"wb") as fp:
            recvFile(port.avrLink,count=count,fp=fp,offset=offset)
    elif cmd == "updiclock":
        speed = int(cmdlist.pop(0))
        port.avrLink.setClock(speed)
    # ~ elif cmd == "clock":
        # ~ speed = int(cmdlist.pop(0))
        # ~ chip.setClock(port.avrLink,speed)
    elif cmd == "verify":
        fileSkip = 0
        flashOffset = 0
        while True:
            filename = cmdlist.pop(0)
            if filename == ":s":
                fileSkip = int(cmdlist.pop(0),0)
            elif filename == ":o":
                flashOffset = int(cmdlist.pop(0),0)
            else:
                break

        with open(filename,"rb") as fp:
            fp.seek(0,2)
            fileSize = fp.tell()
            fp.seek(fileSkip)
            verifySize = fileSize - fileSkip
            numOK=0
            numLeft = verifySize
            ok=True
            while numLeft > 0:
                inFlash = streamReadOneBlock(port.avrLink,numLeft,flashOffset)
                if len(inFlash) == 0:
                    error("Error: No data returned from flash read")
                inFile = readCompletely( fp, len(inFlash), notPast=0xffffffff )

                #we limited numLeft to file size, so we should always get
                #back len(inFlash) bytes
                assert len(inFile) == len(inFlash)

                if inFile != inFlash:
                    print("Flash does not match file",filename)
                    for i in range(len(inFlash)):
                        if inFile[i] != inFlash[i]:
                            ok=False
                            print(f"Mismatch in flash at address {flashOffset+i} = 0x{flashOffset+i:04x}")
                            print(f"Expected: {inFile[i]:3d} = 0x{inFile[i]:02x} = 0b{inFile[i]:08b}")
                            print(f"In flash: {inFlash[i]:3d} = 0x{inFlash[i]:02x} = 0b{inFlash[i]:08b}")


                            # ~ print(f"NVM STATUS: {chip.getNVMControllerStatus(avrLink):08b}")
                            # ~ chip.writeCommandToNVMController(avrLink,0x00)
                            # ~ chip.writeCommandToNVMController(avrLink,0x01)
                            # ~ chip.writeCommandToNVMController(avrLink,0x00)
                            # ~ print(f"NVM STATUS: {chip.getNVMControllerStatus(avrLink):08b}")

                            numLeft=0
                            break
                else:
                    numLeft -= len(inFlash)
                    flashOffset += len(inFlash)
                    numOK += len(inFlash)
                    print( f"{round( 100 * (numOK / verifySize))}% ({numOK} of {verifySize} bytes) OK")
            if ok:
                print("Flash matches file",filename)
    else:
        raise ValueError(f"Bad command: {cmd}")


commandHelpText="""
Commands (all are case insensitive):

    disconnect          Disconnect from UPDI
    erase               Erase all flash & EEPROM
    print s N           Print N bytes of flash starting at address s
    read F N A          Read N bytes of flash starting at offset A and
                        save to file F
    reset               Request AVR reset
    write [args] F      Write file F to flash. Optional arguments:
                          :e        Don't erase flash before writing
                          :o OFFS   Destination address (offset)
                          :s SKIP   Discard SKIP bytes from start of file
                          :u        Don't write unchanged parts of the file
                          :v        Don't verify data page-by-page
                          :z SIZE   Write max of SIZE bytes from file
                        If :e is given, only the pages being written
                        are erased. If :e is omitted, no flash is erased.
                        If the erase command was given before
                        the write command, the chip is only erased once.
                        If :v is given, it is recommended to use the verify
                        command to verify the data after writing.
    verify [args] F     Verify flash matches file F. Optional arguments:
                          :f OFFS   Offset in flash
                          :s NUM    Skip first NUM bytes in file
"""

debugcommands = """
The following are lower-level commands useful for testing and debugging.

    beginflashwrite     Begin flash write operation
    break               Send break
    clock N             Set target CPU clock to N (legal values
                        depend on the chip)
    disabledelay        Disable interbyte delay
    enabledelay         Enable interbyte delay
    enableprogramming   Enable flash programming
    finishflashwrite    End flash write operation
    getsib              Get SIB signature
    geterrorstatus      Get error status from target
    getsignature        Get target device signature
    setguardtime N      Set UPDI guard time
    stats               Show raw number of bytes sent and received
    store A V           Store value V to flash at offset A. Does not
                        erase before writing.
    updiclock N         Set UPDI clock speed in MHz (N=4, 8, 16, 32)

"""


def main() -> None:

    parser = argparse.ArgumentParser(
        epilog=commandHelpText,
        formatter_class=argparse.RawDescriptionHelpFormatter
    )
    parser.add_argument("--list",action="store_true",help="List detected serial ports")
    parser.add_argument("--port",help="Name of serial port to use. Ex: COM3 or /dev/ttyACM0")
    parser.add_argument("--no-enable-programming", action="store_true",help="Don't enable programming mode upon UPDI connection")
    parser.add_argument("--no-disconnect", action="store_true",help="Don't reset the AVR and disconnect when done")
    c = ", ".join(chips.getChips())
    parser.add_argument("--chip", required=False,help=f"Chip model: One of: {c}")
    parser.add_argument("commands",action="store",nargs="*",help="Commands to execute")
    parser.add_argument("--help-debug",action="store_true",help=f"Show information about debugging and lower-level commands")
    parser.add_argument("--updispeed",action="store",help="Set bit rate for UPDI communication."
        "Value must be one of: 300, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 64000, 75000, 115200, 230400, "
        "460800, 500000, 750000, 921600")
    parser.add_argument("--timeout",action="store",help="Set timeout for communications in seconds. This affects both "
        "USB communication and UPDI communication. Must be > 0.001 and < 65000 ")

    args = parser.parse_args()

    if args.list:
        SerialPort.listPorts()
        return

    if args.chip:
        if not chips.isChipNameKnown(args.chip):
            print(f"No such chip '{args.chip}'")
            print("Choose one of:", ", ".join(chips.getChips()))
            sys.exit(1)
        else:
            chip = chips.make(args.chip)
    else:
        chip = None

    if not args.timeout:
        timeout = 5.0
    else:
        timeout = float(args.timeout)
        if timeout < 0.001 or timeout > 65000:
            error("Bad timeout")
            sys.exit(1)

    #conservative speed; can increase with command line options
    updiSpeed = 38400
    if args.updispeed:
        updiSpeed = int(args.updispeed)

    if updiSpeed not in [300, 1200, 2400, 4800, 9600, 14400, 19200, 38400, 57600, 64000, 75000, 115200, 230400, 460800, 500000, 750000, 921600]:
        error("Bad value for --updispeed")
        sys.exit(1)

    port = SerialPort(chip, timeout, updiSpeed)

    if args.no_enable_programming:
        port.avrLink.enableProgrammingOnConnect = False

    if args.port:
        port.setPortName(args.port)

    if args.help_debug:
        print(debugcommands)
        return

    port.connect()

    backendVersion = port.avrLink.GET_VERSION()
    if backendVersion != EXPECTED_BACKEND_VERSION:
        error(f"Backend version mismatch: Expected {EXPECTED_BACKEND_VERSION:08x} but got {backendVersion:08x}")

    port.avrLink.setUpdiTimeout(0.95 * timeout)
    port.avrLink.connectToUPDI()

    try:
        i=0
        commands = args.commands[:]
        while len(commands):
            process_command(port,commands)
        sys.exit(0)
    finally:
        #profiling.report()

        # ~ print("DO RESET")
        if 1:
            if not args.no_disconnect:
                debug("ONEXIT: disconnect")
                try:
                    port.avrLink.requestReset()
                    port.avrLink.disconnect()
                except Timeout:
                    print("Warning: Could not disconnect on exit")


#import cProfile
#cProfile.run("main()")
main()
