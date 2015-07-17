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
	_command = 0xFF;
	_length = 0;
	_expectedLength = 0xFF;
	_receivedCRC = -1;
	_address = address >> 1;
	_computedCRC = _crc8_ccitt_update(0, address);
}

// Append a value to the buffer
// Return false if there is not enough space or the CRC fails.
bool ModuloWriteBuffer::Append(volatile const uint8_t value) {
	if (_command == 0xFF) {
		// The first byte contains the command in the upper 3 bits
		// and the expected length in the lower 5 bits.
		_command = value;
		_computedCRC = _crc8_ccitt_update(_computedCRC, value);
		return true;
	}
	if (_expectedLength == 0xFF) {
		_expectedLength = value;
		_computedCRC =  _crc8_ccitt_update(_computedCRC, value);
		return true;
	}
	if (_length+1 >= MODULO_MAX_BUFFER_SIZE) {
		return false;
	}
	if (_length < _expectedLength) {
		_data[_length] = value;
		_length++;
		_computedCRC = _crc8_ccitt_update(_computedCRC, value);
		return true;
	}
	if (_receivedCRC == -1) {
		_receivedCRC = value;
		return true;
		// return value == _computedCRC;
	}

	return false;
}

bool ModuloWriteBuffer::IsValid() {
	return (_length == _expectedLength and _computedCRC == _receivedCRC);
}
