/*
 * Modulo.cpp
 *
 * Created: 7/9/2014 10:03:45 AM
 *  Author: ekt
 */
#if 1

#include "Modulo.h"
#include "Config.h"
#include "Random.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include <avr/boot.h>
#include <avr/eeprom.h>

static uint8_t currentFunction = 0;
static ModuloDataType currentDataType = ModuloDataTypeNone;
static uint8_t currentByte = 0;
static ModuloBuffer buffer;
static int16_t arrayLength = 0;
static ModuloStatus _status;
static volatile uint8_t *_statusDDR = &DDRA;
static volatile uint8_t *_statusPort = &PORTA;
static volatile uint8_t _statusMask = _BV(1);
static uint8_t _statusCounter = 0;
static uint16_t _statusBreathe = 0;
static volatile uint8_t _deviceAddress = 0;

volatile ModuloReadValueCallback _readValueCallback = 0;
volatile ModuloWriteValueCallback _writeValueCallback = 0;

volatile ModuloBeginWriteArrayCallback _beginWriteArrayCallback = 0;
volatile ModuloWriteArrayDataCallback _writeArrayDataCallback = 0;
volatile ModuloEndWriteArrayCallback _endWriteArrayCallback = 0;

volatile ModuloBeginReadArrayCallback _beginReadArrayCallback = 0;
volatile ModuloReadArrayDataCallback _readArrayDataCallback = 0;
volatile ModuloEndReadArrayCallback _endReadArrayCallback = 0;

volatile uint16_t _deviceID = 0xFFFF;

uint16_t ModuloGetDeviceID() {
	// If _deviceID has been initialized, return it
	if (_deviceID != 0xFFFF) {
		return _deviceID;
	}
	
	// Load the device ID from EEPROM.
	_deviceID = eeprom_read_word(0);
	
	// If nothing was stored in the EEPROM, then
	// generate a random device id.
	while (_deviceID == 0xFFFF) {
		uint32_t r = GenerateRandomNumber();
		_deviceID = r ^ (r >> 16); // xor the low and high words to conserve entropy
		eeprom_write_word(0, _deviceID);
	}
	
	return _deviceID;
}

void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask,
	ModuloReadValueCallback readValue,
	ModuloWriteValueCallback writeValue,
	ModuloBeginWriteArrayCallback beginWriteArray,
	ModuloWriteArrayDataCallback writeArrayData,
	ModuloEndWriteArrayCallback endWriteArray,
	ModuloBeginReadArrayCallback beginReadArray,
	ModuloReadArrayDataCallback readArrayData,
	ModuloEndReadArrayCallback endReadArray)
{
	_statusDDR = statusDDR;
	_statusPort = statusPort;
	_statusMask = statusMask;
	
	_readValueCallback = readValue;
	_writeValueCallback = writeValue;
	_beginWriteArrayCallback = beginWriteArray;
	_writeArrayDataCallback = writeArrayData;
	_endWriteArrayCallback = endWriteArray;
	_beginReadArrayCallback = beginReadArray;
	_readArrayDataCallback = readArrayData;
	_endReadArrayCallback = endReadArray;
	
	if (_statusDDR and _statusMask) {
		*_statusDDR |= _statusMask;
	}
	
	// Ensure that we have a valid device id
	ModuloGetDeviceID();
	
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = (1 << TWDIE) | (1 << TWASIE) | (1 << TWEN) | (1 << TWSIE);

	// TWAA? (Acknowledge Action)
	TWSA = (MODULE_ADDRESS << 1);
	
	// Also listen for message on address 97, which is the smbus default address
	uint8_t smbusDefaultAddress = 97;
	TWSAM = (smbusDefaultAddress << 1) | 1;
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

ModuloDataType _GetDataType(uint8_t function) {
	// Linear search, since we don't store the length of ModuloDataTypes
	for (int i=0; i < 255; i++) {
		if (ModuloDataTypes[i] == ModuloDataTypeNone) {
			break;
		}
		if (i == function) {
			return ModuloDataTypes[i];
		}
	}
	
	switch(function) {
		case MODULO_REGISTER_COMPANY_NAME:
		case MODULO_REGISTER_DEVICE_NAME:
		case MODULO_REGISTER_DOC_URL:
		case MODULO_REGISTER_FUNCTION_NAMES:
			return ModuloDataTypeString;

		case MODULO_REGISTER_DEVICE_VERSION:
		case MODULO_REGISTER_ASSIGN_ADDRESS:
			return ModuloDataTypeUInt8;
		case MODULO_REGISTER_DEVICE_ID:
		case MODULO_REGISTER_SERIAL_NUMBER:
		case MODULO_REGISTER_LOT_NUMBER:
			return ModuloDataTypeUInt32;
			
		case MODULO_REGISTER_FUNCTION_TYPES:
			return ModuloDataTypeString;
		
		case MODULO_REGISTER_STATUS_LED:
		case MODULO_REGISTER_START_ENUMERATION:
			return ModuloDataTypeBool;
		case MODULO_REGISTER_ENUMERATE_NEXT_DEVICE:
		case MODULO_REGISTER_ASSIGN_DEVICE_ID:
			return ModuloDataTypeUInt16;
	}
	
	return ModuloDataTypeNone;
}

static void SetCurrentFunction(uint8_t f) {
	currentFunction = f;
	currentDataType = _GetDataType(f);
	arrayLength = -1;
	currentByte = 0;
	buffer.Reset();
}



void _StartEnumeration() {
	_deviceAddress = 0;

}

static int16_t _deviceIDToAssign;

void _AssignAddress(uint8_t address) {
	if (_deviceIDToAssign == MODULE_ADDRESS) {
		_deviceAddress = address;
	}
}

static void _WriteValue(uint8_t function, ModuloDataType dataType, const ModuloBuffer &buffer) {
	switch (function) {
		case MODULO_REGISTER_DEVICE_ID:
			// XXX: Set the device ID
			return;
		case MODULO_REGISTER_STATUS_LED:
			_status = (ModuloStatus)buffer.Get<uint8_t>();
			return;
		case MODULO_REGISTER_START_ENUMERATION:
			_StartEnumeration();
			return;
		case MODULO_REGISTER_ASSIGN_DEVICE_ID:
			_deviceIDToAssign = buffer.Get<uint16_t>();
			asm("nop");
			return;
		case MODULO_REGISTER_ASSIGN_ADDRESS:
			_AssignAddress(buffer.Get<uint8_t>());
			return;
	}
	
	if (_writeValueCallback) {
		(*_writeValueCallback)(function, buffer);
	}
}

#define SIGRD 5
uint8_t _ReadSignatureByte(uint8_t addr)
{
	return (__extension__({ \
		uint8_t __result; \
		__asm__ __volatile__ \
		( \
		"sts %1, %2\n\t" \
		"lpm %0, Z" "\n\t" \
		: "=r" (__result) \
		: "i" (_SFR_MEM_ADDR(__SPM_REG)), \
		"r" ((uint8_t)(__BOOT_SIGROW_READ)), \
		"z" ((uint16_t)(addr)) \
		); \
		__result; \
	}));
#if 0
	
	uint8_t mask = (1<<RSIG)|(1<<SPMEN);
    uint8_t result=0;
	asm(
	"out %[spmcsr], %[mask]\n\t"
	"lpm %[result], Z\n\t"
	: [result] "=r" (result)
	: [address] "z" (address),
	  [mask] "a" (mask),
	  [spmcsr] "M" (_SFR_IO_ADDR(SPMCSR))
	);
    return result;
#endif
}


uint32_t ModuloGetSerialNumber() {
	volatile unsigned char sigTable[48];
	for (int i=0; i < 48; i++) {
		sigTable[i] = _ReadSignatureByte(i);
	}
	
	volatile uint8_t osctcal0a = OSCTCAL0A;
	volatile uint8_t osctcal0b = OSCTCAL0B;
	volatile uint8_t osctcal0 = OSCCAL0;
	volatile uint8_t osctcal1 = OSCCAL1;

	volatile uint8_t y = _ReadSignatureByte(0x2C); // Byte address of the lot number in the device signature table
	asm("nop");
	return y;
}

uint32_t ModuloGetLotNumber() {
	volatile uint8_t x = OSCTCAL0B;
	volatile uint8_t y = _ReadSignatureByte(0x2D); // Byte address of the lot number in the device signature table
	return x;
}


static bool _ReadValue(uint8_t function, ModuloDataType dataType, ModuloBuffer *buffer) {
	
	if (function >= 200) {
		switch(function) {
			case MODULO_REGISTER_DEVICE_VERSION:
				buffer->Set(ModuloDeviceVersion);
				return true;
			case MODULO_REGISTER_STATUS_LED:
				buffer->Set(_status);
				return true;
 			case MODULO_REGISTER_LOT_NUMBER:
				buffer->Set(ModuloGetLotNumber());
				return true;
			case MODULO_REGISTER_SERIAL_NUMBER:
				buffer->Set(ModuloGetSerialNumber());
				return true;
			case MODULO_REGISTER_ENUMERATE_NEXT_DEVICE:
				if (_deviceAddress != 0) {
					return false;
				}
				buffer->Set((uint16_t)MODULE_ADDRESS);
				return true;
		}
		return false;
	}
	
	if (_readValueCallback) {
		(*_readValueCallback)(function, buffer);
		return true;
	}
}


static void _BeginWriteArray(uint8_t functionID, uint8_t arrayLength) {
	if (_beginWriteArrayCallback) {
		(*_beginWriteArrayCallback)(functionID, arrayLength);
	}
}

static void _WriteArrayData(uint8_t functionID, uint8_t index, uint8_t byte) {
	if (_writeArrayDataCallback) {
		(*_writeArrayDataCallback)(functionID, index, byte);
	}
}

static void _EndWriteArray(uint8_t functionID) {
	if (_endWriteArrayCallback) {
		(*_endWriteArrayCallback)(functionID);
	}
}

static uint8_t _BeginReadArray(uint8_t functionID) {
	switch(functionID) {
		case MODULO_REGISTER_COMPANY_NAME:
			return strlen(ModuloCompanyName);
		case MODULO_REGISTER_DEVICE_NAME:
			return strlen(ModuloDeviceName);
		case MODULO_REGISTER_DOC_URL:
			return strlen(ModuloDocURL);
		case MODULO_REGISTER_FUNCTION_NAMES:
			return strlen(ModuloFunctionNames);
		case MODULO_REGISTER_FUNCTION_TYPES:
			return strlen((char*)ModuloDataTypes);
	}

	if (_beginReadArrayCallback) {
		return (*_beginReadArrayCallback)(functionID);
	}
	return 0;
}

static uint8_t _ReadArrayData(uint8_t functionID, uint8_t index) {
	switch(functionID) {
		case MODULO_REGISTER_DEVICE_NAME:
			return ModuloDeviceName[index];
		case MODULO_REGISTER_COMPANY_NAME:
			return ModuloCompanyName[index];
		case MODULO_REGISTER_DOC_URL:
			return ModuloDocURL[index];
		case MODULO_REGISTER_FUNCTION_NAMES:
			return ModuloFunctionNames[index];
		case MODULO_REGISTER_FUNCTION_TYPES:
			return ModuloDataTypes[index];
	}
	
	if (_readArrayDataCallback) {
		return (*_readArrayDataCallback)(functionID, index);
	}
	return 'X';
}

// Handlers for specific events. These are called directly from the interrupt service routine.
static bool OnAddressReceived(bool isReadOperation) {
	currentByte = 0;
	if (!isReadOperation) {
		currentFunction = MODULO_INVALID_REGISTER;
	}
	if (isReadOperation and currentFunction == MODULO_REGISTER_ENUMERATE_NEXT_DEVICE and _deviceAddress != 0) {
		return false;
	}
	return true;
}

static void OnWriteByte(uint8_t data) {
	
	if (currentFunction == MODULO_INVALID_REGISTER) {
		SetCurrentFunction(data);
	} else if (currentDataType == ModuloDataTypeString) {
		if (arrayLength < 0) {
			arrayLength = data;
			_BeginWriteArray(currentFunction, arrayLength);
		} else {
			_WriteArrayData(currentFunction, currentByte++, data);
		}
		if(currentByte == arrayLength) {
			_EndWriteArray(currentFunction);
			SetCurrentFunction(currentFunction+1);
		}
	} else {
		buffer.data[currentByte] = data;
		currentByte++;
		if (currentByte == _GetNumBytes(currentDataType)) {
			_WriteValue(currentFunction, currentDataType, buffer);
			SetCurrentFunction(currentFunction+1);
		}
	}
}

bool OnReadByte(uint8_t *retval) {
	if (currentFunction == MODULO_INVALID_REGISTER) {
		// Error. Read with no command set.
		return 0;
	}
	
	if (currentDataType == ModuloDataTypeString) {
		if (arrayLength < 0) {
			arrayLength = _BeginReadArray(currentFunction);
			currentByte = 0;
			*retval = arrayLength;
			return true;
		}
		if (currentByte < arrayLength) {
			*retval = _ReadArrayData(currentFunction, currentByte++);
			return true;
		}
		return false;
	} else {
		if (currentByte == 0) {
			if (!_ReadValue(currentFunction, currentDataType, &buffer)) {
				return false;
			}
		}
		
		if (currentByte < _GetNumBytes(currentDataType)) {
			*retval = buffer.data[currentByte];
			currentByte++;
			return true;
		}
		
		//if (currentByte == _GetNumBytes(currentDataType)) {
		//	SetCurrentFunction(currentFunction+1);
		//}
	
		return false;
	}
}

static void OnStop() {
	//if (currentDataType == ModuloDataTypeString and currentByte == arrayLength) {
	//	_EndWriteArray(currentFunction);
	//}
}


static void _Acknowledge(bool ack, bool complete) {
	if (ack) {
		TWSCRB &= ~_BV(TWAA);
	} else {
		TWSCRB |= _BV(TWAA);
	}
	
	TWSCRB |= _BV(1) | !complete;
}

// The interrupt service routine. Examine registers and dispatch to the handlers above.

ISR(TWI_SLAVE_vect)
{
	
	volatile bool dataInterruptFlag = (TWSSRA & (1 << TWDIF)); // Check whether the data interrupt flag is set
	volatile bool isAddressOrStop = (TWSSRA & (1 << TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	volatile bool clockHold = (TWSSRA & _BV(TWCH));
	volatile bool receiveAck = (TWSSRA & _BV(TWRA));
	volatile bool collision = (TWSSRA & _BV(TWC));
	volatile bool busError = (TWSSRA & _BV(TWBE));
	volatile bool isReadOperation = (TWSSRA & (1 << TWDIR));
	volatile bool addressReceived = (TWSSRA & (1 << TWAS)); // Check if we received an address and not a stop
	
	if (isAddressOrStop) {
		//TWSCRB |= (1 << TWCMD0) | (1 << TWCMD1);
		//TWSSRA &= ~(1 << TWASIF);

		if (addressReceived) {
			
			//TWSCRB &= ~_BV(TWAA);
			//TWSCRB |= 3;
			
			bool ack = OnAddressReceived(isReadOperation);
			
			_Acknowledge(ack /*ack*/, !ack /*complete*/);
		} else {
			//TWSCRB |= 2;
			_Acknowledge(true /*ack*/, true /*complete*/);
			OnStop();
		}
	}

	if (dataInterruptFlag) {
		if (isReadOperation) {
			uint8_t data = 0;
			bool ack = OnReadByte(&data);
			if (ack) {
				TWSD = data;
			} else {
				asm("nop");
			}
			_Acknowledge(ack /*ack*/, !ack /*complete*/);
			//TWSCRB |= 3;
		} else {
			uint8_t data = TWSD;
			OnWriteByte(data);
			
			_Acknowledge(true /*ack*/, false /*complete*/);
			//TWSCRB |= 3;
		}
	}

}

#endif
