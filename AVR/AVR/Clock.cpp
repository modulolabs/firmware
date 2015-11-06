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

static volatile unsigned long timer0_overflow_count = 0;

void ClockInit()
{
	// Enable the Timer0 overflow interrupt
    TIMSK0 |= _BV(TOIE0);
	
	// Enable clock with 64x prescaler
    TCCR0B |= _BV(CS01) | _BV(CS00); 
    
	// Ensure that interrupts are enabled
	sei();
}

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
//
// At 8Mhz, MICROSECONDS_PER_TICK is 8.
#define MICROSECONDS_PER_TICK (clockCyclesToMicroseconds(64))

unsigned long micros() {
	// If TCNT0 overflows after disabling interrupts but before reading its value,
	// then the time will be up to 256 ticks off. Not sure how to fix that.
    cli();
	unsigned long ticks = TCNT0;
	unsigned long overflows = timer0_overflow_count;
	sei();
	
	return (overflows*256 + ticks)*MICROSECONDS_PER_TICK;
}

unsigned long millis() {
    return micros()/1000;
}

ISR(TIMER0_OVF_vect)
{
    timer0_overflow_count++;
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
