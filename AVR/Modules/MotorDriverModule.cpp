/*
 * MotorModule.cpp
 *
 * Created: 8/22/2014 9:55:37 AM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_MOTOR)

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>
#include <avr/pgmspace.h>

#include "Modulo.h"
#include "PWM.h"
#include "Clock.h"
#include "ModuloInfo.h"
#include "TwoWire.h"
#include "MotorCurrentLimit.h"
#include "StepperDriver.h"

DECLARE_MODULO("co.modulo.motor", 1);

enum FunctionCode {
	FunctionSetValue,
    FunctionSetMode,
    FunctionSetFrequency,
	FunctionSetCurrentLimit
};

/*
 * INA1 PA7 TOCC6
 * INA2 PB2 TOCC7 
 * INB1 PA1 TOCC0
 * INB2 PA2 TOCC1

 * ENABLEA PB1 
 * ENABLEB PA3 TOCC2
 * SENSEA PA0 ADC0
 * SENSEB PB0 ADC11
 */

#define DEFAULT_FREQUENCY 30000

#define ENABLE_PORT PORTB
#define ENABLE_DDR DDRB
#define ENABLE_MASK _BV(1)

#define FAULT_PORT PORTB
#define FAULT_DDR DDRB
#define FAULT_PUE PUEB
#define FAULT_PIN 0

#define MODE_DISABLED 0
#define MODE_DC 1
#define MODE_STEPPER 2

volatile uint8_t *PORTS[] = {&PORTA, &PORTB, &PORTA, &PORTA};
volatile uint8_t *DDRS[]  = {&DDRA, &DDRB, &DDRA, &DDRA};
uint8_t MASKS[] = {_BV(7), _BV(2), _BV(1), _BV(2)};

PWM pwm[] = {PWM(2,6),  // Timer 2, output compare 6
			 PWM(2,7),  // Timer 2, output compare 7
			 PWM(1,0),  // Timer 1, output compare 0
			 PWM(1,1)}; // Timer 1, output compare 1

StepperDriver stepper;
uint8_t _mode = MODE_DISABLED;


void _setValue(uint8_t channel, uint16_t value) {
    if (channel >= 4) {
        return;
    }
    pwm[channel].SetValue(value);
}

void _setMode(uint8_t mode) {
	ENABLE_DDR |= ENABLE_MASK;
	
	_mode = mode;	
	if (mode == MODE_DISABLED) {
		ENABLE_PORT &= ~ENABLE_MASK;
	} else {
		ENABLE_PORT |= ENABLE_MASK;
	}
}


void _setFrequency( uint16_t frequency) {
	for (int i=0; i < 4; i++) {
	    pwm[i].SetFrequency(frequency);
	}
}






bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch ((FunctionCode)buffer.GetCommand()) {
    	case FunctionSetValue:
            if (buffer.GetSize() != 3) {
                return false;
            }
            _setValue(buffer.Get<uint8_t>(0), buffer.Get<uint16_t>(1));
            return true;
        case FunctionSetMode:
            if (buffer.GetSize() != 1) {
                return false;
            }
            _setMode(buffer.Get<uint8_t>(0));
            return true;
        case FunctionSetFrequency:
            if (buffer.GetSize() != 2) {
                return false;
            }
            _setFrequency(buffer.Get<uint16_t>(0));
            return true;
		case FunctionSetCurrentLimit:
			if (buffer.GetSize() != 1) {
				return false;
			}
			MotorCurrentLimitSet(buffer.Get<uint8_t>(0));
			return true;
	}
	return false;
}

bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
    switch (command) {
    }
    return false;
}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
}

void ModuloReset() {	
	for (int i=0; i < 4; i++) {
		*(DDRS[i]) |= MASKS[i];
		pwm[i].SetFrequency(DEFAULT_FREQUENCY);
	}
	
	FAULT_PUE |= _BV(FAULT_PIN);
	
	_setMode(MODE_DISABLED);
	_setFrequency(DEFAULT_FREQUENCY);
	MotorCurrentLimitInit();
		
    for (int i=0; i < 4; i++) {
        pwm[i].SetValue(0);
        pwm[i].SetCompareEnabled(true);
    }
}


int main(void)
{
	ModuloInit(&DDRA, &PORTA, _BV(5));
	ClockInit();

	_setMode(MODE_STEPPER);
	stepper.offsetPosition(16);
	stepper.setSpeed(10, 1);
	stepper.setTargetPosition(10*256L*200L + 32);
	
	//MotorCurrentLimitSet(32);
	
	uint32_t prevTicks = ClockGetTicks();
	while(1) {
		ModuloUpdateStatusLED();

		uint32_t newTicks = ClockGetTicks();
		uint32_t ticksDelta = (newTicks-prevTicks);
		prevTicks = newTicks;
		
		if (_mode == MODE_STEPPER) {
			stepper.update(ticksDelta, pwm);
		}
	}
}


#endif