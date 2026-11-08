import time
import chips
from typing import Any,Callable
from typing import NamedTuple
from log import debug,error
import typing
from UpdiLink import UpdiLink
from utils import waitUntilTrue
import profiling

class AVR16EBxx(chips.Chip):


    #NVM commands: Note that these differ from the DA/DB/DD series
    NVM_PAGE_ERASE_AND_WRITE = 0x05 #erase and write
    NVM_PAGE_WRITE = 0x04           #write but don't erase
    NVM_CLEAR_PAGE_BUFFER = 0x0f

    #ref: Memories chapter tells start for eeprom, etc.

    def __init__(self, name:str) -> None:
        super().__init__(name,FLASH_SIZE=16, FLASH_PAGE_SIZE = 64 , NVMCTRL_STATUS_OFFSET = 0x06)
        self.pageBufferIsClear=False


    def beginFlashWrite(self,updiLink: "UpdiLink") -> None:
        #ensure no ongoing write operations
        st=time.time_ns()
        debug("beginFlashWrite")
        waitUntilTrue(lambda: False == self.getNVMControllerBusy(updiLink) )
        self.writeCommandToNVMController( updiLink, self.NVM_NOCMD )
        et=time.time_ns()
        profiling.count("beginFlashWrite",et-st)

    def clearPageBuffer(self,updiLink:"UpdiLink") -> None:
        start = time.time_ns()

        if not self.pageBufferIsClear:
            debug("CLEAR PAGE BUFFER")
            self.checkNVMError(updiLink)
            self.writeCommandToNVMController( updiLink, self.NVM_CLEAR_PAGE_BUFFER )
            #datasheet does not specify how long this takes
            #add a read-status to ensure it has completed
            waitUntilTrue(lambda: not self.getNVMControllerBusy(updiLink) )
            self.writeCommandToNVMController( updiLink, self.NVM_NOCMD )

        end = time.time_ns()
        profiling.count("clearPageBuffer",end-start)
        self.pageBufferIsClear=True

    def beginFlashPage(self, updiLink: "UpdiLink", eraseAndWrite: bool, offset:int) -> None :
        start = time.time_ns()
        debug("beginFlashPage")
        self.clearPageBuffer(updiLink)
        #we don't use eraseAndWrite here; that decision is made
        #when the page has been ended
        end = time.time_ns()
        profiling.count("beginFlashPage",end-start)
        pageBufferIsClear=False

    def endFlashPage(self, updiLink: "UpdiLink", eraseAndWrite:bool) -> None:
        start = time.time_ns()

        debug("endFlashPage")

        #Using STS_W:Wrote 4096 bytes in 6.63 seconds = 4946 bits per second = 618 bytes per second
        #Not using STS_W: Wrote 4096 bytes in 6.70 seconds = 4891 bits per second = 611 bytes per second

        if 0:
            updiLink.STS_W(
                [
                    (self.CCP_ADDR,self.ALLOW_SPM),
                    (self.NVMCTRL_CONTROLA, self.NVM_PAGE_ERASE_AND_WRITE)
                ],
                self.NVMCTRL_STATUS,
                self.NVMCTRL_STATUS_BUSY_MASK
            )
            profiling.count("endFlashPage",time.time_ns()-start)
            return
        else:
            st=time.time_ns()
            if eraseAndWrite:
                #erase and write page
                self.writeCommandToNVMController(updiLink,self.NVM_PAGE_ERASE_AND_WRITE)
            else:
                #just write page; no erase
                self.writeCommandToNVMController(updiLink,self.NVM_PAGE_WRITE)
            et=time.time_ns()
            profiling.count("endFlashPage:writeCommandToNVMController",et-st)

            self.pageBufferIsClear=True

            #datasheet says 4ms for write, 6ms for erase+write
            time.sleep(0.006)

            st=time.time_ns()
            waitUntilTrue(lambda: False == self.getNVMControllerBusy(updiLink) )
            et=time.time_ns()
            profiling.count("endFlashPage:getNVMControllerBusy",et-st)

            end = time.time_ns()
            profiling.count("endFlashPage",end-start)

    def storeToFlash(self,updiLink: "UpdiLink", offset:int, value:int) -> None:
        #write value to store in flash
        start = time.time_ns()
        debug("store to flash at",hex(offset))
        updiLink.STS(self.FLASH_START+offset,value)
        end = time.time_ns()
        self.pageBufferIsClear=False
        profiling.count("storeToFlash",end-start)


    def finishFlashWrite(self,updiLink: "UpdiLink") -> None:
        debug("finishFlashWrite")
        st=time.time_ns()
        waitUntilTrue(lambda: False == self.getNVMControllerBusy(updiLink) )
        #write nocmd to complete operation
        self.writeCommandToNVMController(updiLink,self.NVM_NOCMD)
        #this probably isn't needed
        waitUntilTrue(lambda: not self.getNVMControllerBusy(updiLink) )
        et=time.time_ns()
        profiling.count("finishFlashWrite",et-st)


#all EB16 chips use the same protocol
chips.registerChip("avr16eb14", 0x1080, (0x1e, 0x94, 0x49) , lambda name: AVR16EBxx(name)  )
chips.registerChip("avr16eb20", 0x1080, (0x1e, 0x94, 0x40) , lambda name: AVR16EBxx(name)  )
chips.registerChip("avr16eb28", 0x1080, (0x1e, 0x94, 0x3f) , lambda name: AVR16EBxx(name)  )
chips.registerChip("avr16eb32", 0x1080, (0x1e, 0x94, 0x3e) , lambda name: AVR16EBxx(name)  )



    # ~ def mapFlashAddress( self, updiLink:"UpdiLink", addr:int) -> int:
        # ~ #addr = flash-based address (0...32KB)
        # ~ #returns address of mapped flash data (0x8000 based)
        # ~ return self.FLASH_START + addr

    # ~ def getNVMAddr(self, updiLink: "UpdiLink") -> int:
        # ~ addr=0
        # ~ shift=0
        # ~ for i in range(self.NVMCTRL_ADDRESS_COUNT):
            # ~ tmp = updiLink.LDS(addr=self.NVMCTRL_ADDRESS + i)
            # ~ tmp <<= shift
            # ~ addr |= tmp
            # ~ shift +=8
        # ~ return addr
