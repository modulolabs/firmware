/*
 * ADC.cpp
 *
 * Created: 3/19/2015 2:52:13 PM
 *  Author: ekt
 */ 

#include "ADC.h"
#include <avr/io.h>

uint16_t ADCRead(uint8_t channel, uint8_t aref)
{

#if defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny88__)
	ADMUX = channel | _BV(REFS0);
	
#elif defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny841__)
	// Set the channel and use the AVCC reference
	ADMUXA = channel;
	ADMUXB = (aref << REFS0); // Select the voltage reference
	
#else
#error "No ADC Implementation for MCU"
#endif	
	
	ADCSRA |= 7; // 128x prescaler
	ADCSRA |= _BV(ADEN); // Enable the ADC
	
	// Enable the ADC, and start conversion
	ADCSRA |= _BV(ADSC);

	// Wait until the conversion has completed
	while (ADCSRA & _BV(ADSC))
	;

	// Must read low before high
	volatile uint8_t low = ADCL;
	volatile uint8_t high = ADCH;

	return (high << 8) | low;
}
