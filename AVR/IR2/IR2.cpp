/*
 * IRReceiver.cpp
 *
 * Created: 9/12/2015 9:20:52 AM
 *  Author: ekt
 */ 

#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR2)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdint.h>

#include "IR2.h"

#define USECPERTICK 50  // microseconds per clock interrupt tick

//
// The ISR captures raw IR data and stores it in the _irReadBuffer
//
// _irReadBuffer contains alternating mark/space durations.
//
// Each duration can be represented as either as a single byte indicating
// a duration between 1 and 255 ticks, or a 3 byte sequence.
//
// A 3 byte sequences start with the special token 0, followed by a 16 bit
// count of ticks. Since for normal IR protocols almost all intervals are
// less than 255 ticks long, this significantly reduces the required buffer
// size vs using 16 bits per interval.
//
// The initial state is READY, indicating that no signal has been seen yet.
// The state will alternate between MARK and SPACE while the transmission
// is being received.
// When the duration of a SPACE exceeds IR_BREAK_LENGTH ticks, the
// the transmission is complete and the state returns to READY.

enum IRState {
	READY, // Initial state. No signal detected yet.
	MARK,  // Receiving a signal
	SPACE, // Not receiving a signal
};

// The current state
static volatile IRState _state = READY;


static const uint16_t _irBreakLength=512; // Maximum number of tickets for SPACE before ending

// Length of the current mark or space
static volatile uint16_t _currentCount = 0;

// The raw IR data
volatile uint8_t irRawBuffer[IR_BUFFER_SIZE];
volatile uint8_t irRawLen = 0;

static volatile bool _readComplete = false;

void IR2ReceiveEnable() {
	cli();
	
	// Initialize the reader
	_readComplete = false;
	_currentCount = 0;
	irRawLen = 0;
	_state = READY;
	
	// Configure Timer2 to fire an interrupt every USECPERTICK microseconds	  
	TCCR2A = 0;
	TCCR2B = _BV(WGM22) | _BV(CS20);
	OCR2A = F_CPU * USECPERTICK / 1000000;
	TCNT2 = 0;

	// Enable the Timer1 Overflow Interrupt
	TIMSK2 = _BV(OCIE2A);

	sei();
}

void IR2ReceiveDisable() {
	TIMSK2 &= ~_BV(OCIE2A);
}

bool IR2IsReceiveComplete() {
	return _readComplete;
}

void IR2ClearReceivedData() {
	irRawLen = 0;
	_readComplete = false;
}

static bool _irReadPin() {
	return (PINA & _BV(0));
}

// Store the _currentCount as either a mark or space in _irReadBuffer
// Resets _currentcount to 0
static void _storeInterval() {
	// Cannot store a 0-length interval.
	// This shouldn't get hit, but convert it to length 1 just in case.
	if (_currentCount == 0) {
		_currentCount = 1;
	}

	// If the new interval is less than 256 ticks, just store it as a byte.
	if (_currentCount <= 255 and irRawLen < IR_BUFFER_SIZE) {
		irRawBuffer[irRawLen++] = _currentCount;
	}
	
	// If the new interval is 256 ticks or more, store it as a 3 byte sequence.
	if (_currentCount > 255 and irRawLen+2 < IR_BUFFER_SIZE) {
		irRawBuffer[irRawLen] = 0;
		irRawBuffer[irRawLen+1] = _currentCount & 0xFF;
		irRawBuffer[irRawLen+2] = _currentCount / 0xFF;
		irRawLen += 3;
	}

	// Reset the tick counter
	_currentCount = 0;
}

ISR(TIMER2_COMPA_vect)
{
	if (_currentCount != 0xFFFF) {
		_currentCount++;
	}

	// Don't do anything until the previous data has been processed by
	// the application code.
	if (_readComplete) {
		return;
	}

	bool irState = !_irReadPin();
	
	switch(_state) {
		case READY:
			// If we are READY and see a mark, store the interval and change
			// the state to MARK.
			if (irState) {
				_storeInterval();
				_state = MARK;
				_currentCount = 0;
			}
			break;
		case MARK:
			// If we are in a MARK and see a space, store the count
			// in the buffer and change the state to SPACE
			if (!irState) {
				_storeInterval();
				_state = SPACE;
			}
			break;
		case SPACE:
			// If we are in a SPACE and see mark, store the count
			// in the buffer and change the state to MARK.
			if (irState) {
				_storeInterval();
				_state = MARK;
				break;
			}
			
			// If the space is longer than IR_BREAK_LENGTH, the read
			// is complete. Set a flag and return to the READY state.
			if (_currentCount > _irBreakLength) {
				_readComplete = true;
				_state = READY;
			}
			break;
	}
}

#endif
