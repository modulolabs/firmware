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

PWM::PWM(uint8_t timerNum) :
	_timerNum(timerNum),_topValue(0), _evenDutyCycle(0), _oddDutyCycle(0)
{
	_SetMode(14, 2, 2);
	SetFrequency(31250.0);
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

void PWM::EnableOutputCompare(uint8_t output) {
	// Configure the specified output pin to use this timer.
	// Odd outputs will use compare register A
	// Even outputs will use compare register B
	switch(output) {
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
	
	// Set the appropriate bit in the channel output enable register
	TOCPMCOE |= _BV(output);
}

void PWM::DisableOutputCompare(uint8_t output) {
	TOCPMCOE &= ~_BV(_timerNum);
}

void PWM::SetOutputCompareEnable(uint8_t channel, bool enabled)
{
	if (enabled) {
		EnableOutputCompare(channel);
	} else {
		DisableOutputCompare(channel);
	}
}

void PWM::_SetTop(uint16_t value) {
	switch (_timerNum) {
		case 1:
			ICR1H = value >> 8;
			ICR1L = value & 0xFF;
			break;
		case 2:
			ICR2H = value >> 8;
			ICR2L = value & 0xFF;
			break;
	}
}

void PWM::_SetOddCompareValue(uint16_t value) {
	// Note: when writing to 16 bit registers, the high byte
	// must be written before the low byte.
	switch(_timerNum) {
		case 1:
			OCR1AH = value >> 8;
			OCR1AL = value & 0xFF;
			break;
		case 2:
			OCR2AH = value >> 8;
			OCR2AL = value & 0xFF;
			break;
	}
}

void PWM::_SetEvenCompareValue(uint16_t value) {
	// Note: when writing to 16 bit registers, the high byte
	// must be written before the low byte.
	switch(_timerNum) {
		case 1:
			OCR1BH = value >> 8;
			OCR1BL = value & 0xFF;
			break;
		case 2:
			OCR2BH = value >> 8;
			OCR2BL = value & 0xFF;
			break;
	}
}

void PWM::SetFrequency(float freq)
{
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

	_SetEvenCompareValue(_evenDutyCycle*_topValue);
	SetOddDutyCycle(_oddDutyCycle*_topValue);
}

void PWM::SetEvenDutyCycle(float dutyCycle) {
	_evenDutyCycle = fmax(0, fmin(1, dutyCycle));
	
	_SetEvenCompareValue(_evenDutyCycle * _topValue);	
}

void PWM::SetOddDutyCycle(float dutyCycle) {
	_oddDutyCycle = fmax(0, fmin(1, dutyCycle));
	
	_SetOddCompareValue(_oddDutyCycle * _topValue);	
}

#endif
