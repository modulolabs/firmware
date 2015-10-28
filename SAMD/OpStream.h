/*
 * OpStream.h
 *
 * Created: 10/23/2015 3:53:11 PM
 *  Author: ekt
 */ 


#ifndef OPSTREAM_H_
#define OPSTREAM_H_

#include "Adafruit_GFX/Adafruit_GFX.h"
#include "Modulo.h"

class OpStream {

public:

	static const int OpRefresh = 0;
	static const int OpFillScreen = 1;
	static const int OpDrawLine = 2;
	static const int OpSetLineColor = 3;
	static const int OpSetFillColor = 4;
	static const int OpSetTextColor = 5;
	static const int OpDrawRect = 6;
	static const int OpDrawCircle = 7;
	static const int OpDrawTriangle = 8;
	static const int OpDrawString = 9;
	static const int OpSetCursor = 10;
	static const int OpSetTextSize = 11;
	
	OpStream(Adafruit_GFX *display);
	
	
	// Called from the ISR when new data arrives
	void AppendData(const ModuloWriteBuffer &buffer);
	
	void ProcessOp();
	
	// Returns true if the stream is empty, even if the last op is still
	// being executed.
	bool IsEmpty();
	
	// Returns true if the stream is empty AND the last op has been executed.
	bool IsComplete();
	
	// Return the number of bytes available for writing in the stream
	uint16_t GetAvailableSpace();
	
private:

	// Read the next value, of type T, and advance _readPos
	template <class T>
	T _Read() {
		T retval = T();
		for (int i=0; i < sizeof(T); i++) {
			((uint8_t*)&retval)[i] = _ReadByte();
		}
		return retval;
	}
	
	uint8_t _ReadByte();
	
	Adafruit_GFX *_display;
	Color _lineColor;
	Color _fillColor;
	Color _textColor;

	int cursor_x;
	int cursor_y;
	
	static const uint16_t STREAM_SIZE  = 1024;
	uint8_t _data[STREAM_SIZE];
	volatile uint16_t _size;
	volatile uint16_t _writePos;
	volatile uint16_t _readPos;
	volatile bool _complete;
};




#endif /* OPSTREAM_H_ */