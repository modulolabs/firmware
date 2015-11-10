/*
 * MotorCurrentLimit.h
 *
 * Controls the digital potentiometer on the MotorDriver modulo
 *
 * Created: 11/10/2015 10:10:01 AM
 *  Author: ekt
 */ 

#ifndef MOTORCURRENTLIMIT_H_
#define MOTORCURRENTLIMIT_H_

// Initialize the potentiometer to its max value (63).
void MotorCurrentLimitInit();

// Set the potentiometer to a specific value from 0 to 63.
void MotorCurrentLimitSet(int newCurrentLimit);


#endif /* MOTORCURRENTLIMIT_H_ */