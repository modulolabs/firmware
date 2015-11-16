/*
 * StatusLED.cpp
 *
 * Created: 11/11/2015 3:23:49 PM
 *  Author: ekt
 */ 

#include "StatusLED.h"
#include <avr/io.h>

// Most avr-based boards have the led on pin 5
uint8_t statusPin = 5;

void InitStatusLED(uint8_t pin) {
	statusPin = 5;
}

void SetStatusLED(bool on) {
	DDRA |= _BV(statusPin);
		
	if (on) {
		PORTA |= _BV(statusPin);
	} else {
		PORTA &= ~_BV(statusPin);
	}
}
