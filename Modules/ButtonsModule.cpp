/*
 * ButtonsModule.cpp
 *
 * Created: 5/10/2014 9:06:29 PM
 *  Author: ekt
 */ 



#include "Setup.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_BUTTONS)

#include <avr/io.h>
#include "TwoWire.h"

/*
 * LED - PA0
 * GPIO0 - PB0
 * GPIO1 - PB1
 * GPIO2 - PB2
 * GPIO5 - PA2
 */

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {

}

static volatile uint8_t steps = 0;

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {
	*numBytes = 1;
	data[0] = steps;
}

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	
	PUEA = (1 << PINA2);
	PUEB = (1 << PINB0) | (1 << PINB1) | (1 << PINB2);
	
	while(1)
	{
		bool button0 = !(PINB & (1 << PINB0));
		bool button1 = !(PINB & (1 << PINB1));
		bool button2 = !(PINB & (1 << PINB2));
		bool button3 = !(PINA & (1 << PINA2));
		  
		if (button0 or button1 or button2 or button3) {
			PORTA |= 1 << PINA0;
		} else {
			PORTA &= ~(1 << PINA0);
		}
		
		steps = (button3 << 3) | (button2 << 2) | (button1 << 1) | button0;
	}
}

#endif
