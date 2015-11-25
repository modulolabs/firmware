/*
 * Timer.cpp
 *
 * Created: 7/16/2014 12:44:54 PM
 *  Author: ekt
 */

#include <avr/io.h>
#include <avr/interrupt.h>
#include "Modulo.h"
#include "Clock.h"

static volatile uint32_t timer0_overflow_count = 0;

void ClockInit()
{
	// Enable the Timer0 overflow interrupt
    TIMSK0 |= _BV(TOIE0);
	
	// Enable clock with 64x prescaler
    TCCR0B |= _BV(CS01) | _BV(CS00); 
    
	// Ensure that interrupts are enabled
	sei();
}

uint32_t ClockGetTicks() {
	uint8_t sreg = SREG;
	cli();
	uint32_t ticks = TCNT0;
	// if TNCT0 is small and the overflow flag is set, then the counter 
	// overflowed after we disabled interrupts and the ISR hasn't run yet.
	// Count that as an extra overflow.
	if ((TIFR0 & _BV(TOV0)) and ticks < 128) {
		ticks += 256;
	}
	ticks += 256*timer0_overflow_count;
	SREG = sreg;
	
	return ticks;
}

uint32_t micros() {
	return ClockGetTicks()*MICROSECONDS_PER_TICK;
}

uint32_t millis() {
    return micros()/1000;
}

void ClockCallback() __attribute__((weak));
void ClockCallback() {
	
}
void TwoWireWatchdog();

ISR(TIMER0_OVF_vect)
{
    timer0_overflow_count++;
	TwoWireWatchdog();
	ClockCallback();
}

/// Copied from Arduino
void delayMicroseconds(unsigned int us)
{
	// for a one- or two-microsecond delay, simply return.  the overhead of
	// the function calls takes more than two microseconds.  can't just
	// subtract two, since us is unsigned; we'd overflow.
	if (--us == 0)
	return;
	if (--us == 0)
	return;

	// the following loop takes half of a microsecond (4 cycles)
	// per iteration, so execute it twice for each microsecond of
	// delay requested.
	us <<= 1;
	
	// partially compensate for the time taken by the preceeding commands.
	// we can't subtract any more than this or we'd overflow w/ small delays.
	us--;


	// busy wait
	__asm__ __volatile__ (
	"1: sbiw %0,1" "\n\t" // 2 cycles
	"brne 1b" : "=w" (us) : "0" (us) // 2 cycles
	);
}

void delay(unsigned int ms)
{
	delayMicroseconds(ms*1000);
}
