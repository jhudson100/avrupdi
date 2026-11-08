import time
import chips
from typing import Any,Callable
from typing import NamedTuple
from log import debug,debugRep,error
import typing
from UpdiLink import UpdiLink
from utils import waitUntilTrue

class AVRxxDyzz(chips.Chip):

    NVM_FLASH_WRITE = 0x02
    MAX_NVM_ADDRESS = 0xffff   #max address expected in NVMCTRL STATUS register

    def __init__(self, name: str, FLASH_SIZE: int) -> None:
        super().__init__(
            name, FLASH_SIZE=FLASH_SIZE, NVMCTRL_STATUS_OFFSET = 0x02,
            FLASH_PAGE_SIZE=512
        )

    def clearPageBuffer(self,avrLink:"UpdiLink") -> None:
        debug("clearPageBuffer")
        #the avr32dd20 does not have a page buffer
        self.writeCommandToNVMController( avrLink, self.NVM_NOCMD )

    def beginFlashWrite(self,avrLink: "UpdiLink") -> None:
        #ensure no ongoing write operations
        debug("beginFlashWrite")
        waitUntilTrue(lambda: False == self.getNVMControllerBusy(avrLink) )
        self.writeCommandToNVMController( avrLink, self.NVM_NOCMD )

    def beginFlashPage(self, avrLink: "UpdiLink", eraseAndWrite: bool, offset:int) -> None :
        debug("AVR32DD20: beginFlashPage at offset",offset)
        #the avr32dd does not have a page buffer, so we need not clear it
        #however, there is no "erase and write" command, so if we want to erase
        #the page, we must do that now as a separate command
        self.writeCommandToNVMController( avrLink, self.NVM_NOCMD )

        assert offset >= 0
        assert offset < self.FLASH_SIZE

        if eraseAndWrite:
            #if we're erasing, we have to start on a page boundary.
            #if we're only writing, we can start anywhere
            assert offset % self.FLASH_PAGE_SIZE == 0

            #The memory map has a 32KB window into flash.
            #If the device has >32KB of flash,
            #we must map the correct flash area into the window.
            #If the device has <=32KB of RAM, this is a no-op
            addr = self.mapFlashAddress(avrLink,offset)

            self.writeCommandToNVMController(avrLink,self.NVM_PAGE_ERASE)
            avrLink.STS(addr=addr, value=0x00 )
            waitUntilTrue(lambda: False == self.getNVMControllerBusy(avrLink) )
            self.writeCommandToNVMController( avrLink, self.NVM_NOCMD )

        self.writeCommandToNVMController(avrLink,self.NVM_FLASH_WRITE)

        return

    def endFlashPage(self, avrLink: "UpdiLink", eraseAndWrite:bool) -> None:
        debug("endFlashPage")
        # ~ debug("endFlashPage: addr=",hex(self.getNVMAddr(avrLink)))

        #nothing to do for eraseAndWrite

        self.writeCommandToNVMController( avrLink, self.NVM_NOCMD )

        #4ms for write, 6ms for erase+write
        time.sleep(0.006)
        ok = waitUntilTrue(lambda: False == self.getNVMControllerBusy(avrLink), maxTime=3.0 )
        if not ok:
            status = self.getNVMControllerStatus(avrLink)
            error(f"NVM controller always busy? {status:08b}")


    def finishFlashWrite(self,avrLink: "UpdiLink") -> None:
        debug("finishFlashWrite")
        waitUntilTrue(lambda: False == self.getNVMControllerBusy(avrLink) )
        #write nocmd to complete operation
        self.writeCommandToNVMController(avrLink,self.NVM_NOCMD)
        #this probably isn't needed
        waitUntilTrue(lambda: not self.getNVMControllerBusy(avrLink) )


chips.registerChip("avr16dd14",   0x1100, ( 0x1e, 0x95, 0x34 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=16)  )
chips.registerChip("avr16dd20",   0x1100, ( 0x1e, 0x95, 0x33 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=16)  )
chips.registerChip("avr16dd28",   0x1100, ( 0x1e, 0x95, 0x32 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=16)  )
chips.registerChip("avr16dd32",   0x1100, ( 0x1e, 0x95, 0x31 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=16)  )
chips.registerChip("avr32dd14",   0x1100, ( 0x1e, 0x95, 0x3b ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr32dd20",   0x1100, ( 0x1e, 0x95, 0x3a ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr32dd28",   0x1100, ( 0x1e, 0x95, 0x39 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr32dd32",   0x1100, ( 0x1e, 0x95, 0x38 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr32db28",   0x1100, ( 0x1e, 0x95, 0x37 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr32db32",   0x1100, ( 0x1e, 0x95, 0x36 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr32db48",   0x1100, ( 0x1e, 0x95, 0x35 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=32)  )
chips.registerChip("avr64db28",   0x1100, ( 0x1e, 0x96, 0x19 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=64)  )
chips.registerChip("avr64db32",   0x1100, ( 0x1e, 0x96, 0x18 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=64)  )
chips.registerChip("avr64db48",   0x1100, ( 0x1e, 0x96, 0x17 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=64)  )
chips.registerChip("avr64db64",   0x1100, ( 0x1e, 0x96, 0x16 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=64)  )
chips.registerChip("avr128da28",  0x1100, ( 0x1e, 0x97, 0x0a ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da32",  0x1100, ( 0x1e, 0x97, 0x09 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da48",  0x1100, ( 0x1e, 0x97, 0x08 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da64",  0x1100, ( 0x1e, 0x97, 0x07 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da28s", 0x1100, ( 0x1e, 0x97, 0x12 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da32s", 0x1100, ( 0x1e, 0x97, 0x11 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da48s", 0x1100, ( 0x1e, 0x97, 0x10 ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
chips.registerChip("avr128da64s", 0x1100, ( 0x1e, 0x97, 0x0f ), lambda name: AVRxxDyzz(name, FLASH_SIZE=128)  )
