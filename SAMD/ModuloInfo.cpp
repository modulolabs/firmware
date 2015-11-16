/*
 * DeviceID.cpp
 *
 * Created: 10/26/2015 10:17:13 AM
 *  Author: ekt
 */ 

#include "Modulo.h"
#include "ModuloInfo.h"
#include "asf.h"

// Reserve space for the Modulo info in the nvm space
// At build time the .moduloInfo section will be set to a specific fixed address so that the bootloader can
// also access it.
ModuloInfo _moduloInfo = {.id=0xFFFF, .version=0xFFFF, ""};
		
extern ModuloInfo moduloInfo;

static uint16_t _generateDeviceID() {
	// Extract the serial number from the specific addresses according to the data sheet
	uint32_t serialNum[] = {
		*(uint32_t*)(0x0080A00C),
		*(uint32_t*)(0x0080A040),
		*(uint32_t*)(0x0080A044),
	    *(uint32_t*)(0x0080A048)};
	
	// Hash the 128 bit unique ID into a 16 bit unique-ish ID
	uint16_t deviceID = 0;
	for (int i=0; i < 4; i++) {
		deviceID ^= (serialNum[i] & 0xFFFF);
		deviceID ^= (serialNum[i] >> 16);
	}
	return deviceID;
}

void LoadModuloInfo() {
	
	nvm_read_buffer((uint32_t)&moduloInfo, (uint8_t*)&_moduloInfo, sizeof(ModuloInfo));

	if (_moduloInfo.id != 0xFFFF) {
		return;
	}
	
	// Valid id not found in the persistent info. Generate it.
	_moduloInfo.id = _generateDeviceID();
	
	// Save the new ID
	SaveModuloInfo();
}

void SaveModuloInfo() {
	nvm_erase_row((uint32_t)&moduloInfo);
	nvm_write_buffer((uint32_t)&moduloInfo, (uint8_t*)&_moduloInfo, sizeof(ModuloInfo));
}

uint16_t GetDeviceID() {
	return _moduloInfo.id;
}

void SetDeviceID(uint16_t deviceID) {
	_moduloInfo.id = deviceID;
	SaveModuloInfo();
}

uint8_t GetModuloType(uint8_t i) {
	if (i < MODULO_TYPE_SIZE) {
		return moduloInfo.type[i];
	}
	return 0;
}

uint16_t GetModuloVersion() {
	return moduloInfo.version;
}
