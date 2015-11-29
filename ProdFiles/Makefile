
ALL = KnobWithBootloader.hex DisplayWithBootloader.hex BlankSlateWithBootloader.hex \
	  IRWithBootloader.hex JoystickWithBootloader.hex MotorWithBootloader.hex \
	  TempProbeWithBootloader.hex


default : $(ALL)

%WithBootloader.hex : %.hex bootloader-attiny.hex
	./mergeBootloader.py -avr bootloader-attiny.hex $*.hex $*WithBootloader.hex

DisplayWithBootloader.hex : Display.hex bootloader-samd20.hex
	./mergeBootloader.py -samd bootloader-samd20.hex Display.hex DisplayWithBootloader.hex

clean :
	rm *WithBootloader.hex
