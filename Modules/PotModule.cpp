/*
 * PotModule.cpp
 *
 * Created: 6/16/2014 12:15:58 PM
 *  Author: ekt
 */ 


#include "Setup.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_POT)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include "Modulo.h"
#include "Timer.h"

ModuloVariable<float> position(0);

DEFINE_MODULO_CONSTANTS("Integer Labs", "Potentiometer", 0, "http://www.integerlabs.net");
DEFINE_MODULO_FUNCTION_NAMES("Position");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeFloat);

void _ReadModuloValue(uint8_t functionID, ModuloBuffer *buffer) {
	if (functionID == 0) {
		buffer->Set(position.Get());
	}
}

int main(void)
{
	//TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	TimerInit();
	ModuloInit(&DDRA, &PORTA, _BV(1),
			_ReadModuloValue);
	
	// Enable the LED
	//DDRA |= _BV(1);
	//PORTA |= _BV(1);
	
	// Enable drive pins
	DDRB |= _BV(0) | _BV(1);
	PORTB |= _BV(1);
	
	// Configure the ADC
	ADMUXA = 8; // Read from ADC08

	// Configure a timer for the LED

#if 0
	// Non-inverting compare output mode for timer 1 channel B.
	TCCR1A |= _BV(COM1B1);

	// Set the waveform generation mode to 10 bit fast pwm
	TCCR1B |= _BV(WGM12);
	TCCR1A |= _BV(WGM11) | _BV(WGM10);
	
	// Map TOCC0 (pin A1) to ouput compare 1B
	TOCPMSA0 |= _BV(TOCC0S0);
	
	// Enable the output for TOCC0
	TOCPMCOE |= _BV(TOCC0OE);
	
	// Enable the timer clock
	TCCR1B |= _BV(CS10);
#endif

	while(1)
	{
		// Start conversion
		ADCSRA |= _BV(ADEN) | _BV(ADSC);
		
		// Wait for conversion to complete
		while (ADCSRA & _BV(ADSC)) {
		}
		
		// Must read ADCL before ADCH
		uint16_t valueLow  = ADCL;
		uint16_t valueHigh = ADCH;
		
		float floatValue = ((valueHigh << 8) | valueLow)/1023.0;
		
		position.Set(floatValue);
		
		/*
		

		// Apply gamma correction for perceived brightness
		int16_t brightness = (floatValue*floatValue*1023);
		
		// Write the position to the timer compare register for the LED PWM
		// Must write the high byte before the low byte
		OCR1BH = brightness >> 8;
		OCR1BL = brightness & 0xFF;
		*/
		
		_delay_ms(1);
	}
}

#endif
