/*
 * IR2Decode.cpp
 *
 * Created: 9/12/2015 12:24:43 PM
 *  Author: ekt
 */ 

#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR2)

#include <inttypes.h>

#include "IREncoding.h"
#include "IR2.h"

#define USECPERTICK 50  // microseconds per clock interrupt tick
#define MARK_EXCESS 0

#define TOLERANCE 25  // percent tolerance in measurements
//#define LTOL (1.0 - TOLERANCE/100.)
//#define UTOL (1.0 + TOLERANCE/100.)
#define LTOL 3
#define UTOL 5

#define _GAP 5000 // Minimum map between transmissions
#define GAP_TICKS (_GAP/USECPERTICK)

#define TICKS_LOW(us) (int) (((us)*LTOL/(4*USECPERTICK)))
#define TICKS_HIGH(us) (int) (((us)*UTOL/(4*USECPERTICK) + 1))

class IRDecoder {
public:	
	void reset() {
		_currentIndex = 0;
	}
	
	bool match(int16_t measured, int16_t desired_us) {
		return (measured >= TICKS_LOW(desired_us) and measured <= TICKS_HIGH(desired_us));
	}
	
	bool matchMark(int16_t measured, int16_t desired_us) {
		return match(measured, desired_us+MARK_EXCESS);
	}
	
	bool matchSpace(int16_t measured, int16_t desired_us) {
		return match(measured, desired_us-MARK_EXCESS);
	}
	
	void rewind(int count) {
		for (int i=0; i < count; i++) {
			if (_currentIndex >= 3 and irRawBuffer[_currentIndex-3] == 0) {
				_currentIndex -= 3;
			} else if (_currentIndex > 0) {
				_currentIndex--;
			}
		}
	}
	
	int16_t getNextInterval() {
		// Return 0 if we have gone off the end of the data
		if (_currentIndex >= irRawLen) {
			return 0;
		}
		
		// Get the next interval
		int16_t next = irRawBuffer[_currentIndex++];
		
		// 0 is a special token indicating that we should read a 16 bit value
		if (next == 0) {
			next = irRawBuffer[_currentIndex++];
			next |= (irRawBuffer[_currentIndex++] << 8);
		}
		return next;
	}
	
	
	bool decodePulseModulation(const PulseModulationEncoding &encoding, uint32_t *value)
	{
		_currentIndex = 0;
		*value = 0;
		
		// Skip leading space
		getNextInterval();

		// Match the header
		if (encoding.headerMark) {
			if (!matchMark(getNextInterval(), encoding.headerMark)) {
				return false;
			}

			if (!matchSpace(getNextInterval(), encoding.headerSpace)) {
				return false;
			}
		}

		// Match the data
		for (int i = 0; i < (int)encoding.numBits; i++) {
			int16_t mark = getNextInterval();
			int16_t space = getNextInterval();
			
			if (matchMark(mark, encoding.oneMark) and
				matchSpace(space, encoding.oneSpace)) {
				*value = (*value << 1) | 1;
			} else if (matchMark(mark, encoding.zeroMark) and
					   matchSpace(space, encoding.zeroSpace)) {
				*value <<= 1;
			} else {
				return false;
			}
		}

		// Match the stop mark
		if (encoding.stopMark) {
			if (!matchMark(getNextInterval(), encoding.stopMark)) {
				return false;
			}
		}

		// Success!
		return true;
	}
	
private:
	int16_t _currentIndex;	
};



bool IR2Decode(int8_t *protocol, uint32_t *value) {

	IRDecoder decoder;
	
	
	
	PulseModulationEncoding encoding;
	for (volatile int i=0; i < NUM_IR_ENCODINGS ; i++) {
		if (!GetIREncoding(i, &encoding)) {
			break;
		}
		if (decoder.decodePulseModulation(encoding, value)) {
			*protocol = i;
			return true;
		}
	}

	#ifdef RC5
	if (decodeRC5(results)) {
		return true;
	}
	#endif

	#ifdef RC6
	if (decodeRC6(results)) {
		return true;
	}
	#endif

	return false;
}

#endif
