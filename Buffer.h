/*
 * Buffer.h
 *
 * Created: 7/17/2015 9:27:41 AM
 *  Author: ekt
 */ 


#ifndef BUFFER_H_
#define BUFFER_H_

#include <inttypes.h>
#include <util/crc16.h>
#include <string.h>

class ModuloReadBuffer;
class ModuloWriteBuffer;

extern ModuloWriteBuffer moduloWriteBuffer;
extern ModuloReadBuffer moduloReadBuffer;

#define MODULO_MAX_BUFFER_SIZE 31

// Buffer for storing data transmitted from the master to this device.
class ModuloWriteBuffer {
	public:
	ModuloWriteBuffer();
	void Reset(uint8_t address);

	// Append a value to the buffer
	// Return false if there is not enough space or the CRC fails.
	bool Append(const uint8_t value);

	uint8_t GetSize() const {
		return _length;
	}

	// Get the next value, of type T, from the buffer.
	// Return false if there is not enough data.
	template <class T>
	T Get(uint8_t offset) const {

		if (offset+sizeof(T) > _length) {
			return T();
		}
		T retval;
		memcpy(&retval, _data+offset, sizeof(T));
		return retval;
	}

	uint8_t GetCommand() const {
		return _command;
	}
	
	bool IsValid();
	
	uint8_t GetAddress() const {
		return _address;
	}

	private:
	uint8_t _data[MODULO_MAX_BUFFER_SIZE];
	uint8_t _address;
	uint8_t _command;
	uint8_t _length;
	uint8_t _expectedLength;
	uint8_t _computedCRC;
	int16_t _receivedCRC;
};


class ModuloReadBuffer {
	public:
	ModuloReadBuffer() {}

	void Reset(uint8_t address) {
		_address = address;
		_readPosition = 0;
		_length = 0;
	}

	// Append a value of type T to the buffer.
	// Return false if there's not enough room in the buffer.
	template <class T>
	bool AppendValue(const T &value) {
		if (_GetLength()+sizeof(T) >= MODULO_MAX_BUFFER_SIZE-1) {
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
		uint8_t crc = _crc8_ccitt_update(0, address);
		for (int i=0; i < _GetLength(); i++) {
			crc = _crc8_ccitt_update(crc, _data[i]);
		}
		return crc;
	}

	bool GetNextByte(uint8_t *data) {
		if (_readPosition < _GetLength()) {
			*data = _data[_readPosition++];
			return true;
		}
		return false;
	}

	private:
	uint8_t _GetLength() const {
		return _length;
	}

	uint8_t _length;
	uint8_t _address;
	uint8_t _data[MODULO_MAX_BUFFER_SIZE];
	uint8_t _readPosition;
};

#endif /* BUFFER_H_ */