/*
 * Timer.cpp
 *
 * Created: 6/18/2014 9:49:36 AM
 *  Author: ekt
 */ 
#include "PWM.h"
#include "Clock.h"
#include "Config.h"
#include <stdio.h>
#include <math.h>
#include <avr/sfr_defs.h>
#include <avr/io.h>

#if defined(CPU_TINYX41)

PWM::PWM(uint8_t timerNum, uint8_t channel) :
	_timerNum(timerNum), _channel(channel), _topValue(0)
{
	_SetMode(8, 2, 2);
	//SetFrequency(31250);
    SetFrequency(50);
    
    // Configure the specified output pin to use this timer.
    // Odd outputs will use compare register A
    // Even outputs will use compare register B
    switch(_channel) {
        case 0:
        	TOCPMSA0 &= ~(0b11 << TOCC0S0);
        	TOCPMSA0 |= (_timerNum << TOCC0S0);
        	break;
        case 1:
        	TOCPMSA0 &= ~(0b11 << TOCC1S0);
        	TOCPMSA0 |= (_timerNum << TOCC1S0);
        	break;
        case 2:
        	TOCPMSA0 &= ~(0b11 << TOCC2S0);
        	TOCPMSA0 |= (_timerNum << TOCC2S0);
        	break;
        case 3:
        	TOCPMSA0 &= ~(0b11 << TOCC3S0);
        	TOCPMSA0 |= (_timerNum << TOCC3S0);
        	break;
        case 4:
        	TOCPMSA1 &= ~(0b11 << TOCC4S0);
        	TOCPMSA1 |= (_timerNum << TOCC4S0);
        	break;
        case 5:
        	TOCPMSA1 &= ~(0b11 << TOCC5S0);
        	TOCPMSA1 |= (_timerNum << TOCC5S0);
        	break;
        case 6:
        	TOCPMSA1 &= ~(0b11 << TOCC6S0);
        	TOCPMSA1 |= (_timerNum << TOCC6S0);
        	break;
        case 7:
        	TOCPMSA1 &= ~(0b11 << TOCC7S0);
        	TOCPMSA1 |= (_timerNum << TOCC7S0);
        	break;
    }
}

void PWM::_SetMode(uint8_t wgm, uint8_t modeA, uint8_t modeB) {
	switch(_timerNum) {
		case 1:
			TCCR1A =  (modeA << COM1A0) | (modeB << COM1B0) | (wgm & 0b11);
			TCCR1B = (TCCR1B & 0b11100111) | ((wgm >> 2) << WGM12);
			break;
		case 2:
			TCCR2A =  (modeA << COM2A0) | (modeB << COM2B0) | (wgm & 0x3);
			TCCR2B = (TCCR2B & 0b11100111) | ((wgm >> 2) << WGM22);
			break;		
	}
}

void PWM::_SetClockSelect(ClockSelect cs) {
	switch(_timerNum) {
		case 1:
			TCCR1B = (TCCR1B & 0b11111000) | cs;
			break;
		case 2:
			TCCR2B = (TCCR2B & 0b11111000) | cs;
			break;
	}
}

void PWM::SetCompareEnabled(bool enabled) {
	// Set the appropriate bit in the channel output enable register
	if (enabled) {
        TOCPMCOE |= _BV(_channel);
    } else {
        TOCPMCOE &= ~_BV(_channel);
    }
}

void PWM::_SetTop(uint16_t value) {
	switch (_timerNum) {
		case 1:
			ICR1 = value;
			break;
		case 2:
			ICR2 = value;
			break;
	}
}

void PWM::SetValue(uint16_t inValue) {
    // The top value may be less than 65535, depending on frequency
    // Scale the value appropriately.
    uint16_t value = uint32_t(inValue)*_topValue/65535;

    bool odd = _channel % 2;

	switch(_timerNum) {
		case 1:
            if (odd) {
    			OCR1A = value;
            } else {
                OCR1B = value;
            }
			break;
		case 2:
            if (odd) {
    			OCR2A = value;
            } else {
             	OCR2B = value;   
            }
			break;
	}
}


void PWM::SetFrequency(uint32_t freq)
{
	// Using phase correct pwm, must multiplu freq by 2
	freq *= 2;
	
	uint32_t counterMax = (1L << 16);
	
	// Number of CPU cycles per PWM cycle
	uint32_t cycles = F_CPU/freq;

	ClockSelect clockSelect = ClockSelectNone;
	int16_t divisor = 1;
	
    // Choose the highest clock setting that supports the requested frequency.                                                                                                                      
    if (cycles < 2) {
        // Exceeds maximum frequency
		return;                                                                                                           
	} else if (cycles <= counterMax) {
        divisor = 1;
		clockSelect = ClockSelect1;
	} else if (cycles/8 <= counterMax) {
        divisor = 8;
		clockSelect = ClockSelect8;
    } else if (cycles/64 <= counterMax) {
        divisor = 64;
		clockSelect = ClockSelect64;
    } else if (cycles/256 <= counterMax) {
        divisor = 256;
		clockSelect = ClockSelect256;
    } else if (cycles/1024 <= counterMax) {
        divisor = 1024;
		clockSelect = ClockSelect1024;
	} else {
		// Below minimum frequency
		return;
	}

    // Determine the TOP value                                                                                                   
    _topValue = cycles/divisor-1;

	_SetClockSelect(clockSelect);
	_SetTop(_topValue);
}

#endif
