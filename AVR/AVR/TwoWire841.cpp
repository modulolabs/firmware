/*
 * TwoWire841.cpp
 *
 * Created: 7/16/2015 7:16:26 PM
 *  Author: ekt
 */ 

#if defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny441__)

#include "Buffer.h"
#include "TwoWire.h"
#include "Config.h"
#include <avr/io.h>
#include <avr/interrupt.h>

static volatile uint8_t _deviceAddress = 0;

#if !defined(__AVR_ATtiny841__)
#error "Only works with ATtiny841"
#endif


void TwoWireInit(uint8_t broadcastAddress, bool useInterrupts) {
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = _BV(TWEN) | _BV(TWSIE);
	
	if (useInterrupts) {
		TWSCRA |= _BV(TWDIE) | _BV(TWASIE) ;
	}
	
	TWSCRB = _BV(TWHNM);

	// Listen for message on the broadcast address
	TWSAM = (broadcastAddress << 1)| 1;
}

void TwoWireSetDeviceAddress(uint8_t address) {
	_deviceAddress = address;
	TWSA = (address << 1);
}

uint8_t TwoWireGetDeviceAddress() {
	return _deviceAddress;
}

static void _Acknowledge(bool ack, bool complete=false) {
	if (ack) {
		TWSCRB &= ~_BV(TWAA);
	} else {
		TWSCRB |= _BV(TWAA);
	}
	
	TWSCRB |= _BV(TWCMD1) | (complete ? 0 : _BV(TWCMD0));
}


#define TWI_BUFFER_SIZE 32
static uint8_t twiBuffer[TWI_BUFFER_SIZE];
static uint8_t twiBufferLen = 0;
static uint8_t twiReadPos = 0;
static uint8_t twiAddress = 0;

enum TWIState {
	TWIStateIdle,
	TWIStateRead,
	TWIStateWrite
};

static TWIState twiState = TWIStateIdle;


// After certain types of bus errors, the two wire interface can lock up
// with SDA held low. This prevents any further communication on the bus
// including a global reset. Unfortunately, I haven't figured out a way
// to detect and handle this condition using the TWI registers.
//
// This function works around the issue by checking the the SDA pin every
// time it's called. If the pin is low a certain number of times in a row,
// it assumes that the TWI may be locked and resets it.
//
// This function gets called by the global clock overflow handler to ensure
// it is executed at a frequent and predictable rate.
//
uint16_t sdaLowCount = 0;
void TwoWireWatchdog() {
	bool sdaValue = (PINA & _BV(6));
	if (!sdaValue) {
		sdaLowCount++;
	} else {
		sdaLowCount = 0;
	}
	
	// Each tick is 2048us, so 500 is about 1 second
	if (sdaLowCount == 500) {
		TWSCRA &= ~_BV(TWEN);
		TWSCRA |= _BV(TWEN);
	}
}


void TwoWireUpdate() {
	uint8_t status = TWSSRA;
	bool dataInterruptFlag = (status & _BV(TWDIF)); // Check whether the data interrupt flag is set
	bool isAddressOrStop = (status & _BV(TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	bool isReadOperation = (status & _BV(TWDIR));
	bool addressReceived = (status & _BV(TWAS)); // Check if we received an address and not a stop
		
	// Clear the interrupt flags
	// TWSSRA |= _BV(TWDIF) | _BV(TWASIF);
	
	//volatile bool clockHold = (TWSSRA & _BV(TWCH));
	//volatile bool receiveAck = (TWSSRA & _BV(TWRA));
	//volatile bool collision = (TWSSRA & _BV(TWC));
	//volatile bool busError = (TWSSRA & _BV(TWBE));
	
	// Handle address received and stop conditions
	if (isAddressOrStop) {
		// We must capture the address before acknowledging and releasing the clock hold,
		// since otherwise the next byte could arrive before we read TWSD. (particularly
		// in polled mode when another interrupt occurs)
		//
		// The address is in the high 7 bits, the RD/WR bit is in the lsb
		uint8_t newAddress = TWSD >> 1;
		
		// Send an ack unless a read is starting and there are no bytes to read.
		bool ack = (twiBufferLen > 0) or (!isReadOperation) or (!addressReceived);
		_Acknowledge(ack, !addressReceived /*complete*/);
		
		// If we were previously in a write, then execute the callback and setup for a read.
		if ((twiState == TWIStateWrite) and twiBufferLen != 0) {
			twiBufferLen = TwoWireCallback(twiAddress, twiBuffer, twiBufferLen, TWI_BUFFER_SIZE);		
			twiReadPos = 0;
		}
		
		if (!addressReceived) {
			twiState = TWIStateIdle;
		} else if (isReadOperation) {
			twiState = TWIStateRead;
		} else {
			twiState = TWIStateWrite;
			twiBufferLen = 0;
		}
		
		twiAddress = newAddress;	
	}
	
	// Data Read
	if (dataInterruptFlag and isReadOperation) {
		if (twiReadPos < twiBufferLen) {
			TWSD = twiBuffer[twiReadPos++];
			_Acknowledge(true /*ack*/, false /*complete*/);
		} else {
			TWSD = 0;
			_Acknowledge(false /*ack*/, true /*complete*/);
		}
	}
	
	// Data Write
	if (dataInterruptFlag and !isReadOperation) {		
		uint8_t data = TWSD;
		_Acknowledge(true, false);

		if (twiBufferLen < TWI_BUFFER_SIZE) {
			twiBuffer[twiBufferLen++] = data;
		}
	}
}

// The two wire interrupt service routine
ISR(TWI_SLAVE_vect)
{
	TwoWireUpdate();
}

#endif
