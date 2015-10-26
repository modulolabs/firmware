/*
 * IR3.cpp
 *
 * Created: 10/21/2015 2:30:10 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR3)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <inttypes.h>
#include <stdint.h>

#include "IR.h"

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


static const uint16_t IR_BREAK_LENGTH=512; // Maximum number of ticks for SPACE before ending

// Length of the current mark or space
static volatile uint16_t _currentCount = 0;


void IR3ReceiveEnable() {
	cli();
	
	// Initialize the reader
	_currentCount = 0;
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

void IR3ReceiveDisable() {
	TIMSK2 &= ~_BV(OCIE2A);
}

static bool _irReadPin() {
	return (PINA & _BV(0));
}




#if 0
RingBuffer<uint8_t, 256> irRingBuffer;

static void pushByte(uint8_t b) {
	if (irRingBuffer.isFull()) {
		// Drop at least 1 byte, even if it's the STOP token
		irRingBuffer.pop();
				
		while (!irRingBuffer.isEmpty() and irRingBuffer.peek() != IR_TOKEN_END) {
			irRingBuffer.pop();
		}
	}
	irRingBuffer.push(b);
}

#endif

void onIRReadComplete(uint8_t *data, uint8_t len);


static uint8_t _irBuffer[IR_BUFFER_SIZE];
static uint8_t _irBufferLen = 0;

void pushByte(uint8_t x) {
	if (_irBufferLen == IR_BUFFER_SIZE) {
		return;
	}
	
	_irBuffer[_irBufferLen++] = x;
}


// Store the _currentCount as either a mark or space in _irReadBuffer
// Resets _currentcount to 0

static void _storeInterval() {
	// Cannot store a 0-length interval.
	// This shouldn't get hit, but convert it to length 1 just in case.
	if (_currentCount == 0) {
		_currentCount = 1;
	}
	
	if (_currentCount >= 255) {
		pushByte(0);
		pushByte(_currentCount & 0xFF);
		pushByte(_currentCount >> 8);
		_currentCount = 0;
		return;
	}

	pushByte(_currentCount);
	
	// Reset the tick counter
	_currentCount = 0;
}

void receiveISR() {
	if (_currentCount != 0xFFFF) {
		_currentCount++;
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
		if (_currentCount > IR_BREAK_LENGTH) {
			onIRReadComplete(_irBuffer, _irBufferLen);
			_irBufferLen = 0;
			_state = READY;
		}
		break;
	}
}


uint8_t sendData[96] = {10, 100, 10, 100, 10, 100};
uint8_t sendPos = 0;
uint8_t sendLen = 6;

ISR(TIMER2_COMPA_vect)
{
	//receiveISR();
	//enableIROut(38);
	//sendMark();
	return;
	/*
	if (sendPos < sendLen) {
		if (sendData[sendPos] == 0) {
			sendPos;
			if (sendPos % 2 ) {
				sendMark();
			} else {
				sendSpace();
			}
		}
		sendData[sendPos]--;	
	}
	
	if (sendPos >= sendLen) {
		sendPos = 0;
	}
	*/
}


#endif