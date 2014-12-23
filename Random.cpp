/*
 * Random.cpp
 *
 * Created: 9/2/2014 3:04:36 PM
 *  Author: ekt
 *
 */ 

#include <avr/interrupt.h>
#include <avr/wdt.h>
#include <util/atomic.h>

static volatile uint32_t _result;
static volatile int8_t _count;

uint32_t GenerateRandomNumber()
{
    // Initialize the result to 0, and set the number of
    // interrupts to wait for.
    _result = 0;
    _count = 8;
    
    // Enable Timer1
    TCCR1B |= _BV(CS10);
	
    // Enable WDT interrupts
    cli();
    MCUSR = 0;
    WDTCSR |= _BV(WDIE);
    sei();
    
    while (_count > 0)
        ;
    
    // Disable WDT interrupts
    cli();
    MCUSR = 0;
    WDTCSR &= ~_BV(WDIE);
    sei();
    
    // Disable Timer1
    TCCR1B &= ~_BV(CS10);
    
    return _result;
}

ISR(WDT_vect)
{
    _count--;
    // Rotate 8 bits to the left.
    _result = (_result << 8) | (_result >> 24);
    _result = _result ^ TCNT1L;
}
