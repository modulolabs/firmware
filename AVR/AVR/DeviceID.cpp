/*
 * DeviceID.cpp
 *
 * Created: 10/26/2015 12:42:23 PM
 *  Author: ekt
 */ 

#include "DeviceID.h"
#include "Random.h"
#include <avr/eeprom.h>

static uint16_t _deviceID_EEPROM EEMEM = 0xFFFF;
volatile uint16_t _deviceID = 0xFFFF;

uint16_t GetDeviceID() {
	// If _deviceID has been initialized, return it
	if (_deviceID != 0xFFFF) {
		return _deviceID;
	}
	
	// Load the device ID from EEPROM.
	_deviceID = eeprom_read_word(&_deviceID_EEPROM);
	
	// If nothing was stored in the EEPROM, then generate a random device id.
	while (_deviceID == 0xFFFF) {
		uint32_t r = GenerateRandomNumber();
		_deviceID = r ^ (r >> 16); // xor the low and high words to conserve entropy
		eeprom_write_word(&_deviceID_EEPROM, _deviceID);
	}
	
	return _deviceID;
}