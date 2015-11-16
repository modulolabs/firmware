/*
 * TempProbe.cpp
 *
 * Created: 8/18/2015 11:36:19 AM
 *  Author: ekt
 */ 

#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_TEMP_PROBE)

#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include "ModuloInfo.h"

#include <avr/io.h>
#include <util/delay.h>
#include <math.h>

DECLARE_MODULO("co.modulo.tempprobe", 1);

volatile uint16_t temperature = 0;


#define FUNCTION_GET_TEMPERATURE 0

uint32_t R_values[] = {
	190953,
	145953,
	112440,
	87285,
	68260,
	53762,
	42636,
	34038,
	27348,
	22108,
	17979,
	14706,
	12094,
	10000,
	8311,
	6941,
	5825,
	4911,
	4158,
	3536,
	3020,
	2589,
	2228,
	1925,
	1668,
	1451,
	1267,
	1109,
	974,
	858,
};

bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
   switch(command) {
        case FUNCTION_GET_TEMPERATURE:
            buffer->AppendValue<uint16_t>((uint16_t)temperature);
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
#if 0
	if (buttonPressed or buttonReleased) {
		*eventCode = EVENT_BUTTON_CHANGED;
		*eventData = (buttonPressed << 8) | buttonReleased;
		return true;
	} else if (positionChanged) {
		*eventCode = EVENT_POSITION_CHANGED;
		*eventData = (hPos << 8) | (vPos);
		return true;
	}
#endif
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
#if 0
	if (eventCode == EVENT_BUTTON_CHANGED) {
		buttonPressed = false;
		buttonReleased = false;
	}
	if (eventCode == EVENT_POSITION_CHANGED) {
		positionChanged = false;
	}
#endif
}

float valueToTemperature(uint16_t val) {
	float R = 1024.0/val - 1;
	float T0 = 25.0;
	float B = 3435;
	float tInv = 1/(T0+273.15) + (1.0/B)*log(R);
	return (1/tInv - 273.15);
}

float readTemperature() {
	uint16_t newValue = ADCRead(11);
	return valueToTemperature(newValue);		
}

int main(void)
{
	ClockInit();
    
	ModuloInit();

	volatile float filteredTemp = readTemperature();
	
	while(1)
	{
		float filterRate = .01;
		volatile float newTemperature = readTemperature();
		filteredTemp = filterRate*newTemperature + (1.0-filterRate)*filteredTemp;

		// Disable interrupts and atomically update the state
		noInterrupts();
		temperature = filteredTemp*10; // tenths of degrees
		interrupts();
		
		// Rate limit the updates
		_delay_ms(5);
		
		ModuloUpdateStatusLED();
	}
}

#endif
