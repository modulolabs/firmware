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

Adafruit_SSD1331 display;

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

#define FUNCTION_UPDATE 0
#define FUNCTION_DRAW_LINE 1
#define FUNCTION_FILL_SCREEN 2

void ModuloReset() {
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
	buffer->AppendValue<uint32_t>(0);
	
	return true;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_UPDATE:
			if (buffer.GetSize() == 0) {
				display.refresh();
				return true;
			}
			return false;
		case FUNCTION_DRAW_LINE:
			if (buffer.GetSize() == 6) {
				display.drawLine(buffer.Get<uint8_t>(0), buffer.Get<uint8_t>(1),
					buffer.Get<uint8_t>(2), buffer.Get<uint8_t>(3), buffer.Get<uint16_t>(4));

				return true;
			}
			return false;
		case FUNCTION_FILL_SCREEN:
			if (buffer.GetSize() == 2) {
				display.fillScreen(buffer.Get<uint16_t>(0));
				return true;
			}
			return false;
	}
	return false;
}


bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	
}

int main (void)
{
	system_init();
	
	cpu_irq_enable();
	
	configure_i2c_slave();

	
	display.begin();
	display.fillScreen(0);
	display.refresh();
	
	uint8_t count = 0;

	while (1) {
	}
}
