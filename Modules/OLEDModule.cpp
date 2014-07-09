/*
 * OLEDModule.cpp
 *
 * Created: 6/20/2014 10:52:22 PM
 *  Author: ekt
 */ 
#include "Setup.h"
#include "Adafruit_SSD1306.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_OLED)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include "TwoWire.h"


/*
  LED - PA0
  A   - GPIO 4 - PA3
  COM - GPIO 3 - PA7
  B   - GPIO 2 - PB2
*/

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {

}

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {

}


int main(void)
{
	Init();
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);

	
	Adafruit_SSD1306 oled;
	oled.begin();
	oled.display(0);
	oled.startscrollright(0x00, 0x0F) ;
			
	//uint8_t x = 0;
	while (1) {

		//oled.display(x);
		//x = (x+1) % SSD1306_LCDWIDTH;

		//_delay_ms(500);
	}
}

#endif