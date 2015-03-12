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

#include <inttypes.h>

#ifdef __SAM3X8E__
typedef volatile RwReg PortReg;
typedef uint32_t PortMask;
#define _BV(b) (1<<(b))
#else
typedef volatile uint8_t PortReg;
typedef uint8_t PortMask;
#endif

// Select one of these defines to set the pixel color order
#define SSD1331_COLORORDER_RGB
// #define SSD1331_COLORORDER_BGR

#if defined SSD1331_COLORORDER_RGB && defined SSD1331_COLORORDER_BGR
#error "RGB and BGR can not both be defined for SSD1331_COLORODER."
#endif

// Timing Delays
#define SSD1331_DELAYS_HWFILL		(3)
#define SSD1331_DELAYS_HWLINE       (1)

// SSD1331 Commands
#define SSD1331_CMD_DRAWLINE 		0x21
#define SSD1331_CMD_DRAWRECT 		0x22
#define SSD1331_CMD_FILL 			0x26
#define SSD1331_CMD_SETCOLUMN 		0x15
#define SSD1331_CMD_SETROW    		0x75
#define SSD1331_CMD_CONTRASTA 		0x81
#define SSD1331_CMD_CONTRASTB 		0x82
#define SSD1331_CMD_CONTRASTC		0x83
#define SSD1331_CMD_MASTERCURRENT 	0x87
#define SSD1331_CMD_SETREMAP 		0xA0
#define SSD1331_CMD_STARTLINE 		0xA1
#define SSD1331_CMD_DISPLAYOFFSET 	0xA2
#define SSD1331_CMD_NORMALDISPLAY 	0xA4
#define SSD1331_CMD_DISPLAYALLON  	0xA5
#define SSD1331_CMD_DISPLAYALLOFF 	0xA6
#define SSD1331_CMD_INVERTDISPLAY 	0xA7
#define SSD1331_CMD_SETMULTIPLEX  	0xA8
#define SSD1331_CMD_SETMASTER 		0xAD
#define SSD1331_CMD_DISPLAYOFF 		0xAE
#define SSD1331_CMD_DISPLAYON     	0xAF
#define SSD1331_CMD_POWERMODE 		0xB0
#define SSD1331_CMD_PRECHARGE 		0xB1
#define SSD1331_CMD_CLOCKDIV 		0xB3
#define SSD1331_CMD_PRECHARGEA 		0x8A
#define SSD1331_CMD_PRECHARGEB 		0x8B
#define SSD1331_CMD_PRECHARGEC 		0x8C
#define SSD1331_CMD_PRECHARGELEVEL 	0xBB
#define SSD1331_CMD_VCOMH 			0xBE

#include "Adafruit_SSD1331.h"
#include "glcdfont.c"
#include <asf.h>

#define _BV(x) (1 << x)

/********************************** low level pin interface */

#define RD_PIN 8
#define WR_PIN 9
#define DC_PIN 10
#define RESET_PIN 11
#define CS_PIN 14
#define BS1_PIN 16
#define BS2_PIN 17

struct port_config config_port_pin;

void Adafruit_SSD1331::writeByte(uint8_t byte, bool isData) {

	// Set DC high for data, low for command
	port_pin_set_output_level(DC_PIN, isData);
	
	// Pull CS low (active)
	port_pin_set_output_level(CS_PIN, false);
	
	// Bring WR low
	port_pin_set_output_level(WR_PIN, false);
	
	// Drive the data pins
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_group_set_config(&PORTA, 0xFF, &config_port_pin);
	port_group_set_output_level(&PORTA, 0xFF, byte);
	
	// Bring WR high. This is the latch command
	port_pin_set_output_level(WR_PIN, true);
	
	// Stop driving data pins
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_group_set_config(&PORTA, 0xFF, &config_port_pin);
	
	// Pull CS high (inactive)
	port_pin_set_output_level(CS_PIN, true);
}


void Adafruit_SSD1331::writeCommand(uint8_t c) {
	writeByte(c, false);
}


void Adafruit_SSD1331::writeData(uint8_t c) {
	writeByte(c, true);
} 


/*
void Adafruit_SSD1331::drawLine(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color) {	
  // check rotation, move pixel around if necessary
  switch (getRotation()) {
  case 1:
    swap(x0, y0);
    swap(x1, y1);
    x0 = TFTWIDTH - x0 - 1;
    x1 = TFTWIDTH - x1 - 1;
    break;
  case 2:
    x0 = TFTWIDTH - x0 - 1;
    y0 = TFTHEIGHT - y0 - 1;
    x1 = TFTWIDTH - x1 - 1;
    y1 = TFTHEIGHT - y1 - 1;
    break;
  case 3:
    swap(x0, y0);
    swap(x1, y1);
    y0 = TFTHEIGHT - y0 - 1;
    y1 = TFTHEIGHT - y1 - 1;
    break;
  }

  // Boundary check
  if ((y0 >= TFTHEIGHT) && (y1 >= TFTHEIGHT))
	return;
  if ((x0 >= TFTWIDTH) && (x1 >= TFTWIDTH))
	return;	
  if (x0 >= TFTWIDTH)
    x0 = TFTWIDTH - 1;
  if (y0 >= TFTHEIGHT)
    y0 = TFTHEIGHT - 1;
  if (x1 >= TFTWIDTH)
    x1 = TFTWIDTH - 1;
  if (y1 >= TFTHEIGHT)
    y1 = TFTHEIGHT - 1;
  

  writeCommand(SSD1331_CMD_DRAWLINE);
  writeCommand(x0);
  writeCommand(y0);
  writeCommand(x1);
  writeCommand(y1);
  //delay_ms(SSD1331_DELAYS_HWLINE);  
  writeCommand((uint8_t)((color >> 11) << 1));
  writeCommand((uint8_t)((color >> 5) & 0x3F));
  writeCommand((uint8_t)((color << 1) & 0x3F));
  //delay_ms(SSD1331_DELAYS_HWLINE);

}
*/


void Adafruit_SSD1331::refresh() {
	// set x and y coordinate
	writeCommand(SSD1331_CMD_SETCOLUMN);
	writeCommand(0);
	writeCommand(WIDTH-1);

	writeCommand(SSD1331_CMD_SETROW);
	writeCommand(0);
	writeCommand(HEIGHT-1);
	
	// Set DC high for data, low for command
	port_pin_set_output_level(DC_PIN, true);
	
	// Pull CS low (active)
	port_pin_set_output_level(CS_PIN, false);

	// Drive the data pins
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_group_set_config(&PORTA, 0xFF, &config_port_pin);

	for (int i=0; i < WIDTH*HEIGHT; i++) {
		uint16_t color = _pixels[i];
			// Bring WR low
		port_pin_set_output_level(WR_PIN, false);	
	
		port_group_set_output_level(&PORTA, 0xFF, color >> 8);
	
		// Bring WR high. This is the latch command
		port_pin_set_output_level(WR_PIN, true);
	
		// Bring WR low
		port_pin_set_output_level(WR_PIN, false);	
	
		port_group_set_output_level(&PORTA, 0xFF, color & 0xFF);
	
		// Bring WR high. This is the latch command
		port_pin_set_output_level(WR_PIN, true);
	}
	
	// Stop driving data pins
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_group_set_config(&PORTA, 0xFF, &config_port_pin);
	
	// Pull CS high (inactive)
	port_pin_set_output_level(CS_PIN, true);
	
}

void Adafruit_SSD1331::begin() {

	
	port_get_config_defaults(&config_port_pin);

		
	// Configure RD, WR, D/C, RESET, and CS as outputs
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_pin_set_config(RD_PIN, &config_port_pin);
	port_pin_set_config(WR_PIN, &config_port_pin);
	port_pin_set_config(DC_PIN, &config_port_pin);
	port_pin_set_config(RESET_PIN, &config_port_pin);
	port_pin_set_config(CS_PIN, &config_port_pin);
	port_pin_set_config(BS1_PIN, &config_port_pin);
	port_pin_set_config(BS2_PIN, &config_port_pin);
		
	// Pull reset pin low (active)
	port_pin_set_output_level(RESET_PIN, false);
		
	// Set RD, and WR high. (inactive)
	port_pin_set_output_level(RD_PIN, true);
	port_pin_set_output_level(WR_PIN, true);
		
	// Use 8080 interface mode. BS1 & B2 both high.
	port_pin_set_output_level(BS1_PIN, true);
	port_pin_set_output_level(BS2_PIN, true);

	// Pull reset pin high (inactive)
	port_pin_set_output_level(RESET_PIN, true);


    // Initialization Sequence
    writeCommand(SSD1331_CMD_DISPLAYOFF);  	// 0xAE
    writeCommand(SSD1331_CMD_SETREMAP); 	// 0xA0
#if defined SSD1331_COLORORDER_RGB
    writeCommand(0x72);				// RGB Color
#else
    writeCommand(0x76);				// BGR Color
#endif
    writeCommand(SSD1331_CMD_STARTLINE); 	// 0xA1
    writeCommand(0x0);
    writeCommand(SSD1331_CMD_DISPLAYOFFSET); 	// 0xA2
    writeCommand(0x0);
    writeCommand(SSD1331_CMD_NORMALDISPLAY);  	// 0xA4
    writeCommand(SSD1331_CMD_SETMULTIPLEX); 	// 0xA8
    writeCommand(0x3F);  			// 0x3F 1/64 duty
    writeCommand(SSD1331_CMD_SETMASTER);  	// 0xAD
    writeCommand(0x8E);
    writeCommand(SSD1331_CMD_POWERMODE);  	// 0xB0
    writeCommand(0x0B);
    writeCommand(SSD1331_CMD_PRECHARGE);  	// 0xB1
    writeCommand(0x31);
    writeCommand(SSD1331_CMD_CLOCKDIV);  	// 0xB3
    writeCommand(0xF0);  // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
    writeCommand(SSD1331_CMD_PRECHARGEA);  	// 0x8A
    writeCommand(0x64);
    writeCommand(SSD1331_CMD_PRECHARGEB);  	// 0x8B
    writeCommand(0x78);
    writeCommand(SSD1331_CMD_PRECHARGEA);  	// 0x8C
    writeCommand(0x64);
    writeCommand(SSD1331_CMD_PRECHARGELEVEL);  	// 0xBB
    writeCommand(0x3A);
    writeCommand(SSD1331_CMD_VCOMH);  		// 0xBE
    writeCommand(0x3E);
    writeCommand(SSD1331_CMD_MASTERCURRENT);  	// 0x87
    writeCommand(0x06);
    writeCommand(SSD1331_CMD_CONTRASTA);  	// 0x81
    writeCommand(0x91);
    writeCommand(SSD1331_CMD_CONTRASTB);  	// 0x82
    writeCommand(0x50);
    writeCommand(SSD1331_CMD_CONTRASTC);  	// 0x83
    writeCommand(0x7D);
    writeCommand(SSD1331_CMD_DISPLAYON);	//--turn on oled panel
}

/********************************* low level pin initialization */

Adafruit_SSD1331::Adafruit_SSD1331()  {

}

