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
#include <string.h>

#include "Buffer.h"

extern const uint8_t moduloBroadcastAddress;

void moduloUpdate();
void sendPacket(uint8_t packetCode, uint8_t *data, uint8_t len);

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &, ModuloReadBuffer *buffer);
bool ModuloWrite(const ModuloWriteBuffer &buffer);
void ModuloReset();
bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData);
void ModuloClearEvent(uint8_t eventCode, uint16_t eventData);

extern const char *ModuloDeviceType;
extern const uint16_t ModuloDeviceVersion;
extern const char *ModuloCompanyName;
extern const char *ModuloProductName;
extern const char *ModuloDocURL;

enum ModuloStatus {
	ModuloStatusOff,
	ModuloStatusOn,
	ModuloStatusBlinking,
	ModuloStatusBreathing
};

void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask);
	
void ModuloSetStatus(ModuloStatus status);
void ModuloUpdateStatusLED();

void moduloLoop();

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
