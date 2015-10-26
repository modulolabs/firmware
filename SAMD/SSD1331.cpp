/*
 * This code drives the SSD1331 chip in the OLED display module.
 * It was originally based on the Adafruit SSD1331 library, but has
 * since been largely rewritten.
 */

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
#include <math.h>
#include <asf.h>


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
#define SSD1331_CMD_SETTABLE        0xB8
#define SSD1331_CMD_PRECHARGEA 		0x8A
#define SSD1331_CMD_PRECHARGEB 		0x8B
#define SSD1331_CMD_PRECHARGEC 		0x8C
#define SSD1331_CMD_PRECHARGELEVEL 	0xBB
#define SSD1331_CMD_VCOMH 			0xBE

#define _BV(x) (1 << x)

#define RD_PIN 24
#define WR_PIN 9
#define DC_PIN 10
#define RESET_PIN 11
#define CS_PIN 14
#define BS1_PIN 17
#define BS2_PIN 16

static void _setCommandMode() {
	PORTA.OUTCLR.reg = _BV(DC_PIN);
}

static void _setDataMode() {
	PORTA.OUTSET.reg = _BV(DC_PIN);
}

static inline void _writeByte(uint8_t x) {
	// Pull the CS pin, WR pin, and data pins low.
	PORTA.OUTCLR.reg = _BV(CS_PIN) | _BV(WR_PIN) | 0xFF;
	
	// Drive the data pins
	PORTA.OUTSET.reg = x;
	
	// Bring WR high to latch the data.
	PORTA.OUTSET.reg = _BV(WR_PIN);	
}

void SSD1331Refresh(uint8_t width, uint8_t height, uint8_t *data) {
	// set x and y coordinate
	_setCommandMode();
	_writeByte(SSD1331_CMD_SETCOLUMN);
	_writeByte(0);
	_writeByte(width-1);

	_writeByte(SSD1331_CMD_SETROW);
	_writeByte(0);
	_writeByte(height-1);

	_setDataMode();
	
	// Unrolling this loop gets us from 6 instruction per byte
	// to approx 4.2 instructions per byte.
	for (int i = width*height*2-16; i >= 0; i -= 16) {
		_writeByte(data[i+15]);
		_writeByte(data[i+14]);
		_writeByte(data[i+13]);
		_writeByte(data[i+12]);
		_writeByte(data[i+11]);
		_writeByte(data[i+10]);
		_writeByte(data[i+9]);
		_writeByte(data[i+8]);
		_writeByte(data[i+7]);
		_writeByte(data[i+6]);
		_writeByte(data[i+5]);
		_writeByte(data[i+4]);
		_writeByte(data[i+3]);
		_writeByte(data[i+2]);
		_writeByte(data[i+1]);
		_writeByte(data[i]);	
	}	
}

void SSD1331Init() {
	struct port_config config_port_pin;
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

	// Drive the data pins
	config_port_pin.direction = PORT_PIN_DIR_OUTPUT;
	port_group_set_config(&PORTA, 0xFF, &config_port_pin);

    // Initialization Sequence
	_setCommandMode();
    _writeByte(SSD1331_CMD_DISPLAYOFF);  	// 0xAE
    
	_writeByte(SSD1331_CMD_SETREMAP); 	// 0xA0
    _writeByte(0x72);				// RGB Color

    _writeByte(SSD1331_CMD_STARTLINE); 	// 0xA1
    _writeByte(0x0);
    
	_writeByte(SSD1331_CMD_DISPLAYOFFSET); 	// 0xA2
    _writeByte(0x0);
    
	_writeByte(SSD1331_CMD_NORMALDISPLAY);  	// 0xA4
    
	_writeByte(SSD1331_CMD_SETMULTIPLEX); 	// 0xA8
    _writeByte(0x3F);  			// 0x3F 1/64 duty
    
	_writeByte(SSD1331_CMD_SETMASTER);  	// 0xAD
    _writeByte(0x8E);
    
	_writeByte(SSD1331_CMD_POWERMODE);  	// 0xB0
    _writeByte(0x0B);

    _writeByte(SSD1331_CMD_CLOCKDIV);  	// 0xB3
    _writeByte(0xF0);  // 7:4 = Oscillator Frequency, 3:0 = CLK Div Ratio (A[3:0]+1 = 1..16)
	
	_writeByte(SSD1331_CMD_PRECHARGE);  	// 0xB1
    _writeByte(0x31); // high four bits are the charge period, low four bits are discharge period, 
	
	_writeByte(SSD1331_CMD_PRECHARGELEVEL);  	// 0xBB
    _writeByte(0x30);
	
    _writeByte(SSD1331_CMD_PRECHARGEA);  	// 0x8A
    _writeByte(0x80);
    
	_writeByte(SSD1331_CMD_PRECHARGEB);  	// 0x8B
    _writeByte(0x80);
	
    _writeByte(SSD1331_CMD_PRECHARGEC);  	// 0x8B
    _writeByte(0x80);

    _writeByte(SSD1331_CMD_VCOMH);  		// 0xBE
    _writeByte(0x3E);

    _writeByte(SSD1331_CMD_MASTERCURRENT);  // 0x87
	_writeByte(15);

    _writeByte(SSD1331_CMD_CONTRASTA);  	// 0x81
    _writeByte(0xFF); // blue
	
    _writeByte(SSD1331_CMD_CONTRASTB);  	// 0x82
    _writeByte(0x8D); // green
	
    _writeByte(SSD1331_CMD_CONTRASTC);  	// 0x83
    _writeByte(0xED); // red

	_writeByte(SSD1331_CMD_SETTABLE);
	float gamma = 1.5;
	if (gamma != 1.0) {
		for  (int i=0; i < 32; i++) {
			uint8_t val = pow(i/32.0, gamma)*127;
			_writeByte(val);
		}
	} else {
		_writeByte(0xB9); // Linear Table
	}

    _writeByte(SSD1331_CMD_DISPLAYON);	//--turn on oled panel
}

