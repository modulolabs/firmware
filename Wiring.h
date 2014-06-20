// A few macros to support Wiring based code


#ifndef WIRING_H_
#define WIRING_H_

#include <avr/interrupt.h>
#include "Setup.h"

#define INPUT 0
#define OUTPUT 1
#define interrupts() sei()
#define noInterrupts() cli()
#define clockCyclesPerMicrosecond() ( F_CPU / 1000000L )
#define clockCyclesToMicroseconds(a) ( (a) / clockCyclesPerMicrosecond() )

#ifndef cbi
#define cbi(sfr, bit) (_SFR_BYTE(sfr) &= ~_BV(bit))
#endif
#ifndef sbi
#define sbi(sfr, bit) (_SFR_BYTE(sfr) |= _BV(bit))
#endif

unsigned long micros();

#endif /* WIRING_H_ */