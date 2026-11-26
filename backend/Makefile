#ref: https://ww1.microchip.com/downloads/aemDocuments/documents/MCU08/ApplicationNotes/ApplicationNotes/Basic-Bootloader-for-AVR-MCU-DA-Family-DS00003341C.pdf

MCU=avr32dd14
CC=avr-gcc -Wall -g -O2 -mmcu=$(MCU)
OBJCOPY=avr-objcopy -O binary -R .eeprom --gap-fill 0xff

BOOTLOADER_PAGES=2

.PHONY: dist

all: backend.bin

dist: backend.bin
	-/bin/cp backend.bin ..

backend.bin: bootloader.bin main.bin
	@#if this fails, BOOTLOADER_PAGES is too small
	python -c "import os.path,sys;sys.exit(0 if os.path.getsize('bootloader.bin') < $(BOOTLOADER_PAGES)*512 else 1)"
	cat bootloader.bin main.bin > backend.bin

%.bin: %.elf
	$(OBJCOPY) $< $@

bootloader.elf: bootloader.o
	$(CC) $^ -o $@ -Wl,-Map -Wl,bootloader.map

bootloader.o: bootloader.c
	$(CC) -DBOOTLOADER_PAGES=$(BOOTLOADER_PAGES) -c $<

main.elf: report.o serial.o updi.o main.o util.o scratchspace.o Makefile
	$(CC) $^ -o $@ -Wl,-Map -Wl,backend.map -Wl,--section-start=.text=$$(( $(BOOTLOADER_PAGES) * 512 ))
	avr-strip $@
	python getsize.py $@

main.o: main.c $(wildcard *.h)
	$(CC) -c $<

report.o: report.c $(wildcard *.h)
	$(CC) -c $<

serial.o: serial.c $(wildcard *.h)
	$(CC) -c $<

updi.o: updi.c $(wildcard *.h)
	$(CC) -c $<

util.o: util.c $(wildcard *.h)
	$(CC) -c $<

scratchspace.o: scratchspace.c
	$(CC) -c $<

clean:
	-/bin/rm $(wildcard *.o) $(wildcard *.elf) $(wildcard *.bin) $(wildcard *.map)

flash: backend.bin
	python picoupdi.py --port /dev/ttyACM1 --gpio 15 --speed 115200 --chip $(MCU) write backend.bin
	python picoupdi.py --port /dev/ttyACM1 --gpio 15 --speed 115200 --chip $(MCU) writefuse 8 $(BOOTLOADER_PAGES)

reset: backend.bin
	python picoupdi.py --port /dev/ttyACM1 --gpio 15 --speed 115200 --chip $(MCU) getsib

svn: svnadd
	-svn commit -m ""

svnadd:
	-(cd frontend && $(MAKE) svnadd)
	-svn add *.c *.h *.py *.txt
