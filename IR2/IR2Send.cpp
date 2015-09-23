/*
 * IR2Send.cpp
 *
 * Created: 9/12/2015 4:48:50 PM
 *  Author: ekt
 */ 

#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR2)

#include <inttypes.h>
#include <avr/io.h>
#include "Clock.h"
#include "IREncoding.h"

static void sendMark(uint16_t time) {
	// Ouput compares 0, 1, 2, 6, and 7 are all connected to the IR LED
	TOCPMCOE = _BV(0) | _BV(1) | _BV(2) | _BV(6) | _BV(7);
  
	if (time > 0) {
		delayMicroseconds(time);
	}
}

static void sendSpace(uint16_t time) {
	// Sends an IR space for the specified number of microseconds.
	// A space is no output, so the PWM output is disabled.
	TOCPMCOE = 0;
	  
	if (time > 0) {
		delayMicroseconds(time);
	}
}

static void sendPulseModulation(PulseModulationEncoding &encoding, uint32_t data) {
	// Send the header
	if (encoding.headerMark) {
		sendMark(encoding.headerMark);
		sendSpace(encoding.headerSpace);
	}

	// Send the data
	for (int i = 0; i < (int)encoding.numBits; i++) {
		if (data & _BV(i)) {
			sendMark(encoding.oneMark);
			sendSpace(encoding.oneSpace);
			} else {
			sendMark(encoding.zeroMark);
			sendSpace(encoding.zeroSpace);
		}
	}

	// Send stop mark
	if (encoding.stopMark) {
		sendMark(encoding.stopMark);
	}
		
	// Return to idle
	sendSpace(0);
}



bool IR2Send(uint8_t protocol, uint32_t data) {
	PulseModulationEncoding encoding;
	if (GetIREncoding(protocol, &encoding)) {
		sendPulseModulation(encoding, data);
		return true;
	}
	
	return false;
}

#endif
