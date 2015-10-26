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
_writePos(0), _readPos(0),
_complete(true)
{
		
}
	
	
// Called from the ISR when new data arrives
void OpStream::AppendData(const ModuloWriteBuffer &buffer) {
	// Wait until the last sync has completed
	for (int i=0; i < buffer.GetSize(); i++) {
		_data[_writePos++] = buffer.Get<uint8_t>(i);
	}
	_complete = false;
}
	
void OpStream::ProcessOp() {
	if (_complete) {
		return;
	}
		
	if (_readPos < _writePos) {
		// Read and process the next operation
		switch(_Read<uint8_t>()) {
			case OpRefresh:
			SSD1331Refresh(_display->width(), _display->height(), _display->getData());
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
				int x = _Read<uint8_t>();
				int y = _Read<uint8_t>();
				int w = _Read<uint8_t>();
				int h = _Read<uint8_t>();
				int r = _Read<uint8_t>();
				if (_fillColor.a > 0) {
					_display->fillRect(x,y,w,h,_fillColor.Color565());
				}
				if (_lineColor.a > 0) {
					_display->drawRect(x,y,w,h,_lineColor.Color565());
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
			case OpSetCursor:
			cursor_x = _Read<uint8_t>();
			cursor_y = _Read<uint8_t>();
			break;
			case OpSetTextSize:
			_display->setTextSize(_Read<uint8_t>());
			break;
		}
			
	}
		
	system_interrupt_enter_critical_section();
	if (_readPos >= _writePos) {
		_complete = true;
		_readPos = 0;
		_writePos = 0;
	}
	system_interrupt_leave_critical_section();
		
}
	
bool OpStream::IsComplete() {
	return _complete;
}
