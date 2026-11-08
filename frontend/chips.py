import sys
from abc import abstractmethod
import typing
from typing import Callable
if typing.TYPE_CHECKING:
    from UpdiLink import UpdiLink
from log import info
import time
from log import debug, error
from utils import waitUntilTrue

import profiling
debugTimes = profiling.debugTimes
debugCounts = profiling.debugCounts

class Chip:

    #these values are common between many chip models
    #but they can be overridden in descendant classes
    def __init__(self, name:str, *,
            FLASH_SIZE: int,            #number of KB: different for various models
            FLASH_PAGE_SIZE: int,       #different for various models
            NVMCTRL_STATUS_OFFSET:int,         #address of nvm controller's status register: differs between Ex and Dx
            ) -> None:
        self.name=name

        #ref: Peripheral Module Address Map tells address of peripherals
        #Many of these are common to all AVR's, but we make them variables
        #in case we need to change them for some models someday.
        self.CPU_START = 0x0030
        self.CCP_ADDR = self.CPU_START+0x04
        self.ALLOW_SPM = 0x9d
        self.NVMCTRL_START = 0x1000
        self.NVM_NOCMD=0x00
        self.NVM_PAGE_ERASE = 0x08
        self.NVM_CHIP_ERASE = 0x20           #whole chip erase
        self.FLASH_SIZE = FLASH_SIZE * 1024
        self.FLASH_PAGE_SIZE = FLASH_PAGE_SIZE
        self.EEPROM_START = 0x1400
        self.FUSE_START = 0x1050
        self.FLASH_START = 0x8000        #up to 32KB flash can be mapped at once
        self.NVMCTRL_CONTROLA = self.NVMCTRL_START + 0x00
        self.NVMCTRL_STATUS = self.NVMCTRL_START + NVMCTRL_STATUS_OFFSET
        self.NVMCTRL_STATUS_ERROR_MASK = 0b01110000
        self.NVMCTRL_STATUS_BUSY_MASK = 0b11
        self.ALLOW_IOREG = 0xd8     #CCP for ioreg access
        self.NVMCTRL_CONTROLB = self.NVMCTRL_START + 0x01   #everything but flashmap is under ccp

        self.mappedSection = -1     #which 32KB chunk of flash has been mapped to the 0x8000 mark


    def mapFlashAddress( self, avrLink:"UpdiLink", addr:int) -> int:
        #addr is a flash address (0...64KB)
        #maps the section if needed and
        #returns 0x8000-based address for reading/writing
        assert addr >= 0
        assert addr < self.FLASH_SIZE
        section = addr >> 15
        if self.mappedSection != section:
            self._mapFlashRange(avrLink, section )
            self.mappedSection = section
        offsetWithinSection = addr & 0x7fff
        a = self.FLASH_START + offsetWithinSection
        assert a >= 0
        assert a <= 0xffff
        return a


    def verifySignature(self, avrLink:"UpdiLink") -> bool:
        for name,addr,sig in sigs:
            if name == self.name:
                L=[]
                for i in range(3):
                    L.append(avrLink.LDS(addr=addr+i))
                rsig = bytes(L)
                if rsig == sig:
                    return True
                else:
                    print(
                        f"Expected to find {name} but signature does not match:\n"
                        f"Got      {[hex(q) for q in rsig]}\n"
                        f"Expected {[hex(q) for q in sig]}"
                    )
                    return False

        print(f"No signature for {name}")
        return False


    @staticmethod
    def autodetectChip(avrLink: "UpdiLink") -> Chip:
        alreadyRead={}
        matches=[]
        for name,addr,sig in sigs:
            if addr not in alreadyRead:
                L=[]
                for i in range(3):
                    L.append(avrLink.LDS(addr=addr+i))
                alreadyRead[ addr ] = bytes(L)
            if sig == alreadyRead[addr]:
                matches.append(name)
        if len(matches) == 0:
            raise RuntimeError("Could not autodetect chip")
        if len(matches) == 1:
            info("Autodetected",matches[0])
            return make(matches[0])
        else:
            raise RuntimeError("Autodetected several possible chips:",
                ",".join(matches))

    def getFlashPageSize(self):
        return self.FLASH_PAGE_SIZE

    def getFlashStart(self):
        return self.FLASH_START

    def writeCommandToNVMController(self,updiLink: "UpdiLink", cmd: int) -> None:
        start = time.time_ns()
        debug("writeCommandToNVMController:",hex(cmd))

        s1 = time.time_ns()
        #write key to configuration change protection register (CPU_START + offset 0x04)
        #page 40: Key for I/O registers is 0xd8. key for SPM is 0x9d.
        updiLink.STS_M(
            (self.CCP_ADDR,self.ALLOW_SPM),
            (self.NVMCTRL_CONTROLA, cmd)
        )

        return

    def getNVMControllerStatus(self, avrLink: "UpdiLink") -> int:
        reg = avrLink.LDS(self.NVMCTRL_STATUS)
        return reg


        # ~ debug("Set CCP")
        # ~ updiLink.STS(addr=self.CCP_ADDR, value=self.ALLOW_SPM )
        # ~ e1 = time.time_ns()
        # ~ debugTimes["STS CCP"] += e1-s1

        # ~ s1 = time.time_ns()
        # ~ debug("Send nvm command",hex(cmd),"to CONTROLA")
        # ~ updiLink.STS(addr=self.NVMCTRL_CONTROLA, value=cmd)
        # ~ e1 = time.time_ns()
        # ~ debugTimes["STS CONTROLA"] += e1-s1

        # ~ #s1 = time.time_ns()
        # ~ #self.checkNVMError(updiLink)
        # ~ #e1 = time.time_ns()
        # ~ #debugTimes["checkNVMError"] += e1-s1

        # ~ end = time.time_ns()
        # ~ debugTimes["writeCommandToNVMController"] += end-start

    def eraseFlash(self,updiLink: "UpdiLink") -> None:
        self.writeCommandToNVMController( updiLink, self.NVM_NOCMD )
        self.writeCommandToNVMController( updiLink, self.NVM_CHIP_ERASE)
        waitUntilTrue( lambda: False == self.getNVMControllerBusy(updiLink) )
        self.writeCommandToNVMController( updiLink, self.NVM_NOCMD )
        waitUntilTrue(lambda: False == self.getNVMControllerBusy(updiLink) )

    def checkNVMError(self, avrLink: "UpdiLink") -> None:
        debug("checkNVMError")
        assert self.NVMCTRL_STATUS != None
        reg = avrLink.LDS(self.NVMCTRL_STATUS)
        e = reg & self.NVMCTRL_STATUS_ERROR_MASK
        debug("NVMError reports:",bin(reg))
        if e :
            error(f"NVM error: 0b{e:b}")

    def getNVMControllerBusy(self, updiLink: "UpdiLink") -> bool:
        st = time.time_ns()
        reg = updiLink.LDS(self.NVMCTRL_STATUS)
        e = reg  & self.NVMCTRL_STATUS_ERROR_MASK
        if e :
            error(f"NVM error: 0b{e:b}")
        profiling.count("getNVMControllerBusy",time.time_ns()-st)
        return (reg & self.NVMCTRL_STATUS_BUSY_MASK) != 0

    def storeToFlash(self,avrLink: "UpdiLink", offset:int, value:int) -> None:
        """Store the given value at the specified offset. Note that offset
            is  given as the offset from the start of flash, not as an offset
            from the start of the 32KB memory-mapped window of flash.
            beginFlashPage() should have been called previously to ensure
            the correct 32KB chunk of flash has been mapped.
            It is only valid to store to the page that was passed
            to beginFlashPage()."""
        #write value to store in flash
        debug("store to flash at",hex(offset))
        offset &= 0x7fff        #account for 32KB window
        avrLink.STS(self.FLASH_START+offset,value)

    #if section is 0: Map 0...32KB-1 to FLASH_START...FLASH_START+32K
    #if section is 1: Map 32KB...64KB-1 to FLASH_START...FLASH_START+32K
    #etc.
    def _mapFlashRange(self,avrLink:"UpdiLink", section:int) -> None:
        maxSection = self.FLASH_SIZE // 32768
        if maxSection == 0:
            maxSection = 1
        assert section < maxSection, f"FALSE: {section} < {maxSection}"
        assert section >= 0

        v = avrLink.LDS(addr=self.NVMCTRL_CONTROLB)
        v2 = (v & 0b11001111) | (section << 4 )
        avrLink.STS(addr=self.NVMCTRL_CONTROLB, value=v2)


    # ~ @abstractmethod
    # ~ def getNVMAddr(self, avrLink:"UpdiLink") -> int:
        # ~ raise NotImplementedError()

    # ~ @abstractmethod
    # ~ def getFlashStart(self) -> int:
        # ~ raise NotImplementedError()

    @abstractmethod
    def clearPageBuffer(self,avrLink:"UpdiLink") -> None:
        raise NotImplementedError()

    # ~ @abstractmethod
    # ~ def getFlashPageSize(self) -> int:
        # ~ raise NotImplementedError()

    @abstractmethod
    def beginFlashWrite(self,avrLink: "UpdiLink") -> None:
        raise NotImplementedError()

    @abstractmethod
    def beginFlashPage(self, avrLink: "UpdiLink", eraseAndWrite: bool, offset:int) -> None :
        raise NotImplementedError()

    @abstractmethod
    def endFlashPage(self, avrLink: "UpdiLink", eraseAndWrite:bool) -> None:
        raise NotImplementedError()

    @abstractmethod
    def finishFlashWrite(self,avrLink: "UpdiLink") -> None:
        raise NotImplementedError()



chips: dict[str,Callable[ [str], Chip] ]={}
sigs: list[ tuple[str,int,bytes] ] = []

def registerChip( name:str, sigaddr, sig: tuple[int,int,int] , maker: Callable[ [str], Chip]) -> None :
    assert name not in chips
    chips[name] = maker
    sigs.append( (name,sigaddr,bytes(sig)) )

def isChipNameKnown(name:str):
    return name in chips

def make(name:str) -> Chip:
    if name not in chips:
        raise RuntimeError(f"No such chip {name}")
    return chips[name](name)

def getChips() -> list[str]:
    return sorted(list(chips.keys()))
