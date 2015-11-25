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

volatile int16_t temperature = 0;
volatile int16_t lastEventTemperature = 0;

#define FUNCTION_GET_TEMPERATURE 0
#define FUNCTION_GET_RAW_VALUE   1

#define EVENT_TEMPERATURE_CHANGE 0

bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
   switch(command) {
        case FUNCTION_GET_TEMPERATURE:
            buffer->AppendValue<uint16_t>(static_cast<uint16_t>(temperature));
			return true;
		case FUNCTION_GET_RAW_VALUE:
			buffer->AppendValue<uint16_t>( ADCRead(11));
			return true;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_GET_TEMPERATURE:
		case FUNCTION_GET_RAW_VALUE:
			return true;
	}
    return false;
}

void ModuloReset() {

}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	if (temperature != lastEventTemperature) {
		*eventCode = EVENT_TEMPERATURE_CHANGE;
		*eventData = temperature;
		return true;
	}

	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	if (eventCode == EVENT_TEMPERATURE_CHANGE and static_cast<int16_t>(eventData) == temperature) {
		lastEventTemperature = eventData;
	}
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
	volatile float filteredTemp = readTemperature();
	uint32_t lastTemperatureUpdate = 0;
	temperature = filteredTemp*10;
	
	ClockInit();
	ModuloInit();

	while(1)
	{
		ModuloUpdateStatusLED();
				
		// Constantly read the temperature (every 1ms) and apply a low pass IIR filter.
		float filterRate = .01;
		float newTemperature = readTemperature();
		filteredTemp = filterRate*newTemperature + (1.0-filterRate)*filteredTemp;
		_delay_ms(1);
		
		// Every 100ms, update the temperature from the current filtered value
		if ((millis()-lastTemperatureUpdate) > 100) {
			lastTemperatureUpdate = millis();
			
			// Disable interrupts and atomically update the state
			noInterrupts();
			temperature = filteredTemp*10; // tenths of degrees
			interrupts();
		}

	}
}

#endif
