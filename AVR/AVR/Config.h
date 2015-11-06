/*
 * Config.h
 *
 * Created: 4/29/2014 1:58:20 PM
 *  Author: ekt
 */ 


#ifndef CONFIG_H_
#define CONFIG_H_

#define F_CPU 8000000UL

#if defined(__AVR_ATtiny841__) || defined(__AVR_ATtiny841__)
#define CPU_TINYX41
#elif defined(__AVR_ATtiny48__) || defined(__AVR_ATtiny88__)
#define CPU_TINYX8
#endif

#define MODULE_TYPE_UNSET 0
#define MODULE_TYPE_KNOB 1
#define MODULE_TYPE_BLANK_SLATE 2
#define MODULE_TYPE_MOTOR 3
#define MODULE_TYPE_JOYSTICK 4
#define MODULE_TYPE_IR 5
#define MODULE_TYPE_TEMP_PROBE 6
#define MODULE_TYPE_THERMOCOUPLE 7

// Change MODULE_TYPE to the module you are compiling for
#define MODULE_TYPE MODULE_TYPE_UNSET

#ifndef MODULE_TYPE
#error No module type defined
#endif

#if MODULE_TYPE == 0
#error You must set MODULE_TYPE in Config.h before compiling.
#endif

#endif /* CONFIG_H_ */	
