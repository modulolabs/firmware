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



bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    switch (buffer.GetCommand()) {
    case SET_PIXELS_COMMAND:
        if (buffer.GetSize() != 18) {
            return false;
        }
        oled.BeginSetPixels(buffer.Get<uint8_t>(1), buffer.Get<uint8_t>(0));
        for(int i=2; i < 18; i++) {
            oled.fastSPIwrite(buffer.Get<uint8_t>(i));
        }
        oled.EndSetPixels();

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
	
	}
}

#endif