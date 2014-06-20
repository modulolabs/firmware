/*
 * Setup.cpp
 *
 * Created: 4/29/2014 4:55:03 PM
 *  Author: ekt
 */ 

#include <inttypes.h>
#include <avr/io.h>
#include <avr/interrupt.h>
#include "Setup.h"

void Init()
{
	DDRA |= 1;
	PORTA = PORTA ^ 1;
	
	TIMSK0 |= (1 << TOIE0); // Enable the Timer0 overflow interrupt
	
	//TCCR0A |= (1 << WGM01) | ( 1 << WGM00);

	TCCR0B |= (1 << CS01) | (1 << CS00); // Enable clock with 64x prescaler

	//CLKPR = CLKPS0 | CLKPS1;
	
	sei();
}
