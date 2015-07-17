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

static volatile uint8_t _deviceAddress = 0;


void TwoWireInit() {
#if defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny841__)
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = (1 << TWDIE) | (1 << TWASIE) | (1 << TWEN) | (1 << TWSIE);

	// Also listen for message on the broadcast address
	TWSAM = (moduloBroadcastAddress << 1)| 1;
#elif defined(CPU_TINYX8)
	TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);
	TWAR = (MODULE_ADDRESS << 1) | _BV(TWGCE);
	TWAMR = 0xFF;
#endif
}

void TwoWireSetDeviceAddress(uint8_t address) {
	_deviceAddress = address;
	TWSA = (address << 1);
	ModuloSetStatus(ModuloStatusOff);
}

uint8_t TwoWireGetDeviceAddress()
{
	return _deviceAddress;
}

static void _Acknowledge(bool ack, bool complete=false) {
#if defined(CPU_TINYX41)
	if (ack) {
		TWSCRB &= ~_BV(TWAA);
		} else {
		TWSCRB |= _BV(TWAA);
	}
	
	TWSCRB |= _BV(TWCMD1) | (complete ? 0 : _BV(TWCMD0));
#elif defined(CPU_TINYX8)
	if (ack) {
		TWCR |= _BV(TWEA);
		} else {
		TWCR &= ~_BV(TWEA);
	}
#endif
}


// The interrupt service routine. Examine registers and dispatch to the handlers above.

ISR(TWI_SLAVE_vect)
{
	bool dataInterruptFlag = (TWSSRA & (1 << TWDIF)); // Check whether the data interrupt flag is set
	bool isAddressOrStop = (TWSSRA & (1 << TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	bool isReadOperation = (TWSSRA & (1 << TWDIR));
	bool addressReceived = (TWSSRA & (1 << TWAS)); // Check if we received an address and not a stop
	
	
	//volatile bool clockHold = (TWSSRA & _BV(TWCH));
	//volatile bool receiveAck = (TWSSRA & _BV(TWRA));
	//volatile bool collision = (TWSSRA & _BV(TWC));
	//volatile bool busError = (TWSSRA & _BV(TWBE));

	if (isAddressOrStop) {

		if (addressReceived) {
			// The address is in the high 7 bits, the RD/WR bit is in the lsb
			uint8_t address = TWSD;

			if (isReadOperation) {
				moduloReadBuffer.Reset(address);
				//_readBuffer.AppendValue(42);
				bool retval = TwoWireReadCallback(moduloWriteBuffer.GetCommand(), moduloWriteBuffer, &moduloReadBuffer);
				moduloReadBuffer.AppendValue(moduloReadBuffer.ComputeCRC(address));
				_Acknowledge(retval /*ack*/, false /*complete*/);
			} else {
				_Acknowledge(true /*ack*/, false /*complete*/);
				moduloWriteBuffer.Reset(address);
			}
		} else {
			//if (!isReadOperation) {
			
			//}
			//TWSCRB |= 2;
			// XXX: Do we need to acknowledge stops?
			// XXX: Should we disable the stop interrupt?
			_Acknowledge(true /*ack*/, true /*complete*/);
		}
	}

	if (dataInterruptFlag) {
		if (isReadOperation) {
			uint8_t data = 0;
			bool ack = moduloReadBuffer.GetNextByte(&data);
			TWSD = data;
			_Acknowledge(ack /*ack*/, !ack /*complete*/);
		} else {
			
			volatile uint8_t data = TWSD;
			_Acknowledge(true, false);

			// The first byte has the command in the upper 3 bytes
			// and the length in the lower 5 bytes.
			bool ack = moduloWriteBuffer.Append(data);

			volatile bool isValid = moduloWriteBuffer.IsValid();
			if (isValid) {
				TwoWireWriteCallback(moduloWriteBuffer);
			}

			//_Acknowledge(ack /*ack*/, !ack /*complete*/);
		}
	}

}
#endif
