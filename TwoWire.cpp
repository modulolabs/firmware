/*
 * TwoWire.cpp
 *
 * Created: 5/6/2014 10:09:44 AM
 *  Author: ekt
 */ 


#include <avr/io.h>
#include <avr/interrupt.h>
#include "TwoWire.h"

#define MAX_PACKET_LENGTH 32

static volatile TwoWireDataReceivedCallback _dataReceivedCallback;
static volatile TwoWireDataRequestedCallback _dataRequestedCallback;
static bool _addressReceived = false;
static uint8_t _data[MAX_PACKET_LENGTH];
static uint8_t _numBytes = 0;
static uint8_t _bytesSent = 0;

ISR(TWI_SLAVE_vect)
{
	bool isReadOperation = (TWSSRA & (1 << TWDIR)); 
	bool isAddressOrStop = (TWSSRA & (1 << TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	
	if (isAddressOrStop) {
		_addressReceived = (TWSSRA & (1 << TWAS)); // Check if we received an address and not a stop
		
		TWSCRB |= (1 << TWCMD0) | (1 << TWCMD1);
		TWSSRA &= ~(1 << TWASIF);
		
		if (!_addressReceived and !isReadOperation) {
			if (_dataReceivedCallback) {
				(*_dataReceivedCallback)(_data, _numBytes);
			}
		}
		
		if (_addressReceived) {
			_numBytes = 0;
			_bytesSent = 0;
			
			if (isReadOperation) {
				if (_dataRequestedCallback) {
					(*_dataRequestedCallback)(_data, &_numBytes, MAX_PACKET_LENGTH);
				}
			}
		}
	}
	
	bool isDataInterrupt = (TWSSRA & (1 << TWDIF)); // Check whether the data interrupt flag is set
	if (isDataInterrupt) {
		if (isReadOperation) {
			if (_bytesSent < _numBytes) {
				TWSD = _data[_bytesSent++];
			} else {
				// XXX: Send NACK
				TWSD = 0;
			}

		} else {
			uint8_t data = TWSD;
			if (_numBytes < MAX_PACKET_LENGTH) {
				_data[_numBytes++] = data;
			} else {
				// XXX: Send NACK
			}
		}
	}
}

void TwoWireInit(uint8_t address, TwoWireDataReceivedCallback dataReceivedCallback,
	TwoWireDataRequestedCallback dataRequestedCallback)
{	
	_dataReceivedCallback = dataReceivedCallback;
	_dataRequestedCallback = dataRequestedCallback;
	
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = (1 << TWDIE) | (1 << TWASIE) | (1 << TWEN) | (1 << TWSIE) | (1 << TWSME);
	// TWAA? (Acknowledge Action)
	
	TWSA = (address << 1);
}
