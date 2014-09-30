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


DEFINE_MODULO_CONSTANTS("Integer Labs", "Display", 0, "http://www.integerlabs.net/docs/Display");
DEFINE_MODULO_FUNCTION_NAMES("Width,Height,Depth,Index,Pixels");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeUInt16, ModuloDataTypeUInt16, ModuloDataTypeUInt8,
	ModuloDataTypeUInt16, ModuloDataTypeString);

static volatile uint16_t pixelIndex = 0;

Adafruit_SSD1306 oled;

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



int main(void)
{
	oled.begin();
	oled.clear();
	
	ClockInit();
	ModuloInit(&DDRB, &PORTB, _BV(2),
		0 /*readCallback*/,
		_WriteModuloValue,
		_BeginWriteArray,
		_WriteArrayData,
		_EndWriteArray);
	


//	for (int i =0; i < 21*8; i++) {
//		oled.drawChar(i % 21, i/21, i);
//	}

	//oled.display(0);
	//oled.startscrollright(0x00, 0x0F) ;
			
	//uint8_t x = 0;
	while (1) {

		//oled.display(0);
		//x = (x+1) % SSD1306_LCDWIDTH;
		//oled.invertDisplay(true);
		delay(500);
		//oled.invertDisplay(false);
		delay(500);
		
		//oled.clear();
		//oled.drawString(3, 3, "abcdefghijklmnopqrstuvwxyz");
	
	}
}

#endif