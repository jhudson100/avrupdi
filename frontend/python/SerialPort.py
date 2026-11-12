
import serial                       #type: ignore
import serial.tools.list_ports      #type: ignore
import argparse
import sys
import time
import struct
import io
from typing import Any,Callable
from typing import NamedTuple
from log import debug
import typing
# ~ if typing.TYPE_CHECKING:
    # ~ from ATmega4808 import ATmega4808
from UpdiLink import UpdiLink
from Exceptions import ProtocolError,Timeout
from chips import Chip


#dump all data to and from serial port
DEBUG=False

class SerialPort:
    def __init__(self,chip:Chip|None,timeout:float,updiSpeed:int) -> None:
        self.portname:str|None = None
        self.port: Any|None = None
        self.avrLink = UpdiLink(self,chip,updiSpeed)
        self.timeout = timeout

    def setPortName(self,name:str) -> None:
        """Set serial port name. It is an error
           if we're currently connected."""
        if self.avrLink.connected:
            raise ProtocolError("Cannot set port name while connected to Pico")
        self.portname = name

    def connect(self) -> None:

        if self.port != None:
            return

        if not self.portname:
            self.portname = SerialPort.scanForPort()

        if not self.portname:
            raise ProtocolError("No serial port specified, and could not find one with scanning")

        self.port = serial.Serial(self.portname,115200,timeout=self.timeout)
        self.port.reset_input_buffer()
        self.port.reset_output_buffer()

    def discardInput(self) -> None:
        if self.port:
            self.port.reset_input_buffer()

    def disconnect(self) -> None:
        if self.port:
            self.avrLink.disconnect()
            self.port.close()
            self.port=None

    def write(self, data:str|bytes|bytearray) -> None:
        if not self.port:
            raise ProtocolError("Not connected")

        if DEBUG:
            print("SERIALWRITE:",repr(data))

        if type(data) == str:
            data = data.encode()
        self.port.write(data)
        self.port.flush()

    def read(self,count:int) -> bytes:
        if not self.port:
            raise ProtocolError("Not connected")
        lst=[]
        for i in range(count):
            r = self.port.read(1)

            if DEBUG:
                print("SERIALREAD:",r)

            if len(r) == 0:
                raise Timeout("Timeout when reading from serial port")
            lst.append(r)
        return b"".join(lst)

    def close(self) -> None:
        if not self.port:
            return
        self.disconnect()
        self.port.close()
        self.port = None


    @staticmethod
    def listPorts() -> None:
        ports = serial.tools.list_ports.comports()
        for port in ports:
            print(port.device, f"{port.manufacturer}" if port.manufacturer else "")

    @staticmethod
    def scanForPort() -> None|str:
        ports = serial.tools.list_ports.comports()
        matches={}
        for port in ports:
            matches[port.device] = 0
            if "microchip technology" in str(port.manufacturer).lower():
                matches[port.device] += 1
            if str(port.hwid).lower() == "04d8:00dd":
                matches[port.device] += 1
            if "usb serial" in str(port).lower():
                matches[port.device] += 1

        tmp = list(matches.items())
        tmp.sort(key=lambda x: x[1])

        if tmp[-1][1] > 0:
            return port.device   #type:ignore
        return None
