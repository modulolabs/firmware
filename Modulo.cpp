/*
 * Modulo.cpp
 *
 * Created: 7/9/2014 10:03:45 AM
 *  Author: ekt
 */


#include "Modulo.h"
#include "Config.h"
#include "Random.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>
#include <util/crc16.h>

static const uint8_t _broadcastAddress = 9;
static volatile uint8_t _deviceAddress = 0;
static ModuloStatus _status;
static volatile uint8_t *_statusDDR = NULL;
static volatile uint8_t *_statusPort = NULL;
static volatile uint8_t _statusMask = 0;
static uint8_t _statusCounter = 0;
static uint16_t _statusBreathe = 0;
volatile uint16_t _deviceID = 0xFFFF;

#define BroadcastCommandGlobalReset 0
#define BroadcastCommandGetNextDeviceID 1
#define BroadcastCommandSetAddress 2
#define BroadcastCommandGetAddress 3
#define BroadcastCommandGetDeviceType 4
#define BroadcastCommandGetDeviceVersion 5
#define BroadcastCommandGetCompanyName 6
#define BroadcastCommandGetProductName 7
#define BroadcastCommandGetDocURL 8
#define BroadcastCommandGetDocURLContinued 9
#define BroadcastCommandGetInterrupt 10
#define BroadcastCommandSetStatusLED 11


uint16_t ModuloGetDeviceID() {
	// If _deviceID has been initialized, return it
	if (_deviceID != 0xFFFF) {
		return _deviceID;
	}
	
	// Load the device ID from EEPROM.
	_deviceID = eeprom_read_word(0);
	
	// If nothing was stored in the EEPROM, then generate a random device id.
	while (_deviceID == 0xFFFF) {
		uint32_t r = GenerateRandomNumber();
		_deviceID = r ^ (r >> 16); // xor the low and high words to conserve entropy
		eeprom_write_word(0, _deviceID);
	}
	
	return _deviceID;
}

static void _SetDeviceAddress(uint8_t address) {
    _deviceAddress = address;
	TWSA = (address << 1);
	ModuloSetStatus(ModuloStatusOff);
}


void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask)
{
	_statusDDR = statusDDR;
	_statusPort = statusPort;
	_statusMask = statusMask;
	
	if (_statusDDR and _statusMask) {
		*_statusDDR |= _statusMask;
	}
	
	// Ensure that we have a valid device id
	ModuloGetDeviceID();
	
#if defined(CPU_TINYX41)
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = (1 << TWDIE) | (1 << TWASIE) | (1 << TWEN) | (1 << TWSIE);
	
	// Also listen for message on the broadcast address
	TWSAM = (_broadcastAddress << 1)| 1;
#elif defined(CPU_TINYX8)
    TWCR = _BV(TWEN) | _BV(TWEA) | _BV(TWIE);
    TWAR = (MODULE_ADDRESS << 1) | _BV(TWGCE);
    TWAMR = 0xFF;
#endif

    ModuloReset();
	
	ModuloSetStatus(ModuloStatusBlinking);
}


void ModuloSetStatus(ModuloStatus status) {
	_status = status;
}


void ModuloUpdateStatusLED() {
	if (!_statusMask or !_statusPort or !_statusDDR) {
		return;
	}
	switch(_status) {
		case ModuloStatusOff:
			*_statusPort &= ~_statusMask;
			break;
		case ModuloStatusOn:
			*_statusPort |= _statusMask;
			break;
		case  ModuloStatusBlinking:
			_statusCounter = (_statusCounter+1) % 100;
			if (_statusCounter == 0) {
				*_statusPort ^= _statusMask;
			}
			break;
		case ModuloStatusBreathing:
			_statusCounter = (_statusCounter+1) % 10;
			if (_statusCounter == 0) {
				_statusBreathe += .15;
				if (_statusBreathe >= 20) {
					_statusBreathe = 0;
				}
			}
	
			uint8_t threshold = _statusBreathe <= 10 ? _statusBreathe : 20-_statusBreathe;
			if (_statusCounter < threshold) {
				*_statusPort |= _statusMask;
			} else {
				*_statusPort &= ~_statusMask;
			}
			break;
	}
}

uint8_t _GetNumBytes(ModuloDataType dataType) {
	switch(dataType) {
		case ModuloDataTypeNone:
			return 0;
		case ModuloDataTypeBool:
		case ModuloDataTypeUInt8:
		case ModuloDataTypeInt8:
		case ModuloDataTypeBitfield8:
			return 1;	
		case ModuloDataTypeUInt16:
		case ModuloDataTypeInt16:
		case ModuloDataTypeBitfield16:
			return 2;	
		case ModuloDataTypeUInt32:
		case ModuloDataTypeInt32:
		case ModuloDataTypeBitfield32:
		case ModuloDataTypeFloat:
			return 4;
		case ModuloDataTypeString:
		case ModuloDataTypeStream:
			return 255;
	}
	return 1;
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

ModuloWriteBuffer _writeBuffer;
ModuloReadBuffer _readBuffer;

static bool _shouldReplyToBroadcastRead = false;

static bool _ModuloWrite(const ModuloWriteBuffer &buffer) {
    _shouldReplyToBroadcastRead = false;

    if (buffer.GetAddress() == _broadcastAddress) {

        switch(buffer.GetCommand()) {
            case BroadcastCommandGlobalReset:
                if (buffer.GetSize() == 0) {
                    _SetDeviceAddress(0);
                    ModuloReset();
                }
                break;


            case BroadcastCommandGetNextDeviceID:
                if (buffer.GetSize() == 2 and
                    (buffer.Get<uint16_t>(0) <= ModuloGetDeviceID())) {
                    _shouldReplyToBroadcastRead = true;
                    return true;
                }
                break;

            case BroadcastCommandSetAddress:
                if (buffer.GetSize() == 3 and
                    buffer.Get<uint16_t>(0) == ModuloGetDeviceID()) {
                    _SetDeviceAddress(buffer.Get<uint8_t>(2));
                    return true;
                }
                break;

            // These commands all expect the device ID
            case BroadcastCommandGetAddress:
            case BroadcastCommandGetDeviceType:
            case BroadcastCommandGetDeviceVersion:
            case BroadcastCommandGetCompanyName:
            case BroadcastCommandGetProductName:
            case BroadcastCommandGetDocURL:
            case BroadcastCommandGetDocURLContinued:
                if (buffer.GetSize() == 2 and buffer.Get<uint16_t>(0) == ModuloGetDeviceID()) {
                        _shouldReplyToBroadcastRead = true;
                        return true;
                }
                break;
            case BroadcastCommandSetStatusLED:
                if (buffer.GetSize() == 3 and buffer.Get<uint16_t>(0) == ModuloGetDeviceID()) {
                    ModuloSetStatus((ModuloStatus)buffer.Get<uint8_t>(2));
                    return true;
                }
                break;
            case BroadcastCommandGetInterrupt:
                // XXX: not implemented
                return false;
                

        }
        return false;
    }

    return ModuloWrite(buffer);
}

static bool _ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *readBuffer) {
    if (writeBuffer.GetAddress() == _broadcastAddress) {
        if (!_shouldReplyToBroadcastRead) {
            return false;
        }
        switch(writeBuffer.GetCommand()) {
            case BroadcastCommandGetNextDeviceID:
                {
                    // We have to return the deviceID in big endian order, so that if a collision occurs
                    // the smallest value will win.
                    uint16_t deviceID = ModuloGetDeviceID();
                    deviceID = (deviceID << 8) | (deviceID >> 8);
                    readBuffer->AppendValue<uint16_t>(deviceID);
                    return true;
                }
            case BroadcastCommandGetAddress:
                {
                    uint8_t addr = _deviceAddress;
                    readBuffer->AppendValue<uint8_t>(addr);
                    return true;
                }
            case BroadcastCommandGetDeviceType:
                readBuffer->AppendString(ModuloDeviceType);
                return true;
            case BroadcastCommandGetDeviceVersion:
                readBuffer->AppendValue<uint16_t>(ModuloDeviceVersion);
                return true;
            case BroadcastCommandGetCompanyName:
                readBuffer->AppendString(ModuloCompanyName);
                return true;
            case BroadcastCommandGetProductName:
                readBuffer->AppendString(ModuloProductName);
                return true;
            case BroadcastCommandGetDocURL:
                readBuffer->AppendString(ModuloDocURL);
                return true;
            case BroadcastCommandGetDocURLContinued:
                readBuffer->AppendString(ModuloDocURL);
                return true;
            case BroadcastCommandGetInterrupt:
                // XXX: Not implemented
                return false;
        }
        return false;
    }


    return ModuloRead(command, writeBuffer, readBuffer);
}



// The interrupt service routine. Examine registers and dispatch to the handlers above.
#if defined(CPU_TINYX41)
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
                _readBuffer.Reset(address);
                //_readBuffer.AppendValue(42);
				bool retval = _ModuloRead(_writeBuffer.GetCommand(), _writeBuffer, &_readBuffer);
                _readBuffer.AppendValue(_readBuffer.ComputeCRC(address));
                _Acknowledge(retval /*ack*/, false /*complete*/);
			} else {
                _Acknowledge(true /*ack*/, false /*complete*/);
				_writeBuffer.Reset(address);
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
            bool ack = _readBuffer.GetNextByte(&data);
        	TWSD = data;
    		_Acknowledge(ack /*ack*/, !ack /*complete*/);
		} else {
        
			volatile uint8_t data = TWSD;
			_Acknowledge(true, false);

			// The first byte has the command in the upper 3 bytes
			// and the length in the lower 5 bytes.
			bool ack = _writeBuffer.Append(data);

            volatile bool isValid = _writeBuffer.IsValid();
            if (isValid) {
                _ModuloWrite(_writeBuffer);
            }

            //_Acknowledge(ack /*ack*/, !ack /*complete*/);
        }
	}

}
#endif

#if defined(CPU_TINYX8)

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
                _writeBuffer.Reset(address);
                _Acknowledge((data >> 1) == 6);
            }
            break;
        case TW_SLAVE_OWN_ADDR_RX_ACK: //  own address, data received. ack transmitted.
        case TW_SLAVE_GENERAL_RX_ACK:  // general call, data received. ack transmitted.
            {
        	    volatile uint8_t data = TWDR;
        			
        	    bool ack = _writeBuffer.Append(data);

        	    volatile bool isValid = _writeBuffer.IsValid();
        	    if (isValid) {
            	    ModuloWrite(_writeBuffer);
        	    }
        	    _Acknowledge(true);
            }
            break;
        case TW_SLAVE_OWN_ADDR_RX_NACK: // own address, data received. nack transmitted.
        case TW_SLAVE_GENERAL_RX_NACK:  // general call, data received. nack transmitted.
            // Must ack to continue matching slave address
            _Acknowledge(true);
            break;
        case TW_SLAVE_STOP: // Stop or repeated start received.
            // Must ack to continue matching slave address
            _Acknowledge(true);
            break;

         //
         // Slave Transmitter Mode
         //

        case TW_SLAVE_OWN_ADDR_R_ACK: // own address+r received. ack transmitted.
        case TW_SLAVE_ARB_LOST_R_ACK: // own address+r received. ack transmitted. (after arb lost)
            {
                uint8_t address = TWDR;
                _readBuffer.Reset(address);
		        ModuloRead(_writeBuffer.GetCommand(), _writeBuffer, &_readBuffer);
                _readBuffer.AppendValue(_readBuffer.ComputeCRC(address));
                
                if ((data >> 1) != 6) {
                    _Acknowledge(false);
                    break;
                }
                
            }
            // Don't break. Fall through to load the TWDR.
        case TW_SLAVE_DATA_TX_ACK: // Data transmitted, ack received.
            {
                uint8_t data = 0;
                bool ack = _readBuffer.GetNextByte(&data);
                TWDR = data;
                _Acknowledge(ack);
            }
            break;
        case TW_SLAVE_DATA_TX_NACK: // Data transmitted, nack received.
            // Must ack to continue matching slave address
            _Acknowledge(true);
            asm("nop");
            break;
        case TW_SLAVE_DATA_LAST_TX_ACK: // Last data byte transmitted, ack received
            // Must ack to continue matching slave address
            _Acknowledge(true);
            break;
        default:
            asm("nop");
            _Acknowledge(false);
            break;  
    }
}
#endif

ModuloWriteBuffer::ModuloWriteBuffer() {
}
	
void ModuloWriteBuffer::Reset(volatile uint8_t address) {
    _command = 0xFF;
    _length = 0;
    _expectedLength = 0xFF;
    _receivedCRC = -1;
    _address = address >> 1;
    _computedCRC = _crc8_ccitt_update(0, address);
}
	
// Append a value to the buffer
// Return false if there is not enough space or the CRC fails.
bool ModuloWriteBuffer::Append(volatile const uint8_t value) {
	if (_command == 0xFF) {
		// The first byte contains the command in the upper 3 bits
		// and the expected length in the lower 5 bits.
		_command = value;
		_computedCRC = _crc8_ccitt_update(_computedCRC, value);
		return true;
	}
    if (_expectedLength == 0xFF) {
        _expectedLength = value;
        _computedCRC =  _crc8_ccitt_update(_computedCRC, value);
        return true;
    }
	if (_length+1 >= MODULO_MAX_BUFFER_SIZE) {
		return false;
	}
    if (_length < _expectedLength) {
   	    _data[_length] = value;
	    _length++;
        _computedCRC = _crc8_ccitt_update(_computedCRC, value);
        return true;
    }
	if (_receivedCRC == -1) {
        _receivedCRC = value;
        return true;
        // return value == _computedCRC;
    }

	return false;
}

bool ModuloWriteBuffer::IsValid() {
    return (_length == _expectedLength and _computedCRC == _receivedCRC);
}
