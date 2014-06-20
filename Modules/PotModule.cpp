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
#include "TwoWire.h"


/*
  LED - PA0
  A   - GPIO 4 - PA3
  COM - GPIO 3 - PA7
  B   - GPIO 2 - PB2
*/

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {

}

static volatile uint8_t valueHigh = 0;
static volatile uint8_t valueLow = 0;

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {
	*numBytes = 2;
	data[0] = valueHigh;
	data[1] = valueLow;
}

uint8_t previousState = 0;

uint8_t forward[] = {1, 3, 0, 2};
uint8_t backward[] = {2, 0, 3, 1};

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	
	// Enable the LED
	DDRA |= _BV(1);
	PORTA |= _BV(1);
	
	// Enable drive pins
	DDRB |= _BV(0) | _BV(1);
	PORTB |= _BV(1);
	
	// Configure the ADC
	ADMUXA = 8; // Read from ADC08

	// Configure a timer for the LED


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

	while(1)
	{
		// Start conversion
		ADCSRA |= _BV(ADEN) | _BV(ADSC);
		
		// Wait for conversion to complete
		while (ADCSRA & _BV(ADSC)) {
		}
		
		cli();
		// Must read ADCL before ADCH
		valueLow  = ADCL;
		valueHigh = ADCH;
		
		float floatValue = ((valueHigh << 8) | valueLow)/1023.0;

		// Apply gamma correction for perceived brightness
		int16_t brightness = (floatValue*floatValue*1023);
		
		// Write the position to the timer compare register for the LED PWM
		// Must write the high byte before the low byte
		OCR1BH = brightness >> 8;
		OCR1BL = brightness & 0xFF;
			
		sei();
		
		_delay_ms(1);
	}
}

#endif
