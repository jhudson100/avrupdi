# import time
# import chips
# from typing import Any,Callable
# from typing import NamedTuple
# from log import debug,error
# import typing
# from UpdiLink import UpdiLink
# from utils import waitUntilTrue
# import AVR32DD20

# class AVR64DB32(AVR32DD20.AVR32DD20):

     # # ~ MAX_NVM_ADDRESS = 0xffff

    # def __init__(self, name: str) -> None:
        # super().__init__(name)





    # # ~ def beginFlashPage(self, avrLink: "UpdiLink", eraseAndWrite: bool, offset:int) -> None :
        # # ~ debug("AVR64DB32: beginFlashPage for offset",hex(offset))
        # # ~ #the avr32dd does not have a page buffer, so we need not clear it
        # # ~ #however, there is no "erase and write" command, so if we want to erase
        # # ~ #the page, we must do that now as a separate command
        # # ~ self.writeCommandToNVMController( avrLink, self.NVM_NOCMD )

        # # ~ #if we're erasing, we have to start on a page boundary.
        # # ~ #if we're only writing, we can start anywhere
        # # ~ if eraseAndWrite:
            # # ~ assert offset % self.FLASH_PAGE_SIZE == 0
        # # ~ assert offset >= 0
        # # ~ assert offset < 65536
        # # ~ self.mapFlashRange(avrLink, offset >> 15 )
        # # ~ offset %= 32768
        # # ~ super().beginFlashPage(avrLink,eraseAndWrite,offset)

    # # ~ def mapFlashAddress( self, avrLink:"UpdiLink", addr:int) -> int:
        # # ~ #addr is a flash address (0...64KB)
        # # ~ #returns 0x8000-based address for reading
        # # ~ #and maps the section if needed
        # # ~ assert addr >= 0
        # # ~ section = addr >> 15
        # # ~ if self.mappedSection != section:
            # # ~ self._mapFlashRange(avrLink, section )
            # # ~ self.mappedSection = section
        # # ~ offsetWithinSection = addr & 0x7fff
        # # ~ a = self.FLASH_START + offsetWithinSection
        # # ~ assert a >= 0
        # # ~ assert a <= 0xffff
        # # ~ return a

    # # ~ def getAddressToWrite(self, addr:int) -> int:
        # # ~ #addr is a flash address (0...64KB)
        # # ~ #returns 0x8000-based address for reading
        # # ~ section = addr >> 15
        # # ~ assert self.mappedSection == section
        # # ~ offsetWithinSection = addr & 0x7fff
        # # ~ return self.FLASH_START + offsetWithinSection
# # ~ def setClock(self, avrLink:"UpdiLink", clock: int) -> None:
        # # ~ D = { 1: 0, 2: 1, 3: 2, 4: 3, 8: 5, 12: 6, 16: 7, 20: 8, 24: 9}
        # # ~ if clock not in D:
            # # ~ tmp = sorted(list(D.keys()))
            # # ~ tmp2 = ", ".join([str(q) for q in tmp])
            # # ~ error("Invalid clock speed; must be one of", tmp2)
        # # ~ freq = D[clock]
        # # ~ CLKBASE = 0x60
        # # ~ v = avrLink.LDS(addr=CLKBASE + 0x08 )
        # # ~ v &= 0b11000011
        # # ~ v |= (freq<<2)
        # # ~ avrLink.STS(addr=self.CCP_ADDR, value=self.ALLOW_IOREG )
        # # ~ avrLink.STS(addr=CLKBASE + 0x08, value=v)

    # # ~ def beginFlashPageNoMap(self, avrLink: "UpdiLink", eraseAndWrite: bool, offset:int) -> None :
        # # ~ #don't change the memory map and don't reduce offset modulo 32K
        # # ~ #useful if the caller has already handled those tasks.
        # # ~ super().beginFlashPage(avrLink,eraseAndWrite,offset)
