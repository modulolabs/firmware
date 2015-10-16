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


void TwoWireInit() {
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = _BV(TWDIE) | _BV(TWASIE) | _BV(TWEN) | _BV(TWSIE);
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

// The two wire interrupt service routine
ISR(TWI_SLAVE_vect)
{
	uint8_t status = TWSSRA;
	bool dataInterruptFlag = (status & _BV(TWDIF)); // Check whether the data interrupt flag is set
	bool isAddressOrStop = (status & _BV(TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	bool isReadOperation = (status & _BV(TWDIR));
	bool addressReceived = (status & _BV(TWAS)); // Check if we received an address and not a stop
	
	
	//volatile bool clockHold = (TWSSRA & _BV(TWCH));
	//volatile bool receiveAck = (TWSSRA & _BV(TWRA));
	//volatile bool collision = (TWSSRA & _BV(TWC));
	//volatile bool busError = (TWSSRA & _BV(TWBE));
	
	// Address received
	if (isAddressOrStop and addressReceived) {
		// The address is in the high 7 bits, the RD/WR bit is in the lsb
		uint8_t address = TWSD;

		if (isReadOperation) {
			moduloReadBuffer.Reset(address);
			bool retval = TwoWireReadCallback(moduloWriteBuffer.GetCommand(), moduloWriteBuffer, &moduloReadBuffer);
			_Acknowledge(retval /*ack*/, false /*complete*/);
			
			moduloReadBuffer.AppendValue(moduloReadBuffer.ComputeCRC(address));
		} else {
			_Acknowledge(true /*ack*/, false /*complete*/);
			moduloWriteBuffer.Reset(address);
		}
	}
	
	// Data Read
	if (dataInterruptFlag and isReadOperation) {
		uint8_t data = 0;
		bool ack = moduloReadBuffer.GetNextByte(&data);
		TWSD = data;
		_Acknowledge(ack /*ack*/, !ack /*complete*/);
	}
	
	// Data Write
	if (dataInterruptFlag and !isReadOperation) {		
		uint8_t data = TWSD;
		_Acknowledge(true, false);

		moduloWriteBuffer.Append(data);
	}
	
	// Stop
	if (isAddressOrStop and !addressReceived) {
		_Acknowledge(true /*ack*/, true /*complete*/);
		
		if (!isReadOperation and moduloWriteBuffer.IsValid()) {
			TwoWireWriteCallback(moduloWriteBuffer);
		}
	}
}
#endif
