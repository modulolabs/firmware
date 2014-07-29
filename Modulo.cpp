/*
 * Modulo.cpp
 *
 * Created: 7/9/2014 10:03:45 AM
 *  Author: ekt
 */
#if 1

#include "Modulo.h"
#include "Setup.h"
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>

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


volatile ModuloReadValueCallback _readValueCallback = 0;
volatile ModuloWriteValueCallback _writeValueCallback = 0;

volatile ModuloBeginWriteArrayCallback _beginWriteArrayCallback = 0;
volatile ModuloWriteArrayDataCallback _writeArrayDataCallback = 0;
volatile ModuloEndWriteArrayCallback _endWriteArrayCallback = 0;

volatile ModuloBeginReadArrayCallback _beginReadArrayCallback = 0;
volatile ModuloReadArrayDataCallback _readArrayDataCallback = 0;
volatile ModuloEndReadArrayCallback _endReadArrayCallback = 0;

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
	
	// Enable Data Interrupt, Address/Stop Interrupt, Two-Wire Interface, Stop Interrpt
	TWSCRA = (1 << TWDIE) | (1 << TWASIE) | (1 << TWEN) | (1 << TWSIE);

	// TWAA? (Acknowledge Action)
	TWSA = (MODULE_ADDRESS << 1);
}

void ModuloSetStatus(ModuloStatus status) {
	_status = status;
}


void ModuloUpdateStatusLED() {
	if (!_statusMask or !_statusPort or !_statusDDR == 0) {
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
			return ModuloDataTypeUInt8;
		case MODULO_REGISTER_DEVICE_ID:
		case MODULO_REGISTER_SERIAL_NUMBER:
		case MODULO_REGISTER_LOT_NUMBER:
			return ModuloDataTypeUInt32;
			
		case MODULO_REGISTER_FUNCTION_TYPES:
			return ModuloDataTypeString;
		
		case MODULO_REGISTER_STATUS_LED:
			return ModuloDataTypeBool;
			

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

static void _WriteValue(uint8_t function, ModuloDataType dataType, const ModuloBuffer &buffer) {
	if (function == MODULO_REGISTER_DEVICE_ID) {
		// XXX: Set the device ID
		return;
	}
	if (function == MODULO_REGISTER_STATUS_LED) {
		_status = (ModuloStatus)buffer.Get<uint8_t>();
	}
	
	if (_writeValueCallback) {
		(*_writeValueCallback)(function, buffer);
	}
}

uint8_t _ReadDeviceSignatureImprintTable(uint8_t address)
{
	uint8_t mask = (1<<RSIG)|(1<<SPMEN);
    uint8_t result=0;
	/*
    asm
    (
		"out %[spmcsr], %[mask]\n\t"
         "lpm %[result], Z\n\t"
        : [result] "=r" (result)
        : "z" (address),
		  [mask] "a" (mask), 
		  [spmcsr] "I" (_SFR_IO_ADDR(SPMCSR))
		: "r16"
    );
	*/
	asm(
	"out %[spmcsr], %[mask]\n\t"
	"lpm %[result], Z\n\t"
	: [result] "=r" (result)
	: [address] "z" (address),
	  [mask] "a" (mask),
	  [spmcsr] "M" (_SFR_IO_ADDR(SPMCSR))
	);
    return result;
}

uint8_t ReadSignatureByte(uint16_t Address) { 
  //NVM_CMD = NVM_CMD_READ_CALIB_ROW_gc; 
  uint8_t Result;
  SPMCSR = (1<<RSIG)|(1<<SPMEN);
  __asm__ ("lpm %0, Z\n" : "=r" (Result) : "z" (Address)); 
//  __asm__ ("lpm \n  mov %0, r0 \n" : "=r" (Result) : "z" (Address) : "r0"); 
  //NVM_CMD = NVM_CMD_NO_OPERATION_gc; 
  return Result; 
} 



uint32_t ModuloGetSerialNumber() {
	return _ReadDeviceSignatureImprintTable(0); // Byte address of the lot number in the device signature table
}

uint32_t ModuloGetLotNumber() {
	return _ReadDeviceSignatureImprintTable(1);//_ReadDeviceSignatureImprintTable(1); // Byte address of the lot number in the device signature table
}


static void _ReadValue(uint8_t function, ModuloDataType dataType, ModuloBuffer *buffer) {
	
	if (function >= 200) {
		switch(function) {
			case MODULO_REGISTER_DEVICE_VERSION:
				buffer->Set(ModuloDeviceVersion);
				break;
			case MODULO_REGISTER_STATUS_LED:
				buffer->Set(_status);
				break;
 			case MODULO_REGISTER_LOT_NUMBER:
				buffer->Set(ModuloGetLotNumber());
				break;
			case MODULO_REGISTER_SERIAL_NUMBER:
				buffer->Set(ModuloGetSerialNumber());
				break;
		}
		return;
	}
	
	if (_readValueCallback) {
		(*_readValueCallback)(function, buffer);
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
static void OnAddressReceived(bool isReadOperation) {
	currentByte = 0;
	if (!isReadOperation) {
		currentFunction = MODULO_INVALID_REGISTER;
	}
}

static void OnWriteByte(uint8_t data) {
	
	if (currentFunction == MODULO_INVALID_REGISTER) {
		SetCurrentFunction(data);
	} else if (currentDataType == ModuloDataTypeString) {
		if (arrayLength < 0) {
			arrayLength = data;
			_BeginWriteArray(currentFunction, arrayLength);
		} else {
			//_WriteArrayData(currentFunction, currentByte++, data);
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

static uint8_t OnReadByte() {
	if (currentFunction == MODULO_INVALID_REGISTER) {
		// Error. Read with no command set.
		return 0;
	}
	
	if (currentDataType == ModuloDataTypeString) {
		if (arrayLength < 0) {
			arrayLength = _BeginReadArray(currentFunction);
			currentByte = 0;
			return arrayLength;
		}
		if (currentByte < arrayLength) {
			return _ReadArrayData(currentFunction, currentByte++);
		}
		return 0;
	} else {
		if (currentByte == 0) {
			_ReadValue(currentFunction, currentDataType, &buffer);
		}
	
		uint8_t retval = 0;
		
		if (currentByte < _GetNumBytes(currentDataType)) {
			retval = buffer.data[currentByte];
			currentByte++;
		}
		
		//if (currentByte == _GetNumBytes(currentDataType)) {
		//	SetCurrentFunction(currentFunction+1);
		//}
	
		return retval;
	}
}

static void OnStop() {
	//if (currentDataType == ModuloDataTypeString and currentByte == arrayLength) {
	//	_EndWriteArray(currentFunction);
	//}
}


// The interrupt service routine. Examine registers and dispatch to the handlers above.

ISR(TWI_SLAVE_vect)
{
	bool dataInterruptFlag = (TWSSRA & (1 << TWDIF)); // Check whether the data interrupt flag is set
	bool isAddressOrStop = (TWSSRA & (1 << TWASIF)); // Get the TWI Address/Stop Interrupt Flag
	bool clockHold = (TWSSRA & _BV(TWCH));
	bool receiveAck = (TWSSRA & _BV(TWRA));
	bool collision = (TWSSRA & _BV(TWC));
	bool busError = (TWSSRA & _BV(TWBE));
	bool isReadOperation = (TWSSRA & (1 << TWDIR));
	bool addressReceived = (TWSSRA & (1 << TWAS)); // Check if we received an address and not a stop
	
	if (isAddressOrStop) {
		//TWSCRB |= (1 << TWCMD0) | (1 << TWCMD1);
		//TWSSRA &= ~(1 << TWASIF);
		

		if (addressReceived) {
			TWSCRB &= ~_BV(TWAA);
			TWSCRB |= 3;
			
			OnAddressReceived(isReadOperation);
		} else {
			TWSCRB |= 2;
			
			OnStop();
		}
	}

	if (dataInterruptFlag) {
		if (isReadOperation) {
			uint8_t data = OnReadByte();
			TWSD = data;
			TWSCRB |= 3;
			return;
		} else {
			uint8_t data = TWSD;
			OnWriteByte(data);
			TWSCRB |= 3;
			return;
		}
	}

}

#endif
