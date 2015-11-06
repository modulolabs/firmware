/*
 * TwoWire.h
 *
 * Created: 10/23/2015 3:51:00 PM
 *  Author: ekt
 */ 


#ifndef TWOWIRE_H_
#define TWOWIRE_H_

#include <inttypes.h>

void TwoWireInit(bool useInterrupts = true);
void TwoWireSetDeviceAddress(uint8_t address);
uint8_t TwoWireGetDeviceAddress();

// When not using interrupts, call TwoWireUpdate periodically.
void TwoWireUpdate();

int TwoWireCallback(uint8_t address, uint8_t *buffer, uint8_t len, uint8_t maxLen);

#endif /* TWOWIRE_H_ */