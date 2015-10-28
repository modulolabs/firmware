
#include "Adafruit_GFX/Adafruit_GFX.h"
#include "SSD1331.h"
#include "Modulo.h"
#include "TwoWire.h"
#include "OpStream.h"

#include <asf.h>

#include <clock.h>
#include <interrupt.h>
#include <gclk.h>
#include <port.h>
#include <pinmux.h>


const char *ModuloDeviceType = "co.modulo.colordisplay";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Color Display";
const char *ModuloDocURL = "modulo.co/docs/ColorDisplay";

#define FUNCTION_APPEND_OP 0
#define FUNCTION_IS_COMPLETE 1
#define FUNCTION_GET_BUTTONS 2
#define FUNCTION_RAW_WRITE 3
#define FUNCTION_IS_EMPTY 4
#define FUNCTION_GET_AVAILABLE_SPACE 5

volatile uint8_t buttons = 0;
volatile uint8_t buttonsPressed = 0;
volatile uint8_t buttonsReleased = 0;

Adafruit_GFX display;
OpStream stream(&display);


bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_APPEND_OP:
			stream.AppendData(buffer);
			return true;
		case FUNCTION_IS_COMPLETE:
			return true;
		case FUNCTION_IS_EMPTY:
			return true;
		case FUNCTION_GET_BUTTONS:
			return buffer.GetSize() == 0;
		case FUNCTION_RAW_WRITE:
			if (buffer.GetSize() <= 1) {
				return false;
			}
			
			for (int i=1; i < buffer.GetSize(); i++) {
				SSD1331RawWrite(buffer.Get<uint8_t>(0), buffer.Get<uint8_t>(i));
			}
			return true;
		case FUNCTION_GET_AVAILABLE_SPACE:
			return true;
			
	}
	return false;
}

bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
	switch(command) {
		case FUNCTION_IS_COMPLETE:
			buffer->AppendValue(stream.IsComplete());
			return true;
		case FUNCTION_IS_EMPTY:
			buffer->AppendValue(stream.IsComplete());
			return true;
		case FUNCTION_GET_BUTTONS:
			buffer->AppendValue<uint8_t>((uint8_t)buttons);
			return true;
		case FUNCTION_GET_AVAILABLE_SPACE:
			buffer->AppendValue<uint16_t>(stream.GetAvailableSpace());
			return true;
	}
	return false;
}


#define EVENT_CODE_BUTTONS 0

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	if (buttonsPressed or buttonsReleased) {
		*eventCode = EVENT_CODE_BUTTONS;
		*eventData = (buttonsPressed << 8) | buttonsReleased;
		return true;
	}
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	uint16_t expectedData = (buttonsPressed << 8) | buttonsReleased;
	if (eventCode == EVENT_CODE_BUTTONS and eventData == expectedData) {
		buttonsPressed = 0;
		buttonsReleased = 0;
	}
}

void ModuloReset() {
	display.fillScreen(Color(0,0,0).Color565());
	SSD1331Refresh(display.width(), display.height(), display.getData());
}

#define BUTTON_1_PIN 27
#define BUTTON_2_PIN 19
#define BUTTON_3_PIN 18

#define SHDN_PIN 25
#define LED_PIN 15

void drawGradient() {
	for (int x=0; x < WIDTH; x++) {
		for (int y=0; y < HEIGHT; y++) {
			uint16_t c = Color(x*255/WIDTH, 0, y*255/HEIGHT).Color565();
			display.drawPixel(x, y, c);
		}
	}
}

void drawGammaTest() {
	for (int x=0; x < WIDTH; x++) {
		uint16_t c = Color(128, 128, 128).Color565();
		
		for (int y=0; y < HEIGHT/2; y++) {
			display.drawPixel(x, y, c);
		}
		
		for (int y=HEIGHT/2;  y < HEIGHT; y++) {	
			display.drawPixel(x, y, ((x+y)%2)*0xFFFF);
		}
	}
}


int main (void)
{
	system_init();
	delay_init();
	
	cpu_irq_enable();
	
	// XXX: Not calling ModuloInit
	TwoWireInit();

	struct port_config config_port_pin;
	port_get_config_defaults(&config_port_pin);
	
	config_port_pin.direction = PORT_PIN_DIR_INPUT;
	config_port_pin.input_pull = PORT_PIN_PULL_UP;
	port_pin_set_config(BUTTON_1_PIN, &config_port_pin);
	port_pin_set_config(BUTTON_2_PIN, &config_port_pin);
	port_pin_set_config(BUTTON_3_PIN, &config_port_pin);
	
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(SHDN_PIN, &config_port_pin);
		
	port_pin_set_output_level(SHDN_PIN, true);
	
	port_pin_set_config(LED_PIN, &config_port_pin);
	
	port_pin_set_output_level(LED_PIN, true);
	
	SSD1331Init();

	display.fillScreen(Color(80,0,60).Color565());
	SSD1331Refresh(display.width(), display.height(), display.getData());
	
	while (1) {
		stream.ProcessOp();
		
		
		bool button1 = !port_pin_get_input_level(BUTTON_1_PIN);
		bool button2 = !port_pin_get_input_level(BUTTON_2_PIN);
		bool button3 = !port_pin_get_input_level(BUTTON_3_PIN);
		
		uint8_t newButtons = (button3 << 2) | (button2 << 1) | button1;
		
		for (int i=0; i < 3; i++) {
			uint8_t mask = (1 << i);
			if ((newButtons & mask) != (buttons & mask)) {
				if (newButtons & mask) {
					buttons |= mask;
					buttonsPressed |= mask;
				} else {
					buttons &= ~mask;
					buttonsReleased |= mask;
				}
			}
		}
	}
}
