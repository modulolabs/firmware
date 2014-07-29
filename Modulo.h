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
#include <util/atomic.h>
#include <math.h>

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

// A buffer holds 4 bytes, which is the maximum size
// for an atomic read/write.
struct ModuloBuffer {
	mutable volatile uint8_t data[4];
	
	ModuloBuffer() {
		Reset();
	}
	
	void Reset() {
		volatile uint32_t *p = (uint32_t*)data;
		*p = 0;
	}
	
	template <class T>
	void Set(const T& value) {
		volatile T *p = (T*)data;
		*p = value;
	}
	
	
	template <class T>
	T Get() const {
		volatile T *value = (T*)data;
		return *value;
	}
};


typedef void (*ModuloReadValueCallback)(uint8_t functionID, ModuloBuffer *buffer);
typedef void (*ModuloWriteValueCallback)(uint8_t functionID, const ModuloBuffer &buffer);

typedef void (*ModuloBeginWriteArrayCallback)(uint8_t functionID, uint8_t arrayLength);
typedef void (*ModuloWriteArrayDataCallback)(uint8_t functionID, uint8_t index, uint8_t data);
typedef void (*ModuloEndWriteArrayCallback)(uint8_t functionID);

typedef uint8_t (*ModuloBeginReadArrayCallback)(uint8_t functionID);
typedef uint8_t (*ModuloReadArrayDataCallback)(uint8_t functionID, uint8_t index);
typedef void (*ModuloEndReadArrayCallback)(uint8_t functionID);

void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask,
	ModuloReadValueCallback readValue = 0,
	ModuloWriteValueCallback writeValue = 0,
	ModuloBeginWriteArrayCallback beginWriteArray = 0,
	ModuloWriteArrayDataCallback writeArrayData = 0,
	ModuloEndWriteArrayCallback endWriteArray = 0,
	ModuloBeginReadArrayCallback beginReadArray = 0,
	ModuloReadArrayDataCallback readArrayData = 0,
	ModuloEndReadArrayCallback endReadArray = 0);
	
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
#define MODULO_INVALID_REGISTER 255

extern const char ModuloCompanyName[];
extern const char ModuloDeviceName[];
extern const uint8_t ModuloDeviceVersion;
extern const char ModuloDocURL[];
extern const char ModuloFunctionNames[];
extern const ModuloDataType ModuloDataTypes[];

#define DEFINE_MODULO_CONSTANTS(companyName, deviceName, deviceVersion, docURL) \
    const char ModuloCompanyName[] = companyName; \
    const char ModuloDeviceName[] = deviceName; \
    const uint8_t ModuloDeviceVersion = deviceVersion; \
    const char ModuloDocURL[] = docURL;

#define DEFINE_MODULO_FUNCTION_NAMES(functionNames) \
    const char ModuloFunctionNames[] = functionNames;

#define DEFINE_MODULO_FUNCTION_TYPES(...) \
    const ModuloDataType ModuloDataTypes[] = {__VA_ARGS__, ModuloDataTypeNone};



#endif /* MODULO_H_ */