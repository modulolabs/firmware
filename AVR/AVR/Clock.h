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

// the prescaler is set so that timer0 ticks every 64 clock cycles, and the
// the overflow handler is called every 256 ticks.
//
// At 8Mhz, MICROSECONDS_PER_TICK is 8.
#define MICROSECONDS_PER_TICK (clockCyclesToMicroseconds(64))

#define interrupts() sei()
#define noInterrupts() cli()

uint32_t micros();
uint32_t millis();
void delay(uint16_t duration);
void delayMicroseconds(uint16_t duration);

void ClockInit();
uint32_t ClockGetTicks();

#endif /* CLOCK_H_ */
