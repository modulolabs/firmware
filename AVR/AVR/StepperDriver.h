/* 
* StepperDriver.h
*
* Created: 11/10/2015 10:21:00 AM
* Author: ekt
*/


#ifndef __STEPPERDRIVER_H__
#define __STEPPERDRIVER_H__

#include "PWM.h"

#define STEP_TABLE_SIZE 256

class StepperDriver {

public:

	StepperDriver();

	void init();
	
	// Get the current position of the stepper motor in microsteps
	int32_t getPosition();
	
	// Add an offset to the current position
	void addOffset(int32_t offset);
	
	// Step size must be a power of 2 and <= STEP_TABLE_SIZE
	void setSpeed(int32_t ticksPerStep, uint8_t resolution);
	
	// Set the target position
	void setTargetPosition(int32_t position);
	
	// Advance the stepper motor position by the specified number of 8us ticks
	// If the target position was reached in this update, return true.
	bool update(uint32_t ticks, PWM pwms[4]);
	
private:
	void initStepTable();

	void setCoils(PWM pwms[4]);
	
	// Given a position, return the step table value
	void getStepTableValue(int32_t pos, uint16_t *value, bool *negative);
	
	uint32_t _time;
	int32_t _position;
	int32_t _targetPosition;
	uint16_t _ticksPerStep;
	int16_t _stepSize;
	uint32_t _stepSizeMask;
	
	// Map the microstep index to pwm compare value for the first quadrant on coil A.
	uint8_t stepTable[STEP_TABLE_SIZE];

};


#endif //__STEPPERDRIVER_H__
