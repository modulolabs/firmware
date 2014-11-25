#include "Config.h"
#include "Clock.h"
#include "I2CMaster.h"
#include <avr/io.h>
#include <util/delay.h>

// Port for the I2C
static uint8_t _sdaMask = 0;
static volatile uint8_t* _sdaPORT;
static volatile uint8_t* _sdaDDR;
static volatile uint8_t* _sdaPIN;


static uint8_t _sclMask = 0;
static volatile uint8_t* _sclPORT;
static volatile uint8_t* _sclDDR;
static volatile uint8_t* _sclPIN;

#define SDA_HIGH() (*_sdaDDR &= ~_sdaMask)
#define SDA_LOW() (*_sdaDDR |= _sdaMask);
#define SCL_HIGH() (*_sclDDR &= ~_sclMask)
#define SCL_LOW() (*_sclDDR |= _sclMask);

static void _start();
static bool _readBit();
static void _writeBit(bool bit);

void I2CInit(volatile uint8_t *sdaPORT, volatile uint8_t *sdaDDR, volatile uint8_t *sdaPIN, uint8_t sdaPin,
             volatile uint8_t *sclPORT, volatile uint8_t *sclDDR, volatile uint8_t *sclPIN, uint8_t sclPin) {

    // On the uCs we use, DDRX and PINX come right after PORTX in the register map
    // If we use some other uC, we need to check this.
    _sdaPORT = sdaPORT;
    _sdaDDR = sdaDDR;
    _sdaPIN = sdaPIN;

    _sclPORT = sclPORT;
    _sclDDR = sclDDR;
    _sclPIN = sclPIN;

    _sdaMask = _BV(sdaPin);
    _sclMask = _BV(sclPin);

    // Set the outputs low when DDR is enabled
    *_sdaPORT &= ~_sdaMask;
    *_sclPORT &= ~_sclMask;

    SDA_HIGH();
    SCL_HIGH();
}


bool I2CBegin(uint8_t address, bool read) {
    _start();
    return I2CWrite(address << 1 | read);
}

bool I2CWrite(uint8_t c) {
    for (char i = 0; i < 8; i++)
    {
        _writeBit(c & 128);

        c <<= 1;
    }


    return _readBit();
}


uint8_t I2CRead(bool ack) {
    uint8_t result = 0;

    for (char i = 0; i < 8; i++)
    {
        result <<= 1;
        result |= _readBit();
    }

    _writeBit(!ack);

    return result;
}


void I2CStop() {
    // Expects SCL low, SDA indeterminate

    // First ensure SDA is low
    SDA_LOW();

    // Rising SCL when SDA low indicates stop condition
    SCL_HIGH();

    // Release SDA
    SDA_HIGH();
}


static void _start() {
    // Expects SDA and SCL high

    // Falling SCL when SDA is low indicates stop condition
    SDA_LOW();
    SCL_LOW();
}

static void _writeBit(bool bit) {
    // Expects SCL low
    // SDA can only change while SCL is low

    if (bit) {
        SDA_HIGH();
    } else {
        SDA_LOW();
    }
	
    SCL_HIGH();

    // Wait for SCL to go high
    //while (!((*_sclPIN) & _sclMask)) {
    //}

    SCL_LOW();

    // Leaves SCL low, SDA indeterminate
}

static bool _readBit() {
    // Expects SCL low
    // SDA can only change while SCL is low

    // Ensure that we have released SDA
    SDA_HIGH();
	
    SCL_HIGH();

    // Wait for clock stretching
	//while (!(*_sclPIN & _sclMask)) {
	//}

	bool bit = *_sdaPIN & _sdaMask;

    SCL_LOW();

    // Leaves SCL low, SDA high
	return bit;
}
