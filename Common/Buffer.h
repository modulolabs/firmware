/*
 * Buffer.h
 *
 * Created: 7/17/2015 9:27:41 AM
 *  Author: ekt
 */ 


#ifndef BUFFER_H_
#define BUFFER_H_

#include <inttypes.h>
#include <string.h>
#include "crc.h"

class ModuloReadBuffer;
class ModuloWriteBuffer;

extern ModuloWriteBuffer moduloWriteBuffer;
extern ModuloReadBuffer moduloReadBuffer;


// Buffer for storing data transmitted from the master to this device.
class ModuloWriteBuffer {

	static const int COMMAND = 0;
	static const int EXPECTED_LENGTH = 1;
	static const int DATA_START = 2;
		
public:
	ModuloWriteBuffer(uint8_t address, uint8_t *data, uint8_t maxLen);

	uint8_t GetSize() const {
		if (_length < DATA_START+1) {
			return 0;
		}
		return _length-(DATA_START+1);
	}

	// Get the next value, of type T, from the buffer.
	// Return false if there is not enough data.
	template <class T>
	T Get(uint8_t offset) const {

		if (offset+sizeof(T)+DATA_START > _length) {
			return T();
		}
		T retval;
		memcpy(&retval, _data+offset+DATA_START, sizeof(T));
		return retval;
	}

	uint8_t GetCommand() const {
		return _data[COMMAND];
	}
	
	bool IsValid();
	
	uint8_t GetAddress() const {
		return _address;
	}

private:
	uint8_t _address;
	uint8_t _length;
	uint8_t _computedCRC;
	
	uint8_t *_data;
};


class ModuloReadBuffer {
	public:
	ModuloReadBuffer(uint8_t address, uint8_t *data, uint8_t maxLen) {
		_address = address;
		_readPosition = 0;
		_length = 0;
		_data = data;
		_maxLen = maxLen;
	}

	// Append a value of type T to the buffer.
	// Return false if there's not enough room in the buffer.
	template <class T>
	bool AppendValue(const T &value) {
		if (GetLength()+sizeof(T) >= _maxLen-1) {
			return false;
		}
		memcpy(_data+_length, &value, sizeof(T));
		_length += sizeof(T);
		return true;
	}

	void AppendString(const char *s) {
		while (*s) {
			AppendValue<char>(*s);
			s++;
		}
		AppendValue<char>(0);
	}

	uint8_t ComputeCRC(uint8_t address) {
		uint8_t crc = crc_update(0, address);
		for (int i=0; i < GetLength(); i++) {
			crc = crc_update(crc, _data[i]);
		}
		return crc;
	}

	bool GetNextByte(uint8_t *data) {
		if (_readPosition < GetLength()) {
			*data = _data[_readPosition++];
			return true;
		}
		return false;
	}

	uint8_t GetLength() const {
		return _length;
	}
	
private:


	uint8_t _length;
	uint8_t _address;
	uint8_t *_data;
	uint8_t _maxLen;
	uint8_t _readPosition;
};

#endif /* BUFFER_H_ */