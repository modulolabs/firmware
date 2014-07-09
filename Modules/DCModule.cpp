/*
 * DCModule.cpp
 *
 * Created: 6/27/2014 10:55:16 AM
 *  Author: ekt
 */ 


#include "Setup.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_DC)

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



static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {

}

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	

	// Enable the LED
	DDRA |= _BV(5);
	PORTA |= _BV(5);	

	DDRA |= 0xF | _BV(7);
	DDRB |= _BV(0) | _BV(1) | _BV(2);

	volatile uint8_t *in1Port = &PORTA;
	volatile uint8_t *in2Port = &PORTA;
	volatile uint8_t *in3Port = &PORTA;
	volatile uint8_t *in4Port = &PORTB;
	volatile uint8_t *en1Port = &PORTA;
	volatile uint8_t *en2Port = &PORTA;
	volatile uint8_t *en3Port = &PORTB;
	volatile uint8_t *en4Port = &PORTB;
	
	uint8_t in1Mask = _BV(0);
	uint8_t in2Mask = _BV(2);
	uint8_t in3Mask = _BV(7);
	uint8_t in4Mask = _BV(1);
	uint8_t en1Mask = _BV(1);
	uint8_t en2Mask = _BV(3);
	uint8_t en3Mask = _BV(2);
	uint8_t en4Mask = _BV(0);

	*en1Port |= en1Mask;
	*en2Port |= en2Mask;
	*en3Port |= en3Mask;
	*en4Port |= en4Mask;
	
	//*in1Port |= in1Mask;
	//*in3Port |= in3Mask;

	
	while(1)
	{
		*in1Port |= in1Mask;
		_delay_ms(500);
		*in1Port &= ~in1Mask;
		_delay_ms(5000);
		
		*in2Port |= in2Mask;
		_delay_ms(500);
		*in2Port &= ~in2Mask;
		_delay_ms(5000);
		
		/*
		*in1Port ^= in1Mask;
		*in2Port ^= in2Mask;
		*in3Port ^= in3Mask;
		*in4Port ^= in4Mask;
		*/
		
		//_delay_ms(500);
	}
}

#endif
