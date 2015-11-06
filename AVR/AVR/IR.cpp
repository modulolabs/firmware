/*
 * IR3.cpp
 *
 * Created: 10/21/2015 2:30:10 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR)

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

// Maximum number of ticks for SPACE before ending
static uint16_t _irBreakLength=512;

// Length of the current mark or space
static volatile uint16_t _currentCount = 0;

// The _irBuffer is used for both sending and receiving
static uint8_t _irBuffer[IR_BUFFER_SIZE];

// When receiving, this is the number of bytes stored in the buffer
static volatile uint8_t _irReceiveLen = 0;

// When sending, this is the number of bytes stored in the buffer.
static volatile uint8_t _irSendLen = 0;

// When sending, this is the current position within the buffer.
static volatile uint8_t _irSendPos = 6;

// The state to use for the status led during IR transmission/reception
static volatile bool _irActivity = false;

void onIRReadComplete(uint8_t *data, uint8_t len);

// Configure the Timer2 interrupt to fire every USECPERTICK microseconds.
static void _configureTimer() {
	cli();
	
	// Initialize the reader
	_currentCount = 0;
	_state = READY;
	
	
	uint8_t modeA = 2; // Clear on compare match
	uint8_t modeB = 2; // Clear on compare match
	uint8_t wgm = 14;  // Fast PWM with TOP=ICR1
	uint8_t cs = 1;    // No prescaler

	TCCR2A =  (modeA << COM1A0) | (modeB << COM1B0) | (wgm & 0b11);
	TCCR2B =  ((wgm >> 2) << WGM12) | cs;
 
	// Top = F_CPU/(F*N)-1 where
	//    F is the desired frequency
	//    N is the clock prescaler (1, 8, 64, 256, or 1024
	int khz = 20;
	volatile long topValue = F_CPU/khz/1000 - 1;

	// ICR2 is the top value.
	ICR2 = topValue;
	
#if 0
	// Configure Timer2 to fire an interrupt every USECPERTICK microseconds
	TCCR2A = 0;
	TCCR2B = _BV(WGM22) | _BV(CS20);
	OCR2A = F_CPU * USECPERTICK / 1000000;
	TCNT2 = 0;


#endif

	// Enable the Timer2 Overflow Interrupt
	TIMSK2 = _BV(OCIE2A);
	
	sei();
}

// Configure Timer1 for IR output waveform generation at the specified frequency.
static void _configurePWM(int khz) {

	uint8_t modeA = 2; // Clear on compare match
	uint8_t modeB = 2; // Clear on compare match
	uint8_t wgm = 14;  // Fast PWM with TOP=ICR1
	uint8_t cs = 1;    // No prescaler

	TCCR1A =  (modeA << COM1A0) | (modeB << COM1B0) | (wgm & 0b11);
	TCCR1B =  ((wgm >> 2) << WGM12) | cs;
 
	// Top = F_CPU/(F*N)-1 where
	//    F is the desired frequency
	//    N is the clock prescaler (1, 8, 64, 256, or 1024

	volatile long topValue = F_CPU/khz/1000 - 1;

	// ICR1 is the top value.
	ICR1 = topValue;
	
	// OCR1 is the compare value.
	OCR1A = topValue/4;
	OCR1B = topValue/4;
	
	// We use multiple output pins to drive the IR LED. This lets us get more
	// led drive current than would be possible using a single output pin.
	
	// Set the DDR bits for the output pins on PA1, PA2, PA3, PA7, and PB2.
	DDRA |= _BV(1) | _BV(2) | _BV(3) | _BV(7);
	DDRB |= _BV(2);
	
	// Set the output compare MUX to OCR1A/OCR1B for each output pin
	// The outputs are TOCC0, TOCC1, TOCC2, TOCC6, and TOCC7
	TOCPMSA0 |= _BV(TOCC0S0) | _BV(TOCC1S0) | _BV(TOCC2S0);
	TOCPMSA1 |= _BV(TOCC6S0) | _BV(TOCC7S0);
}

void IRInit() {
	_configureTimer();
	_configurePWM(38);
}

// Return whether we are in an idle state: not sending and idle for at least
// IR_BREAK_LENGTH ticks.
bool IRIsIdle() {
	return (_irSendLen == 0) and (_state == READY) and (_currentCount >= _irBreakLength);
}

void IRSend(uint8_t *data, uint8_t len) {
	_irSendLen = len < IR_BUFFER_SIZE ? len : IR_BUFFER_SIZE;
	_irSendPos = 0;
	for (int i=0; i < _irSendLen; i++) {
		_irBuffer[i] = data[i];
	}	
}

bool IRGetActivity() {
	return _irActivity;
}

void IRSetBreakLength(uint16_t len) {
	_irBreakLength = len;
}

// Enable the PWM signal on the IR LED
static void _sendMark() {
	// Ouput compares 0, 1, 2, 6, and 7 are all connected to the IR LED
	TOCPMCOE = _BV(0) | _BV(1) | _BV(2) | _BV(6) | _BV(7);
	_irActivity = true;
}

// Disable the PWM signal on the IR LED
static void _sendSpace() {
	// Sends an IR space for the specified number of microseconds.
	// A space is no output, so the PWM output is disabled.
	TOCPMCOE = 0;
	_irActivity = false;
}

// Read the state of the IR input
static bool _irReadPin() {
	return (PINA & _BV(0));
}

// Store a byte at the end of the receive buffer
void pushByte(uint8_t x) {
	if (_irReceiveLen == IR_BUFFER_SIZE) {
		return;
	}
	
	_irBuffer[_irReceiveLen++] = x;
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

// Called every USECPERTICK when receiving
void receiveTick() {
	bool irState = !_irReadPin();
	_irActivity = irState;
	
	if (_currentCount != 0xFFFF) {
		_currentCount++;
	}
	
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
				onIRReadComplete(_irBuffer, _irReceiveLen);
				_irReceiveLen = 0;
				_state = READY;
			}
			break;
	}
}



// Called every USECPERTICK microseconds when sending
void sendTick() {
	// Skip any intervals of 0 length.
	while (_irSendPos < _irSendLen and _irBuffer[_irSendPos] == 0) {
		_irSendPos++;
	}
	
	// If we have reached the end, reset everything and return.
	if (_irSendPos >= _irSendLen) {
		_irSendPos = 0;
		_irSendLen = 0;
		_currentCount = 0;
		_irReceiveLen = 0;
		_sendSpace();
		return;
	}
	
	// Decrement the interval
	_irBuffer[_irSendPos]--;
	
	// Set the mark or space based on which interval we're in
	if (_irSendPos % 2 ) {
		asm("nop");
		_sendMark();
	} else {
		asm("nop");
		_sendSpace();
	}
		
	

}

ISR(TIMER2_COMPA_vect)
{
	if (_irSendLen > 0) {
		sendTick();
	} else {
		receiveTick();
	}
}


#endif