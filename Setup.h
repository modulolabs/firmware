/*
 * Config.h
 *
 * Created: 4/29/2014 1:58:20 PM
 *  Author: ekt
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#include <inttypes.h>

// For now, we have a fixed set of addresses and we turn
// on the right module code using ifdefs based on the address

#define MODULE_ADDRESS 8

#define MODULE_TYPE_LED 1
#define MODULE_TYPE_BUTTONS 2
#define MODULE_TYPE_KNOB 3
#define MODULE_TYPE_SERVO 4
#define MODULE_TYPE_GPIO 5

// Module 2 is an LED
#if MODULE_ADDRESS == 2
#define MODULE_TYPE MODULE_TYPE_LED
#endif

// Module 3 is an LED
#if MODULE_ADDRESS == 3
#define MODULE_TYPE MODULE_TYPE_LED
#endif

// Module 4 is a set of buttons
#if MODULE_ADDRESS == 4
#define MODULE_TYPE MODULE_TYPE_BUTTONS
#endif

#if MODULE_ADDRESS == 5
#define MODULE_TYPE MODULE_TYPE_KNOB
#endif

#if MODULE_ADDRESS == 6
#define MODULE_TYPE MODULE_TYPE_SERVO
#endif

#if MODULE_ADDRESS == 7
#define MODULE_TYPE MODULE_TYPE_POT
#endif

#if MODULE_ADDRESS == 8
#define MODULE_TYPE MODULE_TYPE_GPIO
#endif

#ifndef MODULE_TYPE
#error No module type set for module address
#endif

#define F_CPU 8000000UL

void Init();
uint32_t GetRandomValue();

#endif /* CONFIG_H_ */