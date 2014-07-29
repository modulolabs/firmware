/*
 * Wiring.cpp
 *
 * Created: 5/2/2014 6:17:29 PM
 *  Author: ekt
 */ 

#include "NeoPixel.h"
#include "Wiring.h"
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <avr/io.h>
#include <avr/interrupt.h>


volatile unsigned long timer0_overflow_count = 0;
volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
#define MICROSECONDS_PER_TIMER0_OVERFLOW (clockCyclesToMicroseconds(64 * 256))

// the whole number of milliseconds per timer0 overflow
#define MILLIS_INC (MICROSECONDS_PER_TIMER0_OVERFLOW / 1000)

#define FRACT_INC ((MICROSECONDS_PER_TIMER0_OVERFLOW % 1000) >> 3)
#define FRACT_MAX (1000 >> 3)

unsigned long micros() {
	unsigned long m;
	uint8_t oldSREG = SREG, t;
	
	cli();
	m = timer0_overflow_count;
	#if defined(TCNT0)
	t = TCNT0;
	#elif defined(TCNT0L)
	t = TCNT0L;
	#else
	#error TIMER 0 not defined
	#endif

	
	#ifdef TIFR0
	if ((TIFR0 & _BV(TOV0)) && (t < 255))
	m++;
	#else
	if ((TIFR & _BV(TOV0)) && (t < 255))
	m++;
	#endif

	SREG = oldSREG;
	
	return ((m << 8) + t) * (64 / clockCyclesPerMicrosecond());
}

#if defined(__AVR_ATtiny24__) || defined(__AVR_ATtiny44__) || defined(__AVR_ATtiny84__)
ISR(TIM0_OVF_vect)
#else
ISR(TIMER0_OVF_vect)
#endif
{

	
	// copy these to local variables so they can be stored in registers
	// (volatile variables must be read from memory on every access)
	unsigned long m = timer0_millis;
	unsigned char f = timer0_fract;

	m += MILLIS_INC;
	f += FRACT_INC;
	if (f >= FRACT_MAX) {
		f -= FRACT_MAX;
		m += 1;
	}

	timer0_fract = f;
	timer0_millis = m;
	timer0_overflow_count++;
}

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
