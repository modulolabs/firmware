/*
 * Buffer.cpp
 *
 * Created: 7/17/2015 9:30:06 AM
 *  Author: ekt
 */ 

#include "Buffer.h"
#include <util/crc16.h>

ModuloWriteBuffer moduloWriteBuffer;
ModuloReadBuffer moduloReadBuffer;

ModuloWriteBuffer::ModuloWriteBuffer() {
}

void ModuloWriteBuffer::Reset(volatile uint8_t address) {
	_length = 0;
	_address = address;
	_computedCRC = _crc8_ccitt_update(0, address);
}

// Append a value to the buffer
// Return false if there is not enough space or the CRC fails.
bool ModuloWriteBuffer::Append(volatile const uint8_t value) {
	if (_length >= MODULO_MAX_BUFFER_SIZE) {
		return false;
	}

	if (_length > 0) {
		_computedCRC = _crc8_ccitt_update(_computedCRC, _data[_length-1]);
	}
	_data[_length++] = value;

	
	return false;
}

bool ModuloWriteBuffer::IsValid() {
	return (_length > DATA_START and _length-3 == _data[EXPECTED_LENGTH] and _data[_length-1] == _computedCRC);
}
