/*
 * DeviceID.cpp
 *
 * Created: 10/26/2015 10:17:13 AM
 *  Author: ekt
 */ 

#include "Modulo.h"

static uint16_t _deviceID = 0xFFFF;

uint16_t GetDeviceID() {
	if (_deviceID != 0xFFFF) {
		return _deviceID;
	}
	
	// Extract the serial number from the specific addresses according to the data sheet
	uint32_t serialNum[] = {
		*(uint32_t*)(0x0080A00C),
		*(uint32_t*)(0x0080A040),
		*(uint32_t*)(0x0080A044),
	    *(uint32_t*)(0x0080A048)};
	
	// Hash the 128 bit unique ID into a 16 bit unique-ish ID
	_deviceID = 0;
	for (int i=0; i < 4; i++) {
		_deviceID ^= (serialNum[i] & 0xFFFF);
		_deviceID ^= (serialNum[i] >> 16);
	}
	
	return _deviceID;
}

static const uint8_t moduloType[MODULO_TYPE_SIZE] = "co.modulo.colordisplay";

uint8_t GetModuloType(uint8_t i) {
	if (i < MODULO_TYPE_SIZE) {
		return moduloType[i];
	}
	return 0;
}

uint16_t GetModuloVersion() {
	return 1;
}
