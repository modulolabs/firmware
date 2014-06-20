/*
 * TwoWire.h
 *
 * Created: 5/10/2014 8:13:18 PM
 *  Author: ekt
 */ 


#ifndef TWOWIRE_H_
#define TWOWIRE_H_

typedef void(*TwoWireDataReceivedCallback)(uint8_t *data, uint8_t numBytes);
typedef void(*TwoWireDataRequestedCallback)(uint8_t *data, uint8_t *numBytes, uint8_t maxLength);

// Initialize the TwoWire slave interface. The provided callbacks will be excuted when
// data is either received or requested.
void TwoWireInit(uint8_t address, TwoWireDataReceivedCallback dataReceivedCallback,
	TwoWireDataRequestedCallback dataRequestedCallback);

#endif /* TWOWIRE_H_ */

