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
static volatile unsigned long timer0_millis = 0;
static unsigned char timer0_fract = 0;

void ClockInit()
{
    TIMSK0 |= _BV(TOIE0); // Enable the Timer0 overflow interrupt

#if defined(CPU_TINYX8)
    TCCR0A |= _BV(CS01) | _BV(CS00); // Enable clock with 64x prescaler
#elif  defined (CPU_TINYX41)
    TCCR0B |= _BV(CS01) | _BV(CS00); // Enable clock with 64x prescaler
#else
    #error "No implementation of ClockInit() for current CPU"
#endif

	
    sei();
}

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

unsigned long millis() {
    return micros()/1000;
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
	
    ModuloUpdateStatusLED();
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
