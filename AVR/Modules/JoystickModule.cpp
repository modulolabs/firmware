/*
 * Joystick.cpp
 *
 * Created: 3/19/2015 2:44:52 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_JOYSTICK)

#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include "ModuloInfo.h"

#include <avr/io.h>
#include <util/delay.h>

DECLARE_MODULO("co.modulo.joystick", 1);

volatile bool buttonState = false;
volatile bool buttonPressed = false;
volatile bool buttonReleased = false;
volatile bool positionChanged = false;
volatile uint8_t hPos = 0;
volatile uint8_t vPos = 0;

#define HORIZONTAL_ADC 11
#define VERTICAL_ADC 10
#define BUTTON_PIN 2

#define FUNCTION_GET_BUTTON 0
#define FUNCTION_GET_POSITION 1

#define EVENT_BUTTON_CHANGED 0
#define EVENT_POSITION_CHANGED 1


bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
   switch(command) {
        case FUNCTION_GET_BUTTON:
            buffer->AppendValue<uint8_t>(buttonState);
            return true;
        case FUNCTION_GET_POSITION:
			buffer->AppendValue<uint8_t>((uint8_t)hPos);
			buffer->AppendValue<uint8_t>((uint8_t)vPos);
			return true;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_GET_BUTTON:
		case FUNCTION_GET_POSITION:
			return buffer.GetSize() == 0;
	}
    return false;
}

void ModuloReset() {

}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	if (buttonPressed or buttonReleased) {
		*eventCode = EVENT_BUTTON_CHANGED;
		*eventData = (buttonPressed << 8) | buttonReleased;
		return true;
	} else if (positionChanged) {
		*eventCode = EVENT_POSITION_CHANGED;
		*eventData = (hPos << 8) | (vPos);
		return true;
	}
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	if (eventCode == EVENT_BUTTON_CHANGED) {
		buttonPressed = false;
		buttonReleased = false;
	}
	if (eventCode == EVENT_POSITION_CHANGED) {
		positionChanged = false;
	}
}

int main(void)
{
	ClockInit();
    
	ModuloInit(&DDRA, &PORTA, _BV(5));
	PUEB |= (1 << BUTTON_PIN);

	while(1)
	{
		// Determine the new position and button state
		volatile uint8_t newHPos = ADCRead(HORIZONTAL_ADC)/4;
		volatile uint8_t newVPos = ADCRead(VERTICAL_ADC)/4;
		volatile bool newButtonState = !(PINB & (1 << BUTTON_PIN));
		
		// Disable interrupts and atomically update the state
		noInterrupts();
		if (newButtonState and !buttonState) {
			buttonPressed = true;
		}
		if (buttonState and !newButtonState) {
			buttonReleased = true;
		}
		if (newHPos != hPos or newVPos != vPos) {
			positionChanged = true;
		}
		hPos = newHPos;
		vPos = newVPos;
		buttonState = newButtonState;
		interrupts();
		
		// Rate limit the updates
		_delay_ms(5);
		
		ModuloUpdateStatusLED();
	}
}

#endif
