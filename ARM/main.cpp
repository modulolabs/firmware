/**
 * \file
 *
 * \brief Empty user application template
 *
 */

/**
 * \mainpage User Application template doxygen documentation
 *
 * \par Empty user application template
 *
 * Bare minimum empty user application template
 *
 * \par Content
 *
 * -# Include the ASF header files (through asf.h)
 * -# Minimal main function that starts with a call to system_init()
 * -# "Insert application code here" comment
 *
 */

/*
 * Include header files for all drivers that have been imported from
 * Atmel Software Framework (ASF).
 */
 /**
 * Support and FAQ: visit <a href="http://www.atmel.com/design-support/">Atmel Support</a>
 */
#include <asf.h>
#include "Adafruit_GFX/Adafruit_SSD1331.h"
#include <clock.h>
#include <interrupt.h>
#include <gclk.h>
#include <port.h>
#include <pinmux.h>
#include "Modulo.h"



extern "C" {
	void _read() {
		
	}
	void _write() {
		
	}
};



#include <sercom.h>

#define DATA_LENGTH 32
uint8_t read_buffer[DATA_LENGTH] = {0x1, 0x2, 0x03};
uint8_t write_buffer[DATA_LENGTH] = {7,8,9,10};
volatile uint8_t addr;

struct i2c_slave_module i2c_slave_instance;
enum i2c_slave_direction dir;
struct i2c_slave_packet packet = {
	.data_length = DATA_LENGTH,
	.data = write_buffer
};
uint8_t deviceAddress = 0xFF;



void i2c_read_request_callback(struct i2c_slave_module *module)
{
	packet.data_length = 0;
	packet.data = write_buffer;

	// Get the address that we received the request on.
	// The address is in the high 7 bits of addr.
	SercomI2cs *const i2c_hw = &(module->hw->I2CS);
    addr = i2c_hw->DATA.reg;
	
	if ((addr >> 1) == 9 or (addr >> 1) == deviceAddress) {
		if (ModuloHandleDataRequested(addr, write_buffer)) {
			packet.data_length = DATA_LENGTH;		
		}
	}
	
	i2c_slave_write_packet_job(module, &packet);
			
}



void i2c_write_request_callback(struct i2c_slave_module *const module)
{

	packet.data_length = DATA_LENGTH;
	packet.data = read_buffer;
	
	SercomI2cs *const i2c_hw = &(module->hw->I2CS);
	addr = i2c_hw->DATA.reg;
	asm("nop");
	
	if ((addr >> 1) == 9 or (addr >> 1) == deviceAddress) {
		packet.data_length = DATA_LENGTH;
	} else {
		packet.data_length = 0;	
	}
	
	if (i2c_slave_read_packet_job(module, &packet) != STATUS_OK) {
				
	}
			
}

void i2c_read_complete_callback(struct i2c_slave_module *module) {

	
	ModuloHandleDataReceived(addr, read_buffer);
}


void i2c_write_complete_callback(struct i2c_slave_module *const module) {
	
}



void configure_i2c_slave(uint8_t address=0xFF) {

	struct i2c_slave_config config_i2c_slave;
	i2c_slave_get_config_defaults(&config_i2c_slave);
	config_i2c_slave.address = 0x09;
	config_i2c_slave.address_mask = 0xFF;
	config_i2c_slave.address_mode = I2C_SLAVE_ADDRESS_MODE_MASK;
	config_i2c_slave.pinmux_pad0 = PINMUX_PA22C_SERCOM3_PAD0;
	config_i2c_slave.pinmux_pad1 = PINMUX_PA23C_SERCOM3_PAD1;
	
	i2c_slave_init(&i2c_slave_instance, SERCOM3, &config_i2c_slave);
	i2c_slave_enable(&i2c_slave_instance);
	
	i2c_slave_register_callback(&i2c_slave_instance,
	i2c_read_request_callback,
	I2C_SLAVE_CALLBACK_READ_REQUEST);
	i2c_slave_enable_callback(&i2c_slave_instance,
	I2C_SLAVE_CALLBACK_READ_REQUEST);

	i2c_slave_register_callback(&i2c_slave_instance,
	i2c_read_complete_callback,
	I2C_SLAVE_CALLBACK_READ_COMPLETE);
	i2c_slave_enable_callback(&i2c_slave_instance,
	I2C_SLAVE_CALLBACK_READ_COMPLETE);
	
	i2c_slave_register_callback(&i2c_slave_instance,
	i2c_write_request_callback,
	I2C_SLAVE_CALLBACK_WRITE_REQUEST);
	i2c_slave_enable_callback(&i2c_slave_instance,
	I2C_SLAVE_CALLBACK_WRITE_REQUEST);

	i2c_slave_register_callback(&i2c_slave_instance,
	i2c_write_complete_callback,
	I2C_SLAVE_CALLBACK_WRITE_COMPLETE);
	i2c_slave_enable_callback(&i2c_slave_instance,
	I2C_SLAVE_CALLBACK_WRITE_COMPLETE);
}

void i2c_change_address(uint8_t address) {
	deviceAddress = address;
}

const char *ModuloDeviceType = "co.modulo.colordisplay";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Color Display";
const char *ModuloDocURL = "modulo.co/docs/ColorDisplay";

#define FUNCTION_APPEND_OP 0
#define FUNCTION_IS_COMPLETE 1
#define FUNCTION_GET_BUTTONS 2

volatile uint8_t buttons;

void ModuloReset() {
}

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
	
	OpStream(Adafruit_SSD1331 *display) : 
		_display(display), _writePos(0), _readPos(0), _complete(true),
		_lineColor(Color(255,255,255,255)),
		_fillColor(Color(255,255,255,255)),
		_textColor(Color(255,255,255,255))
	{
		
	}
	
		
	// Called from the ISR when new data arrives
	void AppendData(const ModuloWriteBuffer &buffer) {
		// Wait until the last sync has completed
		for (int i=0; i < buffer.GetSize(); i++) {
			_data[_writePos++] = buffer.Get<uint8_t>(i);
		}
		_complete = false;
	}
	
	void ProcessOp() {
		if (_complete) {
			return;
		}
		
		if (_readPos < _writePos) {
			// Read and process the next operation
			switch(_Read<uint8_t>()) {
				case OpRefresh:
					_display->refresh();
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
	
	bool IsComplete() {
		return _complete;
	}
private:

	// Read the next value, of type T, and advance _readPos
	template <class T>
	T _Read() {
		T retval;
		if (_readPos+sizeof(T) >= STREAM_SIZE) {
			return retval;
		}
		memcpy(&retval, _data+_readPos, sizeof(T));
		_readPos += sizeof(T);
		return retval;
	}

	Adafruit_SSD1331 *_display;
	Color _lineColor;
	Color _fillColor;
	Color _textColor;

	int cursor_x;
	int cursor_y;
	
	static const uint16_t STREAM_SIZE  = 8192;
	uint8_t _data[STREAM_SIZE];
	volatile uint16_t _writePos;	
	volatile uint16_t _readPos;
	volatile bool _complete;
};

Adafruit_SSD1331 display;
OpStream stream(&display);


bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_APPEND_OP:
			stream.AppendData(buffer);
			return true;
		case FUNCTION_IS_COMPLETE:
			return true;
		case FUNCTION_GET_BUTTONS:
			return buffer.GetSize() == 0;
	}
	return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
	switch(writeBuffer.GetCommand()) {
		case FUNCTION_IS_COMPLETE:
			buffer->AppendValue(stream.IsComplete());
			return true;
		case FUNCTION_GET_BUTTONS:
			buffer->AppendValue<uint8_t>((uint8_t)buttons);
			return true;
	}
	return false;
}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	
}

#define BUTTON_1_PIN 27
#define BUTTON_2_PIN 19
#define BUTTON_3_PIN 18


int main (void)
{
	delay_ms(200);
	system_init();
	
	cpu_irq_enable();
	
	configure_i2c_slave();

	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(BUTTON_1_PIN, &config_port_pin);
	port_pin_set_config(BUTTON_2_PIN, &config_port_pin);
	port_pin_set_config(BUTTON_3_PIN, &config_port_pin);
	
	display.begin();
	display.fillScreen(0);
	display.refresh();

	while (1) {
		stream.ProcessOp();
		
		bool button1 = !port_pin_get_input_level(BUTTON_1_PIN);
		bool button2 = !port_pin_get_input_level(BUTTON_2_PIN);
		bool button3 = !port_pin_get_input_level(BUTTON_3_PIN);
		
		buttons = (button3 << 2) | (button2 << 1) | button1;
	}
}
