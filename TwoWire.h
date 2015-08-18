/*
 * TwoWire.h
 *
 * Created: 7/16/2015 7:19:10 PM
 *  Author: ekt
 */ 


#ifndef TWOWIRE_H_
#define TWOWIRE_H_

#define TWO_WIRE_MAX_BUFFER_SIZE 8

void TwoWireInit();
void TwoWireSetAddress(uint8_t address);
uint8_t TwoWireGetAddress();

bool TwoWireTransmitBegin(uint8_t address);
bool TwoWireTransmitUInt8(uint8_t d);
bool TwoWireTransmitUInt16(uint16_t d);
bool TwoWireTransmitString(const char *s);
bool TwoWireTransmitCRC();
bool TwoWireTransmitEnd();

bool TwoWireWriteCallback(const ModuloWriteBuffer &buffer);
bool TwoWireReadCallback(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *readBuffer);

#endif /* TWOWIRE_H_ */