/*********************************************************************
This is a library for our Monochrome OLEDs based on SSD1306 drivers

  Pick one up today in the adafruit shop!
  ------> http://www.adafruit.com/category/63_98

These displays use SPI to communicate, 4 or 5 pins are required to  
interface

Adafruit invests time and resources providing this open source code, 
please support Adafruit and open-source hardware by purchasing 
products from Adafruit!

Written by Limor Fried/Ladyada  for Adafruit Industries.  
BSD license, check license.txt for more information
All text above, and the splash screen below must be included in any redistribution
*********************************************************************/

#include "Config.h"

#include <avr/pgmspace.h>
#include <util/delay.h>
#include <stdlib.h>
#include <string.h>


#include "Adafruit_SSD1306.h"
extern const unsigned char font[] PROGMEM;


// initializer for I2C - we only indicate the reset pin!
Adafruit_SSD1306::Adafruit_SSD1306()  {
	hwSPI = true;
	csport = &PORTA; // GPIO 1
	cspinmask = _BV(2);
		
	mosiport = &PORTA; // GPIO 2
	mosipinmask = _BV(1);
		
	clkport = &PORTA; // GPIO 0
	clkpinmask = _BV(3);
		

	
	resetport = &PORTB; // GPIO 7
	dcport = &PORTB; // GPIO 6

	bool isAdafruitBoard = false;
	if (isAdafruitBoard) {
		resetpinmask = _BV(0);
		dcpinmask = _BV(1);
	} else {
		resetpinmask = _BV(1);
		dcpinmask = _BV(0);
	}
}
  

void Adafruit_SSD1306::begin(bool reset, uint8_t vccstate) {
	_vccstate = vccstate;

	DDRA |= cspinmask | mosipinmask | clkpinmask;
	DDRB |= resetpinmask | dcpinmask;

    //SPI.begin ();
    //SPI.setClockDivider (SPI_CLOCK_DIV2); // 8 MHz
	// Initialize SPI
	if (hwSPI) {
		REMAP |= _BV(SPIMAP);
		SPCR |= _BV(CPOL) | _BV(CPHA);
		SPCR |= _BV(MSTR);
		SPCR |= _BV(SPE);
		
		// Simulate completion of a previous write
		//SPSR |= _BV(SPIF);
		SPDR = 0;
	}
	
  if (reset) {
    // Setup reset pin direction (used by both SPI and I2C)  
    //pinMode(rst, OUTPUT);
    //digitalWrite(rst, HIGH);
	*resetport |= resetpinmask;
	
    // VDD (3.3V) goes high at start, lets just chill for a ms
    _delay_ms(1);
	
    // bring reset low
    //digitalWrite(rst, LOW);
    *resetport &= ~resetpinmask;
	
	// wait 10ms
    _delay_ms(10);
    
	// bring out of reset
    //digitalWrite(rst, HIGH);
	*resetport |= resetpinmask;
	
    // turn on VCC (9V?)
  }

   #if defined SSD1306_128_32
    // Init sequence for 128x32 OLED module
    ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_command(0x80);                                  // the suggested ratio 0x80
    ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_command(0x1F);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_command(0x0);                                   // no offset
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
    ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x10); }
    else 
      { ssd1306_command(0x14); }
    ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_command(0x00);                                  // 0x0 act like ks0108
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_command(0x02);
    ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
    ssd1306_command(0x8F);
    ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x22); }
    else 
      { ssd1306_command(0xF1); }
    ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6
  #endif

  #if defined SSD1306_128_64
    // Init sequence for 128x64 OLED module
    ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_command(0x80);                                  // the suggested ratio 0x80
    ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_command(0x3F);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_command(0x0);                                   // no offset
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
    ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x10); }
    else 
      { ssd1306_command(0x14); }
    ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_command(0x00);                                  // 0x0 act like ks0108
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_command(0x12);
    ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x9F); }
    else 
      { ssd1306_command(0xFF); }
    ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x22); }
    else 
      { ssd1306_command(0xF1); }
    ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6
  #endif

  #if defined SSD1306_96_16
    // Init sequence for 96x16 OLED module
    ssd1306_command(SSD1306_DISPLAYOFF);                    // 0xAE
    ssd1306_command(SSD1306_SETDISPLAYCLOCKDIV);            // 0xD5
    ssd1306_command(0x80);                                  // the suggested ratio 0x80
    ssd1306_command(SSD1306_SETMULTIPLEX);                  // 0xA8
    ssd1306_command(0x0F);
    ssd1306_command(SSD1306_SETDISPLAYOFFSET);              // 0xD3
    ssd1306_command(0x00);                                   // no offset
    ssd1306_command(SSD1306_SETSTARTLINE | 0x0);            // line #0
    ssd1306_command(SSD1306_CHARGEPUMP);                    // 0x8D
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x10); }
    else 
      { ssd1306_command(0x14); }
    ssd1306_command(SSD1306_MEMORYMODE);                    // 0x20
    ssd1306_command(0x00);                                  // 0x0 act like ks0108
    ssd1306_command(SSD1306_SEGREMAP | 0x1);
    ssd1306_command(SSD1306_COMSCANDEC);
    ssd1306_command(SSD1306_SETCOMPINS);                    // 0xDA
    ssd1306_command(0x2);	//ada x12
    ssd1306_command(SSD1306_SETCONTRAST);                   // 0x81
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x10); } // 0x10
    else 
      { ssd1306_command(0xAF); } // 0xAF
    ssd1306_command(SSD1306_SETPRECHARGE);                  // 0xd9
    if (vccstate == SSD1306_EXTERNALVCC) 
      { ssd1306_command(0x22); }
    else 
      { ssd1306_command(0xF1); }
    ssd1306_command(SSD1306_SETVCOMDETECT);                 // 0xDB
    ssd1306_command(0x40);
    ssd1306_command(SSD1306_DISPLAYALLON_RESUME);           // 0xA4
    ssd1306_command(SSD1306_NORMALDISPLAY);                 // 0xA6
  #endif

  ssd1306_command(SSD1306_DISPLAYON);//--turn on oled panel
}


void Adafruit_SSD1306::invertDisplay(uint8_t i) {
  if (i) {
    ssd1306_command(SSD1306_INVERTDISPLAY);
  } else {
    ssd1306_command(SSD1306_NORMALDISPLAY);
  }
}

void Adafruit_SSD1306::ssd1306_command(uint8_t c) { 
    // SPI
    //digitalWrite(cs, HIGH);
    *csport |= cspinmask;
    //digitalWrite(dc, LOW);
    *dcport &= ~dcpinmask;
    //digitalWrite(cs, LOW);
    *csport &= ~cspinmask;
    fastSPIwrite(c);
    //digitalWrite(cs, HIGH);
    *csport |= cspinmask;
}

// startscrollright
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void Adafruit_SSD1306::startscrollright(uint8_t start, uint8_t stop){
  ssd1306_command(SSD1306_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0x00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrollleft
// Activate a right handed scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void Adafruit_SSD1306::startscrollleft(uint8_t start, uint8_t stop){
  ssd1306_command(SSD1306_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X00);
  ssd1306_command(0XFF);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagright
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void Adafruit_SSD1306::startscrolldiagright(uint8_t start, uint8_t stop){
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);  
  ssd1306_command(0X00);
  ssd1306_command(SSD1306_LCDHEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_RIGHT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

// startscrolldiagleft
// Activate a diagonal scroll for rows start through stop
// Hint, the display is 16 rows tall. To scroll the whole display, run:
// display.scrollright(0x00, 0x0F) 
void Adafruit_SSD1306::startscrolldiagleft(uint8_t start, uint8_t stop){
  ssd1306_command(SSD1306_SET_VERTICAL_SCROLL_AREA);  
  ssd1306_command(0X00);
  ssd1306_command(SSD1306_LCDHEIGHT);
  ssd1306_command(SSD1306_VERTICAL_AND_LEFT_HORIZONTAL_SCROLL);
  ssd1306_command(0X00);
  ssd1306_command(start);
  ssd1306_command(0X00);
  ssd1306_command(stop);
  ssd1306_command(0X01);
  ssd1306_command(SSD1306_ACTIVATE_SCROLL);
}

void Adafruit_SSD1306::stopscroll(void){
  ssd1306_command(SSD1306_DEACTIVATE_SCROLL);
}

// Dim the display
// dim = true: display is dimmed
// dim = false: display is normal
void Adafruit_SSD1306::setContract(uint8_t contrast) {
  // the range of contrast to too small to be really useful
  // it is useful to dim the display
  ssd1306_command(SSD1306_SETCONTRAST);
  ssd1306_command(contrast);
}

void Adafruit_SSD1306::ssd1306_data(uint8_t c) {
    // SPI
    //digitalWrite(cs, HIGH);
    *csport |= cspinmask;
    //digitalWrite(dc, HIGH);
    *dcport |= dcpinmask;
    //digitalWrite(cs, LOW);
    *csport &= ~cspinmask;
	
    fastSPIwrite(c);
    
	//digitalWrite(cs, HIGH);
    *csport |= cspinmask;
}

void Adafruit_SSD1306::BeginSetPixels(int column, int page)
{
  ssd1306_command(SSD1306_COLUMNADDR);
  ssd1306_command(column);   // Column start address (0 = reset)
  ssd1306_command(SSD1306_LCDWIDTH-1); // Column end address (127 = reset)

  ssd1306_command(SSD1306_PAGEADDR);
  ssd1306_command(page); // Page start address (0 = reset)
  #if SSD1306_LCDHEIGHT == 64
  ssd1306_command(7); // Page end address
  #endif
  #if SSD1306_LCDHEIGHT == 32
  ssd1306_command(3); // Page end address
  #endif
  #if SSD1306_LCDHEIGHT == 16
  ssd1306_command(1); // Page end address
  #endif

  // SPI
  *csport |= cspinmask;
  *dcport |= dcpinmask;
  *csport &= ~cspinmask;
}

void Adafruit_SSD1306::EndSetPixels()
{
	*csport |= cspinmask;
}

void Adafruit_SSD1306::display(uint8_t val) {


#if 0
	for (uint8_t page=0; page < 8; page++) {
		for (uint8_t x=0; x < SSD1306_LCDWIDTH; x++) {
			uint8_t data;
			for (uint8_t y=page*8; y < (page+1)*8; y++) {
				bool bit = false;
				if (x == y+val) {
					bit = true;
				}
				data >>= 1;
				if (bit) {
					data |= 0x80;
				}
			}
			fastSPIwrite(data);
		}
	}
#endif
#if 0

    for (uint16_t i=0; i<(SSD1306_LCDWIDTH*SSD1306_LCDHEIGHT/8); i++) {
		//if ( i < SSD1306_LCDWIDTH*1.5) {
			fastSPIwrite(1);
		//} else {
		//	fastSPIwrite(0);
		//}
    //  fastSPIwrite('U');
      //ssd1306_data(buffer[i]);
    }
	
#endif

	drawChar(0, 0, 'e', 0, 0, 1);
	drawChar(0, 0, 'k', 0, 0, 1);
	drawChar(0, 0, 't', 0, 0, 1);
	drawChar(0, 0, ' ', 0, 0, 1);
	drawChar(0, 0, 'w', 0, 0, 1);
	drawChar(0, 0, 'a', 0, 0, 1);
	drawChar(0, 0, 's', 0, 0, 1);
	drawChar(0, 0, ' ', 0, 0, 1);
	drawChar(0, 0, 'h', 0, 0, 1);
	drawChar(0, 0, 'e', 0, 0, 1);
	drawChar(0, 0, 'r', 0, 0, 1);
	drawChar(0, 0, 'e', 0, 0, 1);
	drawChar(0, 0, '!', 0, 0, 1);
	
    *csport |= cspinmask;
}

void Adafruit_SSD1306::clear() {
	BeginSetPixels(0,0);


	for (int i=0; i < 128*8; i++) {
		fastSPIwrite(0);
	}

    EndSetPixels();
}

// Draw a character
void Adafruit_SSD1306::drawChar(int16_t x, int16_t y, unsigned char c,
	uint16_t color, uint16_t bg, uint8_t size) {
	BeginSetPixels(6*x+1, y);

	for (int i=0; i<6; i++ ) {
		uint8_t line;
		if (i == 5)
			line = 0x0;
		else
			line = pgm_read_byte(font+(c*5)+i);
		
		fastSPIwrite(line);
	}

    EndSetPixels();
}

void Adafruit_SSD1306::drawString(int x, int y, const char *s)
{
	uint8_t line;
	BeginSetPixels(6*x+1, y);
	for (int i=0; s[i] != 0 and i+x < 21; i++) {
		for (int j=0; j<6; j++ ) {
			if (j == 5)
				line = 0x0;
			else
				line = pgm_read_byte(font+(s[i]*5)+j);
			fastSPIwrite(line);
		}
	}
	EndSetPixels();	
}

void Adafruit_SSD1306::fastSPIwrite(uint8_t d) {
	SPDR = d;
	while (!(SPSR & _BV(SPIF))) {
	}
}
