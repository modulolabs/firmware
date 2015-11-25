/*
 * OpStream.cpp
 *
 * Created: 10/23/2015 3:53:23 PM
 *  Author: ekt
 */ 
#include "OpStream.h"
#include "SSD1331.h"
#include <asf.h>

OpStream::OpStream(Adafruit_GFX *display) :
	_display(display),
	_lineColor(Color(255,255,255,255)),
	_fillColor(Color(255,255,255,255)),
	_textColor(Color(255,255,255,255)),
	_size(0), _writePos(0), _readPos(0), _complete(true)
{	
}
	
	
// Called from the ISR when new data arrives
void OpStream::AppendData(const ModuloWriteBuffer &buffer) {
	system_interrupt_enter_critical_section();
		
	// Drop this buffer if it would overflow.
	// XXX: Need a way to retry or check before sending.
	if (_size+buffer.GetSize() < STREAM_SIZE) {
		for (int i=0; i < buffer.GetSize(); i++) {
			_data[_writePos] = buffer.Get<uint8_t>(i);
			_writePos = (_writePos+1) % STREAM_SIZE;
			_size++;
		}
	}
	
	_complete = false;
	
	system_interrupt_leave_critical_section();
	
}

uint8_t OpStream::_ReadByte() {
	if (_size == 0) {
		return 0;
	}

	system_interrupt_enter_critical_section();
	uint8_t retval = _data[_readPos];
	_size--;
	_readPos = (_readPos+1) % STREAM_SIZE;
	system_interrupt_leave_critical_section();
	
	return retval;
}


void OpStream::ProcessOp() {
	if (_size == 0) {
		return;
	}
		

	// Read and process the next operation
	switch(_Read<uint8_t>()) {
		case OpClear:
			_display->fillScreen(0);
			_lineColor = Color(255,255,255,255);
			_fillColor = Color(255,255,255,255);
			_textColor = Color(255,255,255,255);
			_display->setTextSize(1);
			cursor_x = 0;
			cursor_y = 0;
			break;
		case OpRefresh:
			SSD1331Refresh(_display->width(), _display->height(), _display->getData(), _Read<uint8_t>());
			break;
		case OpFillScreen :
			_display->fillScreen(_Read<Color>().Color565());
			break;
		case OpSetLineColor:
			_lineColor = _Read<Color>();
			break;
		case OpSetFillColor:
			_fillColor = _Read<Color>();
			break;
		case OpSetTextColor:
			_textColor = _Read<Color>();
			break;
		case OpDrawLine:
			if (_lineColor.a > 0) {
				_display->drawLine(_Read<uint8_t>(), _Read<uint8_t>(),
				_Read<uint8_t>(), _Read<uint8_t>(),
				_lineColor.Color565());
			}
			break;
		case OpDrawRect:
		{
			int x = _Read<int8_t>();
			int y = _Read<int8_t>();
			int w = _Read<uint8_t>();
			int h = _Read<uint8_t>();
			int r = _Read<uint8_t>();
			if (_fillColor.a > 0) {
				if (r) {
					_display->fillRoundRect(x,y,w,h,r,_fillColor.Color565());
				} else {
					_display->fillRect(x,y,w,h,_fillColor.Color565());
				}
			}
			if (_lineColor.a > 0) {
				if (r) {
					_display->drawRoundRect(x,y,w,h,r,_lineColor.Color565());
				} else {
					_display->drawRect(x,y,w,h,_lineColor.Color565());
				}
			}
		}
			break;
		case OpDrawCircle:
		{
			int x = _Read<uint8_t>();
			int y = _Read<uint8_t>();
			int r = _Read<uint8_t>();
			if (_fillColor.a > 0) {
				_display->fillCircle(x,y,r,_fillColor.Color565());
			}
			if (_lineColor.a > 0) {
				_display->drawCircle(x,y,r,_lineColor.Color565());
			}
		}
			break;
		case OpDrawString:
		{
			char c = _Read<uint8_t>();
			while (c) {
				switch(c) {
					case '\n':
					cursor_y += 8*_display->getTextSize();
					cursor_x = 0;
					break;
					case '\r':
					break;
					default:
					_display->drawChar(cursor_x, cursor_y, c, _textColor.Color565(), 0);
					cursor_x += 6*_display->getTextSize();
					break;
				}
				c = _Read<uint8_t>();
			}
			break;
		}
		case OpDrawTriangle:
		{
			int x0 = _Read<int8_t>();
			int y0 = _Read<int8_t>();
			int x1 = _Read<int8_t>();
			int y1 = _Read<int8_t>();
			int x2 = _Read<int8_t>();
			int y2 = _Read<int8_t>();
			if (_fillColor.a > 0) {
				_display->fillTriangle(x0,y0,x1,y1,x2,y2,_fillColor.Color565());
			}
			if (_lineColor.a > 0) {
				_display->drawTriangle(x0,y0,x1,y1,x2,y2,_lineColor.Color565());
			}
			break;			
		}
		case OpSetCursor:
			cursor_x = _Read<uint8_t>();
			cursor_y = _Read<uint8_t>();
			break;
		case OpSetTextSize:
			_display->setTextSize(_Read<uint8_t>());
			break;
	}
	
	// We just executed an op, so if it's empty now then it's also complete.
	_complete = IsEmpty();
}
	
bool OpStream::IsComplete() {
	return _complete;
}

bool OpStream::IsEmpty() {
	return (_size == 0);
}

uint16_t OpStream::GetAvailableSpace() {
	return STREAM_SIZE-_size;
}

