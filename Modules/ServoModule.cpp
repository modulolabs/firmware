/*
 * ServoModule.cpp
 *
 * Created: 5/12/2014 3:39:24 PM
 *  Author: ekt
 */ 

/*
 * ButtonsModule.cpp
 *
 * Created: 5/10/2014 9:06:29 PM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_SERVO)

#include <avr/io.h>
#include "TwoWire.h"
#include "Wiring.h"

/*
 * LED - PA0
 * GPIO0 - PB0
 * GPIO1 - PB1
 * GPIO2 - PB2
 * GPIO5 - PA2
 */

static volatile uint8_t positions[3] = {255, 255, 255};

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {
	for (int i=0; i < 3 && i < numBytes; i++) {
		positions[i] = data[i];
	}
}

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {

}

int main(void)
{
		
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	
	
	// PA3 = GPIO4 = TOCC2
	// Set the output mux for TOCC2 to OC1B
	TOCPMSA0 |= (0 << TOCC2S1) | (1 << TOCC2S0); 

	// Set the output enable bit for TOCC2
	TOCPMCOE |= (1 << TOCC2OE);
	
	// Set the compare output mode (COM1B)
	TCCR1A |= (1 << COM1B1) | (0 << COM1B0);

	// Set the waveform generation mode to 7 (10 bit fast pwm)
	TCCR1A |= (7 << WGM10);
	
	// Enable the clock with 1024x prescaling
	TCCR1B = (3 << CS00);
	
	// Set the output compare valu
	OCR1BH = 0;
	OCR1BL = 50;
	
	
	// Min - 50
	// Center - 108
	// Max - 150
	
	
	// Set the data direction register
	DDRA |= (1 << PINA3);
	
	while (1) {
		OCR1BH = 0;
		OCR1BL = 40 + positions[0]*110L/255;
	}
}

#endif
