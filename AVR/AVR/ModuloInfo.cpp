/*
 * DeviceID.cpp
 *
 * Created: 10/26/2015 12:42:23 PM
 *  Author: ekt
 */ 

#include "ModuloInfo.h"
#include "Random.h"
#include <avr/eeprom.h>

// The top of EEPROM is reserved for metadata about the modulo.
// It is stored in EEPROM, rather than as a string constant in
// program memory so that the bootloader can inspect it.
//
// This does mean, however, that the eeprom section MUST be programmed
// in order for the modulo to work.

extern ModuloInfo moduloInfo __attribute__((section(".moduloInfo")));

volatile uint16_t _moduloID = 0xFFFF;

uint16_t GetDeviceID() {
	// If _deviceID has been not been initialized, load it from the EEPROM
	if (_moduloID == 0xFFFF) {
		// Load the device ID from EEPROM.
		volatile uint16_t * volatile addr = &moduloInfo.id;
		
		_moduloID = eeprom_read_word(&moduloInfo.id);

		if (_moduloID == 0xFFFF) {
			uint32_t r = GenerateRandomNumber();
			_moduloID = r ^ (r >> 16); // xor the low and high words to conserve entropy
			eeprom_write_word(&moduloInfo.id, _moduloID);
		}

	}
	
	
	return _moduloID;
}

uint16_t GetModuloVersion() {
	return eeprom_read_word(&moduloInfo.version);	
}

// Return a single byte the specified index from the device type
uint8_t GetModuloType(uint8_t i) {
	char c = 0;
	if (i < MODULO_TYPE_SIZE) {
		c = eeprom_read_byte(&(moduloInfo.type[i]));
	}
	
	// Uninitialized bytes will read 0xFF.
	if (c == 0xFF) {
		c = 0;
	}
	
	return c;
}