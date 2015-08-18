/*
 * TwoWireTiny48.cpp
 *
 * Created: 7/17/2015 10:02:07 AM
 *  Author: ekt
 */ 

#if defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny88__)

#include "Modulo.h"
#include "Buffer.h"
#include "TwoWire.h"

static volatile uint8_t _deviceAddress = 0;

static volatile uint8_t twiTxAddress = 0;
static volatile uint8_t twiTxBufferSize = 0;
static volatile uint8_t twiTxBufferPos = 0;
static volatile uint8_t twiTxBuffer[TWO_WIRE_MAX_BUFFER_SIZE];

static volatile uint8_t twiRxAddress = 0;
static volatile uint8_t twiRxBufferSize = 0;
//static volatile uint8_t twiRxBuffer[TWO_WIRE_MAX_BUFFER_SIZE];
static volatile bool twiRxComplete = false;

extern uint8_t _end;
extern uint8_t _stack;

void TwoWireInit() {
	
	TWAR =  _BV(TWGCE);
	TWAMR = 0xFF;
	TWBR = 0;
	TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);
	TWCR &= ~(_BV(TWSTA) | _BV(TWSTO));
	sei();
}

void TwoWireSetAddress(uint8_t address) {
	_deviceAddress = address;
	TWAR = (address << 1) | _BV(TWGCE);
	ModuloSetStatus(ModuloStatusOff);
}

uint8_t TwoWireGetAddress()
{
	return _deviceAddress;
}


bool TwoWireTransmitBegin(uint8_t address) {

	// Wait for the last transmit to complete
	while (twiTxBufferSize != 0) {
		/*
		if (i++ > 1000) {
			TWCR = 0;
			TWCR = _BV(TWIE) | _BV(TWSTA) | _BV(TWEN) | _BV(TWEA);
			i = 0;
		}
		*/
	}

	twiTxBufferPos = 0;
	twiTxBufferSize = 0;
	twiTxAddress = address;
	return true;
}

bool TwoWireTransmitUInt8(uint8_t d) {
	if (twiTxBufferSize < TWO_WIRE_MAX_BUFFER_SIZE) {
		twiTxBuffer[twiTxBufferSize++] = d;
		return true;
	}
	return false;
}

bool TwoWireTransmitUInt16(uint16_t d) {
	return (TwoWireTransmitUInt8(d & 0xFF) and
			TwoWireTransmitUInt8(d >> 8));
}

bool TwoWireTransmitString(const char *s) {
	uint8_t len = strlen(s)+1;
	for (int i=0; i < len; i++) {
		TwoWireTransmitUInt8(s[i]);
	}
	return true;
}

bool TwoWireTransmitCRC() {
	uint16_t crc = _crc16_update(0xFFFF, twiTxAddress);
	for (int i=0; i < twiTxBufferSize; i++) {
		crc = _crc16_update(crc, twiTxBuffer[i]);
	}
	TwoWireTransmitUInt16(crc);
	return true;
}

bool TwoWireTransmitEnd() {
	// Wait for bus to become idle and send start condition.
	//    Enable TwoWire (TWEN=1)
	//    Clear interrupt flag (TWINT=1)
	//    Send START (TWSTA=1)
	TWCR = _BV(TWINT) | _BV(TWEA) | _BV(TWSTA) | _BV(TWEN) | _BV(TWIE);
	return true;
}

static void twiRespond(bool ack=true, bool start=false, bool stop=false) {
	TWCR = (ack << TWEA) | (start << TWSTA) | (stop << TWSTO) |
		 (1 << TWINT) | (1 << TWIE) | (1 << TWEN);
}



#define TW_BUS_ERROR 0x00              // A bus error has occurred due to an illegal start or stop
#define TW_BUS_UNDEFINED 0xF8          // No relevant state information available

// Master
#define TW_MASTER_START 0x08           // start transmitted
#define TW_MASTER_REPEATED_START 0x10  // repeated start transmitted
#define TW_MASTER_ARB_LOST 0x38        // arbitration lost

// Master Transmitter
#define TW_MASTER_ADDR_W_ACK  0x18     // address+w transmitted, ack received
#define TW_MASTER_ADDR_W_NACK 0x20     // address+w transmitted, nack received
#define TW_MASTER_DATA_TX_ACK 0x28     // data transmitted, ack received
#define TW_MASTER_DATA_TX_NACK 0x30    // data transmitted, nack received

// Master Receiver
#define TW_MASTER_ADDR_R_ACK 0x40      // address+r transmitted, ack received
#define TW_MASTER_ADDR_R_NACK 0x48     // address+r transmitted, nack received
#define TW_MASTER_DATA_RX_ACK 0x50     // data received, ack transmitted
#define TW_MASTER_DATA_RX_NACK 0x58    // data received, nack transmitted

// Slave Receiver
#define TW_SLAVE_OWN_ADDR_W_ACK 0x60   // own address received, ack transmitted
#define TW_SLAVE_ARB_LOST_W_ACK 0x68   // arbitration lost, own address+w received
#define TW_SLAVE_GENERAL_W_ACK 0x70    // general call received, ack transmitted
#define TW_SLAVE_ARB_LOST_GNERAL 0x78  // arbitration lost, general call received
#define TW_SLAVE_OWN_ADDR_RX_ACK 0x80  // data received at own address, ack transmitted
#define TW_SLAVE_OWN_ADDR_RX_NACK 0x88 // data received at own address, nack transmitted
#define TW_SLAVE_GENERAL_RX_ACK 0x90   // data received at general call, ack transmitted
#define TW_SLAVE_GENERAL_RX_NACK 0x98  // data received at general call, nack transmitted
#define TW_SLAVE_STOP 0xA0             // stop or repeated start received

// Slave Transmitter
#define TW_SLAVE_OWN_ADDR_R_ACK 0xA8   // own address+r received, ack transmitted
#define TW_SLAVE_ARB_LOST_R_ACK 0xB0   // own address+r received after arb lost, ack transmitted
#define TW_SLAVE_DATA_TX_ACK 0xB8      // data byte transmitted, ack received
#define TW_SLAVE_DATA_TX_NACK 0xC0     // data byte transmitted, nack received
#define TW_SLAVE_DATA_LAST_TX_ACK 0xC8 // last data bye transmitted, ack received

ISR(TWI_vect)
{
	volatile uint8_t status = TWSR;
	volatile uint8_t data = TWDR;

	switch(status) {
		
		//
		// Slave Receiver Mode
		//
		
	case TW_SLAVE_OWN_ADDR_W_ACK:  // own address+w received.  ack transmitted.
	case TW_SLAVE_ARB_LOST_W_ACK:  // own address+w received.  ack transmitted. (after arb lost)
	case TW_SLAVE_GENERAL_W_ACK:   // general call+w received. ack transmitted.
	case TW_SLAVE_ARB_LOST_GNERAL: // general call+w received. ack transmitted. (after arb lost)
		{
			volatile uint8_t address = TWDR;
			if (address >> 1 == _deviceAddress or
				address >> 1 == moduloBroadcastAddress) {
				moduloWriteBuffer.Reset(address);
				twiRespond(true);
			} else {
				twiRespond(false);
			}
		}
		break;
	case TW_SLAVE_OWN_ADDR_RX_ACK: //  own address, data received. ack transmitted.
	case TW_SLAVE_GENERAL_RX_ACK:  // general call, data received. ack transmitted.
		{
			volatile uint8_t data = TWDR;
			bool ack = moduloWriteBuffer.Append(data);
			twiRespond(true);
		}
		break;
	case TW_SLAVE_OWN_ADDR_RX_NACK: // own address, data received. nack transmitted.
	case TW_SLAVE_GENERAL_RX_NACK:  // general call, data received. nack transmitted.
		// Must ack to continue matching slave address
		twiRespond(true);
		break;
	case TW_SLAVE_STOP: // Stop or repeated start received.
		// Must ack to continue matching slave address
		twiRespond(true);
		if ( moduloWriteBuffer.IsValid()) {
			TwoWireWriteCallback(moduloWriteBuffer);
		}
		break;

		//
		// Slave Transmitter Mode
		//

	case TW_SLAVE_OWN_ADDR_R_ACK: // own address+r received. ack transmitted.
	case TW_SLAVE_ARB_LOST_R_ACK: // own address+r received. ack transmitted. (after arb lost)
		{
			uint8_t address = TWDR;
			moduloReadBuffer.Reset(address);
			TwoWireReadCallback(moduloWriteBuffer.GetCommand(), moduloWriteBuffer, &moduloReadBuffer);
			moduloReadBuffer.AppendValue(moduloReadBuffer.ComputeCRC(address));
			
			if ((data >> 1) != moduloBroadcastAddress and
				(data >> 1) != _deviceAddress) {
				twiRespond(false);
				break;
			}
			
		}
		// Don't break. Fall through to load the TWDR.
	case TW_SLAVE_DATA_TX_ACK: // Data transmitted, ack received.
		{
			uint8_t data = 0;
			bool ack = moduloReadBuffer.GetNextByte(&data);
			TWDR = data;
			twiRespond(ack);
		}
		break;
	case TW_SLAVE_DATA_TX_NACK: // Data transmitted, nack received.
		// Must ack to continue matching slave address
		twiRespond(true);
		asm("nop");
		break;
	case TW_SLAVE_DATA_LAST_TX_ACK: // Last data byte transmitted, ack received
		// Must ack to continue matching slave address
		twiRespond(true);
		break;
		
		
	//
	// Master Transmitter Mode
	//
	case TW_MASTER_START:
	case TW_MASTER_REPEATED_START:

		// Start or repeated start condition has been sent. Transmit SLA+W
		TWDR = (twiTxAddress << 1);
		
		twiRespond(true);
		break;
		
	case TW_MASTER_ADDR_W_ACK:
	case TW_MASTER_DATA_TX_ACK:
		// SLA+W or data byte was acknowledged.
		if (twiTxBufferPos < twiTxBufferSize) {
			// Transmit the next byte
			TWDR = twiTxBuffer[twiTxBufferPos++];
			
			// Clear the interrupt to continue
			twiRespond(true);
		} else {
			// No more data to send. Send stop condition.
			twiRespond(true /*ack*/, false /*start*/, true /*stop*/);
			
			// Reset the transmit buffer.
			twiTxBufferSize = 0;
		}
		
		break;
	
	case TW_MASTER_ADDR_W_NACK:
	case TW_MASTER_DATA_TX_NACK:
		// SLA+W or data byte was not acknowledged. Send stop condition.
		twiRespond(true /*ack*/, false /*start*/, true /*stop*/);
		
		// Reset the transmit buffer.
		twiTxBufferSize = 0;
		break;
	
	case TW_MASTER_ARB_LOST:
		// Arbitration was lost. Slave mode entered.
		asm("nop");
		twiRespond();
		break;
		
	case TW_BUS_UNDEFINED:
		asm("nop");
		break;
		
	default :
		asm("nop");
	
		twiRespond(false);
		twiTxBufferSize = 0;
		twiRxComplete = true;
		break;
	}
}
#endif
