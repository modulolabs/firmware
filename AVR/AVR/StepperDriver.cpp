/* 
* StepperDriver.cpp
*
* Created: 11/10/2015 10:21:00 AM
* Author: ekt
*/


#include "StepperDriver.h"

StepperDriver::StepperDriver()
{
	initStepTable();
	init();
}

void StepperDriver::init() {
	_position = 0;
	_targetPosition = 0;
	
	setSpeed(20, 1);
}
	
// Get the current position of the stepper motor in microsteps
int32_t StepperDriver::getPosition() {
	return _position;
}
	
// Add an offset to the current position
void StepperDriver::addOffset(int32_t offset) {
	_position += offset;
}
	
// Given a position, return the step table value
void StepperDriver::getStepTableValue(int32_t pos, uint16_t *value, bool *negative) {
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
void StepperDriver::setSpeed(int32_t ticksPerStep, uint8_t resolution) {
	_ticksPerStep = ticksPerStep;
	_stepSize = (256 >> resolution);
		
	// Prepare a mask, that when bitwise anded with the
	// position will make the position a multiple of the
	// step size
	_stepSizeMask = ~(((uint32_t)_stepSize)-1);
}
	
void StepperDriver::setTargetPosition(int32_t position) {
	_targetPosition = position;
}
	
// Enable and set a breakpoint on the nop below
// to measure the stepper motor update time.
// #define MEASURE_UPDATE_TIME	

bool StepperDriver::update(uint32_t ticks, PWM pwm[4]) {
#if MEASURE_UPDATE_TIME		
	uint32_t startTime = ClockGetTicks();
#endif

	_time += ticks;
	if (_time < _ticksPerStep) {
		return false;
	}
	
	bool targetReached = false;
	while (_time >= _ticksPerStep) {
		_time -= _ticksPerStep;
				
		if (_position < _targetPosition) {
			_position = _position+_stepSize;
			if (_position >= _targetPosition) {
				_position = _targetPosition;
				targetReached = true;
			}
		} else if (_position > _targetPosition) {
			_position = _position-_stepSize;
			if (_position <= _targetPosition) {
				_position = _targetPosition;
				targetReached = true;
			}
		}
	}
		
	setCoils(pwm);
		
#if MEASURE_UPDATE_TIME
	uint32_t endTime = ClockGetTicks();
	volatile uint32_t delta = endTime-startTime;
	if (delta > 100) {
		asm("nop");
	}
#endif

	return targetReached;
}
	
void StepperDriver::setCoils(PWM pwm[4]) {
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

void StepperDriver::initStepTable() {
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
