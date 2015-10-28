/*
 * Clock.h
 *
 * Created: 7/16/2014 12:45:46 PM
 *  Author: ekt
 */ 


#ifndef CLOCK_H_
#define CLOCK_H_

#include "Config.h"

#include <inttypes.h>
#include <avr/interrupt.h>

#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )

#define interrupts() sei()
#define noInterrupts() cli()

unsigned long micros();
unsigned long millis();
void delay(uint16_t duration);
void delayMicroseconds(uint16_t duration);

void ClockInit();

#endif /* CLOCK_H_ */