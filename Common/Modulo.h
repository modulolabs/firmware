/*
 * Modulo.h
 *
 * Created: 7/9/2014 10:03:26 AM
 *  Author: ekt
 */ 


#ifndef MODULO_H_
#define MODULO_H_

#include <inttypes.h>
#include <string.h>

#include "Buffer.h"

extern const uint8_t moduloBroadcastAddress;

bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer);
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
	uint8_t statusMask,
	bool useTwoWireInterrupt = true);
	
void ModuloSetStatus(ModuloStatus status);
void ModuloUpdateStatusLED();

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
