/*
 * TwoWire.h
 *
 * Created: 7/16/2015 7:19:10 PM
 *  Author: ekt
 */ 


#ifndef TWOWIRE_H_
#define TWOWIRE_H_


void TwoWireInit();
void TwoWireSetDeviceAddress(uint8_t address);
uint8_t TwoWireGetDeviceAddress();

bool TwoWireWriteCallback(const ModuloWriteBuffer &buffer);
bool TwoWireReadCallback(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *readBuffer);

#endif /* TWOWIRE_H_ */