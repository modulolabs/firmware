/*
 * Wiring.cpp
 *
 * Created: 5/2/2014 6:17:29 PM
 *  Author: ekt
 */ 

#include "NeoPixel.h"
#include "Setup.h"
#include "Wiring.h"
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

const uint16_t PROGMEM port_to_mode_PGM[] = {
	NOT_A_PORT,
	(uint16_t) &DDRA,
	(uint16_t) &DDRB,
};

const uint16_t PROGMEM port_to_output_PGM[] = {
	NOT_A_PORT,
	(uint16_t) &PORTA,
	(uint16_t) &PORTB
};

const uint16_t PROGMEM port_to_input_PGM[] = {
	NOT_A_PORT,
	(uint16_t) &PINA,
	(uint16_t) &PINB
};

const uint8_t PROGMEM digital_pin_to_port_PGM[] = {
	PA, /* 0 */
	PA,
	PA,
	PA,
	PB,
	PB,
	PB,
	PB,
};

const uint8_t PROGMEM digital_pin_to_bit_mask_PGM[] = {
	_BV(3),
	_BV(2),
	_BV(1),
	_BV(0),
	_BV(7),
	_BV(2),
	_BV(1),
	_BV(0)
};


void pinMode(uint8_t pin, uint8_t mode)
{
	uint8_t bit = digitalPinToBitMask(pin);
	uint8_t port = digitalPinToPort(pin);
	volatile uint8_t *reg, *out;

	if (port == NOT_A_PIN) return;

	// JWS: can I let the optimizer do this?
	reg = portModeRegister(port);
	out = portOutputRegister(port);

	if (mode == INPUT) {
		uint8_t oldSREG = SREG;
		cli();
		*reg &= ~bit;
		*out &= ~bit;
		SREG = oldSREG;
		} else if (mode == INPUT_PULLUP) {
		uint8_t oldSREG = SREG;
		cli();
		*reg &= ~bit;
		*out |= bit;
		SREG = oldSREG;
		} else {
		uint8_t oldSREG = SREG;
		cli();
		*reg |= bit;
		SREG = oldSREG;
	}
}

void delay(uint16_t duration)
{
	while (duration--) {
		_delay_ms(1);
	}
}

void delay_us(uint16_t duration)
{
	while (duration--) {
		_delay_us(1);
	}
}