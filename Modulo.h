/*
 * Modulo.h
 *
 * Created: 7/9/2014 10:03:26 AM
 *  Author: ekt
 */ 


#ifndef MODULO_H_
#define MODULO_H_

#include <inttypes.h>
#include <avr/interrupt.h>
#include <util/crc16.h>
#include <util/atomic.h>
#include <math.h>
#include <string.h>

class ModuloWriteBuffer;
class ModuloReadBuffer;

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &, ModuloReadBuffer *buffer);
bool ModuloWrite(const ModuloWriteBuffer &buffer);
void ModuloReset();

extern const char *ModuloDeviceType;
extern const uint16_t ModuloDeviceVersion;
extern const char *ModuloCompanyName;
extern const char *ModuloProductName;
extern const char *ModuloDocURL;

enum ModuloDataType {
	ModuloDataTypeNone,
	ModuloDataTypeBool,
	ModuloDataTypeUInt8,
	ModuloDataTypeUInt16,
	ModuloDataTypeUInt32,
	ModuloDataTypeInt8,
	ModuloDataTypeInt16,
	ModuloDataTypeInt32,
	ModuloDataTypeString,
	ModuloDataTypeStream,
	ModuloDataTypeFloat,
	ModuloDataTypeBitfield8,
	ModuloDataTypeBitfield16,
	ModuloDataTypeBitfield32
};

enum ModuloStatus {
	ModuloStatusOff,
	ModuloStatusOn,
	ModuloStatusBlinking,
	ModuloStatusBreathing
};

#define MODULO_MAX_BUFFER_SIZE 31

// Buffer for storing data transmitted from the master to this device.
class ModuloWriteBuffer {
public:
	ModuloWriteBuffer();
	void Reset(uint8_t address);

	// Append a value to the buffer
	// Return false if there is not enough space or the CRC fails.
	bool Append(const uint8_t value);

    uint8_t GetSize() const {
        return _length;
    }

	// Get the next value, of type T, from the buffer.
	// Return false if there is not enough data.
	template <class T>
	T Get(uint8_t offset) const {

		if (offset+sizeof(T) > _length) {
			return T();
		}
        T retval;
        memcpy(&retval, _data+offset, sizeof(T));
	    return retval;
    }

    uint8_t GetCommand() const {
        return _command;
    }
	
	bool IsValid();
	
    uint8_t GetAddress() const {
        return _address;
    }

private:
	uint8_t _data[MODULO_MAX_BUFFER_SIZE];
    uint8_t _address;
	uint8_t _command;
	uint8_t _length;
	uint8_t _expectedLength;
    uint8_t _computedCRC;
    int16_t _receivedCRC;
};

class ModuloReadBuffer {
public:
    ModuloReadBuffer() {}

    void Reset(uint8_t address) {
        _address = address;
        _readPosition = 0;
        _length = 0;
    }

   	// Append a value of type T to the buffer.
    // Return false if there's not enough room in the buffer.
   	template <class T>
   	bool AppendValue(const T &value) {
       	if (_GetLength()+sizeof(T) >= MODULO_MAX_BUFFER_SIZE-1) {
           	return false;
       	}
       	memcpy(_data+_length, &value, sizeof(T));
        _length += sizeof(T);
       	return true;
   	}

    void AppendString(const char *s) {
        while (*s) {
            AppendValue<char>(*s);
            s++;
        }
        AppendValue<char>(0);
    }

    uint8_t ComputeCRC(uint8_t address) {
        uint8_t crc = _crc8_ccitt_update(0, address);
        for (int i=0; i < _GetLength(); i++) {
            crc = _crc8_ccitt_update(crc, _data[i]);
        }
        return crc;
    }

    bool GetNextByte(uint8_t *data) {
        if (_readPosition < _GetLength()) {
            *data = _data[_readPosition++];
            return true;
        }
        return false;
    }    

private:
    uint8_t _GetLength() const {
        return _length;
    }

    uint8_t _length;
    uint8_t _address;
    uint8_t _data[MODULO_MAX_BUFFER_SIZE];
    uint8_t _readPosition;
};

void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask);
	
void ModuloSetStatus(ModuloStatus status);
void ModuloUpdateStatusLED();

template <class T>
class ModuloVariable {
public:
	ModuloVariable(const T &initialValue) : _value(initialValue) {
	}
	
	ModuloVariable() : _value(T()) {
	}
	
	// Get the value of this variable. 
	T Get() {
		T result;
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			result = _value;
		}
		return result;
	}
	
	// Set the value of this variable
	void Set(const T &value) {
		ATOMIC_BLOCK(ATOMIC_RESTORESTATE) {
			_value = value;
		}
	}
	
private:
	volatile T _value;
};


#define MODULO_REGISTER_COMPANY_NAME 200
#define MODULO_REGISTER_DEVICE_NAME 201
#define MODULO_REGISTER_DEVICE_VERSION 202
#define MODULO_REGISTER_DEVICE_ID 203
#define MODULO_REGISTER_DOC_URL 204
#define MODULO_REGISTER_FUNCTION_NAMES 205
#define MODULO_REGISTER_FUNCTION_TYPES 206
#define MODULO_REGISTER_STATUS_LED 207
#define MODULO_REGISTER_LOT_NUMBER 208
#define MODULO_REGISTER_SERIAL_NUMBER 209
#define MODULO_REGISTER_START_ENUMERATION 250
#define MODULO_REGISTER_ENUMERATE_NEXT_DEVICE 252
#define MODULO_REGISTER_ASSIGN_DEVICE_ID 253
#define MODULO_REGISTER_ASSIGN_ADDRESS 254
#define MODULO_INVALID_REGISTER 255

#define DEFINE_MODULO_CONSTANTS(companyName, deviceName, deviceVersion, docURL) \
    const char ModuloCompanyName[] = companyName; \
    const char ModuloDeviceName[] = deviceName; \
    const uint8_t ModuloDeviceVersion = deviceVersion; \
    const char ModuloDocURL[] = docURL;

#define DEFINE_MODULO_FUNCTION_NAMES(functionNames) \
    const char ModuloFunctionNames[] = functionNames;

#define DEFINE_MODULO_FUNCTION_TYPES(...) \
    const ModuloDataType ModuloDataTypes[] = {__VA_ARGS__, ModuloDataTypeNone};

uint16_t ModuloGetDeviceID();



#endif /* MODULO_H_ */
