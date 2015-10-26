/*
 * Buffer.cpp
 *
 * Created: 7/17/2015 9:30:06 AM
 *  Author: ekt
 */ 

#include "Buffer.h"
#include <util/crc16.h>

ModuloWriteBuffer::ModuloWriteBuffer(uint8_t address, uint8_t *data, uint8_t len) {
	_length = len;
	_address = address;
	_data = data;
	_computedCRC = _crc8_ccitt_update(0, address);

	for (int i=0; (i+1) < len; i++) {
		_computedCRC = _crc8_ccitt_update(_computedCRC, _data[i]);
	}
}

bool ModuloWriteBuffer::IsValid() {
	return (_length > DATA_START and _length-3 == _data[EXPECTED_LENGTH] and _data[_length-1] == _computedCRC);
}
