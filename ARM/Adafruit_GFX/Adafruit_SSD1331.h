/*************************************************** 
  This is a library for the 0.96" 16-bit Color OLED with SSD1331 driver chip

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/products/684

  These displays use SPI to communicate, 4 or 5 pins are required to  
  interface
  Adafruit invests time and resources providing this open source code, 
  please support Adafruit and open-source hardware by purchasing 
  products from Adafruit!

  Written by Limor Fried/Ladyada for Adafruit Industries.  
  BSD license, all text above must be included in any redistribution
 ****************************************************/
#include "Adafruit_GFX.h"


class Adafruit_SSD1331  : public Adafruit_GFX {
 public:
  Adafruit_SSD1331();

  void begin(void);
  void refresh();


 private:
	void writeData(uint8_t d);
	void writeCommand(uint8_t c);
	void writeByte(uint8_t d, bool isData);
	
    void spiwrite(uint8_t);
};
