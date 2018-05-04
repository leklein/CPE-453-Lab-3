#Change this variable to point to your Arduino device
#Mac - it may be different
#DEVICE = /dev/tty.usbmodem1431

#Linux (/dev/ttyACM0 or possibly /dev/ttyUSB0)
#DEVICE = /dev/ttyACM0 

#Windows
DEVICE = COM3 

#program3a target to compile the code and flash to the arduino
program_3a: program3a.c os.c serial.c
	avr-gcc -mmcu=atmega2560 -DF_CPU=16000000 -O2 -o program3a.elf program3a.c os.c serial.c
	avr-objcopy -O ihex program3a.elf program3a.hex
	avr-size program3a.elf
	avrdude -D -pm2560 -P $(DEVICE) -c wiring -F -u -U flash:w:program3a.hex

#remove build files
clean:
	rm -fr *.elf *.hex *.o
