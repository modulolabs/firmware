/*
 * TwoWire841.cpp
 *
 * Created: 7/16/2015 7:16:26 PM
 *  Author: ekt
 */ 

#if defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny441__)

#include "Modulo.h"
#include "Buffer.h"
#include "TwoWire.h"
#include "Config.h"

static volatile uint8_t _deviceAddress = 0;

#if !defined(__AVR_ATtiny841__)
#error "Only works with ATtiny841"
#endif


void TwoWireInit(bool useInterrupts) {
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = _BV(TWEN);
	
	if (useInterrupts) {
		TWSCRA |= _BV(TWDIE) | _BV(TWASIE) | _BV(TWSIE);
	}
	
	TWSCRB = _BV(TWHNM);

	// Also listen for message on the broadcast address
	TWSAM = (moduloBroadcastAddress << 1)| 1;
}

void TwoWireSetDeviceAddress(uint8_t address) {
	_deviceAddress = address;
	TWSA = (address << 1);
	ModuloSetStatus(ModuloStatusOff);
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



void TwoWireUpdate() {
	uint8_t status = TWSSRA;
	bool dataInterruptFlag = (status & _BV(TWDIF)); // Check whether the data interrupt flag is set
	bool isAddressOrStop = (status & _BV(TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	bool isReadOperation = (status & _BV(TWDIR));
	bool addressReceived = (status & _BV(TWAS)); // Check if we received an address and not a stop
	
	
	//volatile bool clockHold = (TWSSRA & _BV(TWCH));
	//volatile bool receiveAck = (TWSSRA & _BV(TWRA));
	//volatile bool collision = (TWSSRA & _BV(TWC));
	//volatile bool busError = (TWSSRA & _BV(TWBE));
	
	// Handle address received and stop conditions
	if (isAddressOrStop) {
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
		
		// The address is in the high 7 bits, the RD/WR bit is in the lsb
		twiAddress = TWSD >> 1;
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
