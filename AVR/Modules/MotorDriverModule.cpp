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

#define UD_PORT PORTA
#define UD_PIN 0
#define UD_DDR DDRA
#define CS_PORT PORTA
#define CS_PIN 3
#define CS_DDR DDRA


volatile uint8_t *PORTS[] = {&PORTA, &PORTB, &PORTA, &PORTA};
volatile uint8_t *DDRS[]  = {&DDRA, &DDRB, &DDRA, &DDRA};
uint8_t MASKS[] = {_BV(7), _BV(2), _BV(1), _BV(2)};

PWM pwm[] = {PWM(2,6),  // Timer 2, output compare 6
			 PWM(2,7),  // Timer 2, output compare 7
			 PWM(1,0),  // Timer 1, output compare 0
			 PWM(1,1)}; // Timer 1, output compare 1

// Map the microstep index to pwm compare value for the first quadrant on coil A.
// See additional details in _setFrequency.
#define STEP_TABLE_SIZE 256
uint8_t stepTable[STEP_TABLE_SIZE];


int _currentLimit = 0;
void _initCurrentLimit();
void _setCurrentLimit(int newCurrentLimit);
void _currentIncr(int count);
void _currentDecr(int count);

#define MODE_DISABLED 0
#define MODE_DC 1
#define MODE_STEPPER 2

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

void _initStepTable() {
	// Initialize the microstep table.
	//
	// The step table maps a microstep index in the first quadrant to a PWM compare value
	// for coil A. Other coils/quadrants use the same table, but reversed and/or inverted.
	// 
	//    Full Step  - All 1.0
	//    Half Steps - 1.0 for first 3/4, 0.5 for remaining 1/4
	//    Microstep (square path)   - 1.0 for first half, linearly decreasing for second half
	//    Microstep (circular path) - cos((pi/2)*(i/length))
	//
	// The values stored are inverted and relative to the pwm topValue. So
	//     1.0 -> 0
	//     0.0 -> topValue
	
	for (int i=0; i < STEP_TABLE_SIZE/2; i++) {
		stepTable[i] = 0;
	}
	for (int i=0; i < STEP_TABLE_SIZE/2; i++) {
		stepTable[i+STEP_TABLE_SIZE/2] = i*2;
	}
}

void _setFrequency( uint16_t frequency) {
	for (int i=0; i < 4; i++) {
	    pwm[i].SetFrequency(frequency);
	}
	
	// When changing the frequency, re-intialize the step table
	_initStepTable();
}


class StepperMotor {

public:

	StepperMotor() : _position(0),  _targetPosition(0), _ticksPerStep(200*125),
		_stepSize(1), _stepSizeMask(0xFFFFFFFF) {
	}
	
	// Get the current position of the stepper motor in microsteps
	int32_t getPosition() {
		return _position;
	}
	
	// Add an offset to the current position
	void offsetPosition(int32_t offset) {
		_position += offset;
	}
	
	// Given a position, return the step table value
	void getStepTableValue(int32_t pos, uint16_t *value, bool *negative) {
		pos = pos % (STEP_TABLE_SIZE*4);
		if (pos < 0) {
			pos += (STEP_TABLE_SIZE*4);
		}
		
		// Ensure that the position is a multiple of the step size
		pos &= _stepSizeMask;
		
		uint8_t quadrant = pos / STEP_TABLE_SIZE;
		uint8_t index = pos % STEP_TABLE_SIZE;

		// Reverse the function in quadrants 1 and 3
		if (quadrant == 1 or quadrant == 3) {
			index = (STEP_TABLE_SIZE-1)-index;
		}
		
		// Get the value
		*value = stepTable[index];
		
		// Invert the value in quadrants 1 and 2
		*negative = (quadrant == 1 or quadrant == 2);
	}
	
	// Step size must be a power of 2 and <= STEP_TABLE_SIZE
	void setSpeed(int32_t ticksPerStep, int16_t stepSize) {
		_ticksPerStep = ticksPerStep;
		_stepSize = stepSize;
		
		// Prepare a mask, that when bitwise anded with the
		// position will make the position a multiple of the
		// step size
		_stepSizeMask = ~(((uint32_t)stepSize)-1);
	}
	
	void setTargetPosition(int32_t position) {
		_targetPosition = position;
	}
	
// Enable and set a breakpoint on the nop below
// to measure the stepper motor update time.
// #define MEASURE_UPDATE_TIME	

	void update(uint32_t ticks) {
#if MEASURE_UPDATE_TIME		
		uint32_t startTime = ClockGetTicks();
#endif

		_time += ticks;
		if (_time < _ticksPerStep) {
			return;
		}
					
		while (_time >= _ticksPerStep) {
			_time -= _ticksPerStep;
				
			if (_position < _targetPosition) {
				_position = _position+_stepSize;
				if (_position > _targetPosition) {
					_position = _targetPosition;
				}
			} else if (_position > _targetPosition) {
				_position = _position-_stepSize;
				if (_position < _targetPosition) {
					_position = _targetPosition;
				}
			}
		}
		
		setCoils();
		
#if MEASURE_UPDATE_TIME
		uint32_t endTime = ClockGetTicks();
		volatile uint32_t delta = endTime-startTime;
		if (delta > 100) {
			asm("nop");
		}
#endif
	}
	
	void setCoils() {
		uint32_t top = pwm[0].GetTopValue();
		
		// Offset by half the step table size because that puts full steps
		// at the strongest drive strength
		uint32_t pos = _position + STEP_TABLE_SIZE/2;
		
		uint16_t coilA;
		bool coilANegative;
		getStepTableValue(pos, &coilA, &coilANegative);
		if (!coilANegative) {
			pwm[0].SetRawValue(65535);
			pwm[1].SetRawValue((coilA*top)/256);
		} else {
			pwm[0].SetRawValue((coilA*top)/256);
			pwm[1].SetRawValue(65535);
		}

		uint16_t coilB;
		bool coilBNegative;
		getStepTableValue(pos-STEP_TABLE_SIZE, &coilB, &coilBNegative);
		if (!coilBNegative) {
			pwm[2].SetRawValue(65535);
			pwm[3].SetRawValue((coilB*top)/256);
		} else {	
			pwm[2].SetRawValue((coilB*top)/256);
			pwm[3].SetRawValue(65535);
		}	
	}
	
private:
	uint32_t _time;
	int32_t _position;
	int32_t _targetPosition;
	uint16_t _ticksPerStep;
	int16_t _stepSize;
	uint32_t _stepSizeMask;	
};


void _initCurrentLimit() {
	CS_DDR |= _BV(CS_PIN);
	UD_DDR |= _BV(UD_PIN);
	
	_currentLimit = 0;
	_currentIncr(64);
}

void _currentIncr(int count) {
	UD_PORT |= _BV(UD_PIN);
	_delay_us(1);
		
	CS_PORT &= ~_BV(CS_PIN);
	_delay_us(1);
		
	for (int i=0; i < count; i++) {
		UD_PORT &= ~_BV(UD_PIN);
		_delay_us(1);
			
		UD_PORT |= _BV(UD_PIN);
		_delay_us(1);
	}
	
	_currentLimit += count;
	if (_currentLimit > 64) {
		_currentLimit = 64;
	}
	
	CS_PORT |= _BV(CS_PIN);
	_delay_us(1);	
}

void _currentDecr(int count) {
	UD_PORT &= ~_BV(UD_PIN);
	_delay_us(1);
	CS_PORT &= ~_BV(CS_PIN);
	_delay_us(1);
		
	for (int i=0; i < count; i++) {
		UD_PORT |= _BV(UD_PIN);
		_delay_us(1);
		UD_PORT &= ~_BV(UD_PIN);
		_delay_us(1);	
	}
	
	_currentLimit -= count;
	if (_currentLimit < 0) {
		_currentLimit = 0;
	}
	
	CS_PORT |= _BV(CS_PIN);
	_delay_us(1);	
}

void _setCurrentLimit(int newCurrentLimit) {
	int delta = newCurrentLimit-_currentLimit;
	if (delta > 0) {
		_currentIncr(delta);
	} else if (delta < 0) {
		_currentDecr(-delta);
	}
}

StepperMotor stepper;



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
			_setCurrentLimit(buffer.Get<uint8_t>(0));
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
	_initCurrentLimit();
	
	for (int i=0; i < 4; i++) {
		*(DDRS[i]) |= MASKS[i];
		pwm[i].SetFrequency(DEFAULT_FREQUENCY);
	}
	
	FAULT_PUE |= _BV(FAULT_PIN);
	
	_currentDecr(64);
	_currentLimit = 0;
	
	_setCurrentLimit(64);
	
	_setMode(MODE_DISABLED);
	_initCurrentLimit();
	_setFrequency(DEFAULT_FREQUENCY);
	
    for (int i=0; i < 4; i++) {
        pwm[i].SetValue(0);
        pwm[i].SetCompareEnabled(true);
    }
}


int main(void)
{
	ModuloInit(&DDRA, &PORTA, _BV(5));
	ModuloReset();
	ClockInit();
	
	_setMode(MODE_STEPPER);
	stepper.offsetPosition(16);
	stepper.setSpeed(20, 1);
	stepper.setTargetPosition(10*256L*200L + 32);
	
	uint32_t prevTicks = ClockGetTicks();
	while(1) {
		ModuloUpdateStatusLED();

		uint32_t newTicks = ClockGetTicks();
		uint32_t ticksDelta = (newTicks-prevTicks);
		prevTicks = newTicks;
		
		if (_mode == MODE_STEPPER) {
			stepper.update(ticksDelta);
		}
	}
}


#endif