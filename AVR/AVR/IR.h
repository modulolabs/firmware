/*
 * IR3.h
 *
 * Created: 10/21/2015 2:30:59 PM
 *  Author: ekt
 */ 


#ifndef IR3_H_
#define IR3_H_

void IR3ReceiveEnable();
void IR3ReceiveDisable();

bool IR3IsReceiveComplete();
uint8_t IR3Receive(uint8_t *buffer, uint8_t maxLen);

#define IR_BUFFER_SIZE 96

#define IR_MAX_LENGTH 253
#define IR_TOKEN_END 254
#define IR_TOKEN_START 255

#if 0
template <class T, int SIZE>
class RingBuffer {
	public:
	RingBuffer() : _size(0), _readIndex(0), _writeIndex(0) {}
		
	bool push(uint8_t b) {
		if (_size == SIZE) {
			return false;
		}
		data[_writeIndex] = b;
		_writeIndex = (_writeIndex+1) % SIZE;
		_size++;
		return true;
	}

	uint8_t pop() {
		if (_size == 0) {
			return false;
		}
		uint8_t b = data[_readIndex];
		_readIndex = (_readIndex+1) % SIZE;
		_size--;
		return b;
	}
	
	bool isEmpty() {
		return _size == 0;
	}
	
	bool isFull() {
		return _size == SIZE;
	}
	
	uint8_t peek() {
		return data[_readIndex];
	}
	
	uint8_t getSize() {
		return _size;
	}
	
	private:
	T data[SIZE];
	volatile uint16_t _size;
	volatile uint16_t _readIndex;
	volatile uint16_t _writeIndex;
	
};

extern RingBuffer<uint8_t, 256> irRingBuffer;
#endif

#endif /* IR3_H_ */