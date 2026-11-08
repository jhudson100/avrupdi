MCU=avr32dd14
CC=avr-gcc -Wall -O2 -mmcu=$(MCU)

all: backend.bin

backend.bin: backend.elf
	avr-objcopy -O binary -R .eeprom --gap-fill 0xff backend.elf backend.bin

backend.elf: report.o serial.o updi.o backend.o util.o
	$(CC) $^ -o $@ -Wl,-Map -Wl,linker.map
	avr-strip $@
	python getsize.py $@

backend.o: backend.c $(wildcard *.h)
	$(CC) -c $<

report.o: report.c $(wildcard *.h)
	$(CC) -c $<

serial.o: serial.c $(wildcard *.h)
	$(CC) -c $<

updi.o: updi.c $(wildcard *.h)
	$(CC) -c $<

util.o: util.c $(wildcard *.h)
	$(CC) -c $<

clean:
	-/bin/rm $(wildcard *.o) $(wildcard *.elf) $(wildcard *.bin)

flash: backend.bin
	python picoupdi.py --port /dev/ttyACM1 --gpio 15 --speed 115200 --chip $(MCU) write backend.bin

reset: backend.bin
	python picoupdi.py --port /dev/ttyACM1 --gpio 15 --speed 115200 --chip $(MCU) getsib

svn: svnadd
	-svn commit -m ""

svnadd:
	-(cd frontend && $(MAKE) svnadd)
	-svn add *.c *.h *.py *.txt
