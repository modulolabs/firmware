/*
 * OLEDModule.cpp
 *
 * Created: 6/20/2014 10:52:22 PM
 *  Author: ekt
 */ 
#include "Config.h"


#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_OLED)

#include <avr/io.h>
#include <avr/interrupt.h>
#include "OLED/Adafruit_SSD1306.h"
#include "Modulo.h"
#include "Clock.h"
#include <avr/delay.h>

/*
  LED - PA0
  A   - GPIO 4 - PA3
  COM - GPIO 3 - PA7
  B   - GPIO 2 - PB2
*/

static const uint8_t DISPLAY_WIDTH = 128;
static const uint8_t DISPLAY_HEIGHT = 64;

const char *ModuloDeviceType = "co.modulo.MiniDisplay";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "MiniDisplay";
const char *ModuloDocURL = "modulo.co/docs/MiniDisplay";

static volatile uint16_t pixelIndex = 0;

Adafruit_SSD1306 oled;

/*
void _WriteModuloValue(uint8_t functionID, const ModuloBuffer &buffer) {
	switch (functionID) {
		case 3: // Index
			pixelIndex = buffer.Get<uint16_t>();
			break;
	}
}

void _BeginWriteArray(uint8_t functionID, uint8_t arrayLength) {	
	oled.BeginSetPixels(pixelIndex % DISPLAY_WIDTH, pixelIndex / DISPLAY_WIDTH);
}

void _WriteArrayData(uint8_t functionID, uint8_t index, uint8_t data) {
	pixelIndex++;
	oled.fastSPIwrite(data);
}

void _EndWriteArray(uint8_t functionID) {
	oled.EndSetPixels();
}
*/

#define SET_PIXELS_COMMAND 0
#define GET_BUTTONS_COMMAND 1

#define USE_PIXEL_BUFFER 0
volatile bool pixelBufferEmpty= true;
volatile uint8_t row, column;
volatile uint8_t pixelBuffer[16];

void ModuloReset() {
    oled.clear();
	PUEB |= _BV(0) | _BV(1);
	PUEA |= _BV(0);
}

uint8_t _getButtons() {
    bool button0 = !(PINB & _BV(0));
    bool button1 = !(PINB & _BV(1));
    bool button2 = !(PINA & _BV(0));
    return (button0 | (button1 << 1) | (button2 << 2));
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    switch(command) {
    case GET_BUTTONS_COMMAND:
        buffer->AppendValue<uint8_t>(_getButtons());
        return true;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    switch (buffer.GetCommand()) {
    case GET_BUTTONS_COMMAND:
        return true;
    case SET_PIXELS_COMMAND:
        if (buffer.GetSize() != 18) {
            return false;
        }

#if USE_PIXEL_BUFFER
        if (pixelBufferEmpty) {
            row = buffer.Get<uint8_t>(1);
            column = buffer.Get<uint8_t>(0);
            for(int i=2; i < 18; i++) {
                pixelBuffer[i-2] = buffer.Get<uint8_t>(i);
            }
            pixelBufferEmpty = false;
        }

#else
        oled.BeginSetPixels(buffer.Get<uint8_t>(1), buffer.Get<uint8_t>(0));
        for(int i=2; i < 18; i++) {
            oled.fastSPIwrite(buffer.Get<uint8_t>(i));
        }
        oled.EndSetPixels();
#endif

        return true;
        
    }
    return false;
}


int main(void)
{
	
	ClockInit();
	ModuloInit(&DDRA, &PORTA, _BV(5));
    
    oled.clear();

    //for (int i =0; i < 21*8; i++) {
    //	oled.drawChar(i % 21, i/21, i);
    //}
	while (1) {
#if USE_PIXEL_BUFFER
            oled.BeginSetPixels(row, column);
            for(int i=0; i < 16; i++) {
                oled.fastSPIwrite(pixelBuffer[i]);
            }
            oled.EndSetPixels();
            pixelBufferEmpty = true;
#endif
	}
}

#endif