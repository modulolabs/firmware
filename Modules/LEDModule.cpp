/*
 * Firmware.cpp
 *
 * Created: 5/10/2014 8:08:46 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_LED)

#include <avr/io.h>
#include "TwoWire.h"
#include "NeoPixel.h"

Adafruit_NeoPixel strip = Adafruit_NeoPixel(1, NEO_GRB + NEO_KHZ800);

static volatile bool colorChanged;
static volatile uint8_t color[3];

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {
	if (numBytes >= 3) {
		colorChanged = true;
		color[0] = data[0];
		color[1] = data[1];
		color[2] = data[2];
	}
}

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived);
	Init();
	
	while(1)
	{
		if (colorChanged) {
			colorChanged = false;
			strip.begin();
			//strip.setPixelColor(0, Wheel(count/100));
			strip.setPixelColor(0, strip.Color(color[0], color[1], color[2]));
			strip.show();
		}
	}
}

#endif
