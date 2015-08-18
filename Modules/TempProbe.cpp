/*
 * TempProbe.cpp
 *
 * Created: 8/3/2015 1:00:47 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_TEMP_PROBE)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include <util/delay.h>

volatile uint16_t temperature = 0;

const char *ModuloDeviceType = "co.modulo.joystick";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Joystick";
const char *ModuloDocURL = "modulo.co/docs/joystick";


#define FUNCTION_GET_TEMPERATURE 0



bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
   switch(command) {
        case FUNCTION_GET_TEMPERATURE:
			{
				uint16_t t = temperature;
	            buffer->AppendValue<uint16_t>(t);
			}
	        return true;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {

	}
    return false;
}

void ModuloReset() {

}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
/*
	if (buttonPressed or buttonReleased) {
		*eventCode = EVENT_BUTTON_CHANGED;
		*eventData = (buttonPressed << 8) | buttonReleased;
		return true;
	} else if (positionChanged) {
		*eventCode = EVENT_POSITION_CHANGED;
		*eventData = (hPos << 8) | (vPos);
		return true;
	}
	*/
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	/*
	if (eventCode == EVENT_BUTTON_CHANGED) {
		buttonPressed = false;
		buttonReleased = false;
	}
	if (eventCode == EVENT_POSITION_CHANGED) {
		positionChanged = false;
	}
	*/
}

int main(void)
{
	ClockInit();
    
	ModuloInit(&DDRA, &PORTA, _BV(5));

	while(1)
	{
		// Determine the new position and button state
		volatile uint16_t newValue = ADCRead(11);
		
		// Disable interrupts and atomically update the state
		noInterrupts();
		
		interrupts();
		
		// Rate limit the updates
		_delay_ms(5);
		
		moduloLoop();
		

	}
}

#endif
