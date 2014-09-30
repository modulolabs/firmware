

// http://codinglab.blogspot.com/2008/10/i2c-on-avr-using-bit-banging.html
#include "Config.h"
#include "Clock.h"
#include <avr/io.h>
#include <avr/delay.h>

// Port for the I2C
#define I2C_DDR DDRB
#define I2C_PIN PINB
#define I2C_PORT PORTB

// Pins to be used in the bit banging
#define I2C_CLK 0
#define I2C_DAT 1

#define I2C_DATA_HI()\
I2C_DDR &= ~ (1 << I2C_DAT);

#define I2C_DATA_LO()\
I2C_DDR |= (1 << I2C_DAT);

#define I2C_CLOCK_HI()\
I2C_DDR &= ~ (1 << I2C_CLK);

#define I2C_CLOCK_LO()\
I2C_DDR |= (1 << I2C_CLK);


// Inits bitbanging port, must be called before using the functions below
//
void I2C_Init()
{
	PUEB |= _BV(0) | _BV(1);
	I2C_PORT &= ~ ((1 << I2C_DAT) | (1 << I2C_CLK));

	I2C_CLOCK_HI();
	I2C_DATA_HI();

	delay_us(1);
}


// Send a START Condition
//
void I2C_Start()
{
	// set both to high at the same time
	I2C_DDR &= ~ ((1 << I2C_DAT) | (1 << I2C_CLK));
	delay_us(1);

	I2C_DATA_LO();
	delay_us(1);

	I2C_CLOCK_LO();
	delay_us(1);
}

// Send a STOP Condition
//
void I2C_Stop()
{
	I2C_DATA_LO();
	delay_us(1);
	
	I2C_CLOCK_HI();
	delay_us(1);

	I2C_DATA_HI();
	delay_us(1);
}


void I2C_WriteBit(unsigned char c)
{
	if (c > 0)
	{
		I2C_DATA_HI();
	}
	else
	{
		I2C_DATA_LO();
	}
	delay_us(1);
	
	// SDA is not allowed to change while SCL is high
	I2C_CLOCK_HI();
	
	while (!(I2C_PIN & _BV(I2C_CLK))) {
	}
	delay_us(1);
	
	I2C_CLOCK_LO();
	delay_us(1);
	
	if (c > 0)
    {
        I2C_DATA_LO();
    }

    delay_us(1);
}

unsigned char I2C_ReadBit()
{
	I2C_DATA_HI();
	
	I2C_CLOCK_HI();
	
	while (!(I2C_PIN & _BV(I2C_CLK))) {
	}
	delay_us(1);

	bool bit = I2C_PIN & _BV(I2C_DAT);

	I2C_CLOCK_LO();
	delay_us(1);

	return bit;
}





// write a byte to the I2C slave device
//
unsigned char I2C_Write(unsigned char c)
{
	for (char i = 0; i < 8; i++)
	{
		I2C_WriteBit(c & 128);

		c <<= 1;
	}

	//I2C_DATA_HI();
	bool retval = I2C_ReadBit();
	delay_us(3);
	return retval;
	//return 0;
}


// read a byte from the I2C slave device
//
unsigned char I2C_Read(unsigned char ack)
{
	unsigned char res = 0;

	for (char i = 0; i < 8; i++)
	{
		res <<= 1;
		res |= I2C_ReadBit();
	}

	if (ack > 0)
	{
		I2C_WriteBit(0);
	}
	else
	{
		I2C_WriteBit(1);
	}

	//I2C_DATA_HI();
	
	delay_us(3);

	return res;
}