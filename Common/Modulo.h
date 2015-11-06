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

enum ModuloStatus {
	ModuloStatusOff,
	ModuloStatusOn,
	ModuloStatusBlinking
};

void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask,
	bool useTwoWireInterrupt = true);
	
void ModuloSetStatus(ModuloStatus status);
void ModuloUpdateStatusLED();

#define MODULO_TYPE_SIZE 32
uint8_t GetModuloType(uint8_t i);
uint16_t GetModuloVersion();

#endif /* MODULO_H_ */
