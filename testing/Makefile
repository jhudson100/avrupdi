CC=avr-gcc -Wall -O2 -g

%.bin: %.elf
	avr-objcopy -O binary -R .eeprom --gap-fill 0xff $< $@

prog16eb14.elf: prog.c Makefile
	$(CC) -DLED_PORT=PORTC "-DLED_PIN=(1<<1)" -mmcu=avr16eb14 $< -o $@

prog64db32.elf: prog.c Makefile
	$(CC) -DLED_PORT=PORTA "-DLED_PIN=(1<<3)" -mmcu=avr64db32 $< -o $@

prog32dd20.elf: prog.c Makefile
	$(CC) -DLED_PORT=PORTA "-DLED_PIN=(1<<6)" -mmcu=avr32dd20 $< -o $@

prog128da48.elf: prog.c Makefile
	$(CC) -DLED_PORT=PORTD "-DLED_PIN=(1<<4)" -mmcu=avr128da48 $< -o $@

clean:
	-/bin/rm $(wildcard *.elf) $(wildcard *.bin)
