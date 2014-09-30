/*
 * KnobModule.cpp
 *
 * Created: 5/11/2014 10:19:44 AM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_KNOB)

#include <avr/io.h>
#include <avr/delay.h>
#include "TwoWire.h"


/*
  LED - PA0
  A   - GPIO 4 - PA3
  COM - GPIO 3 - PA7
  B   - GPIO 2 - PB2
*/

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {

}

static volatile int8_t steps = 0;

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {
	*numBytes = 1;
	data[0] = steps;
	steps = 0;
}

uint8_t previousState = 0;

uint8_t forward[] = {1, 3, 0, 2};
uint8_t backward[] = {2, 0, 3, 1};

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	
	// Enable outputs
	DDRA |= (1 << 7) | 1;
		
	// Enable pullups on the A/B pins
	PUEA |= (1 << 3);
	PUEB |= (1 << 2);
	
	while(1)
	{
		bool a = ((1 << PINA3) & PINA) != 0;
		bool b = ((1 << PINB2) & PINB) != 0;
		
		if (!b) {
			PORTA |= 1;
		} else {
			PORTA &= ~1;
		}
		
		uint8_t newState = a << 1 | b;
		if (newState != previousState) {
			if (newState == forward[previousState]) {
				steps++;
			} else if (newState == backward[previousState]) {
				steps--;
			}
			previousState = newState;
		}
		_delay_ms(1);
	}
}

#endif
