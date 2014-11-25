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

#define MODULE_TYPE_LED 1
#define MODULE_TYPE_DPAD 2
#define MODULE_TYPE_KNOB 3
#define MODULE_TYPE_SERVO 4
#define MODULE_TYPE_GPIO 5
#define MODULE_TYPE_RTC 6
#define MODULE_TYPE_OLED 7
#define MODULE_TYPE_DC 8
#define MODULE_TYPE_THERMOCOUPLE 9
#define MODULE_TYPE_STEPPER 10
#define MODULE_TYPE_MOTOR 11
#define MODULE_TYPE_AC 12

#define MODULE_TYPE MODULE_TYPE_DPAD

// For now, define the address to be the same as the type
#define MODULE_ADDRESS MODULE_TYPE


#ifndef MODULE_TYPE
#error No module type defined
#endif

#ifndef MODULE_ADDRESS
#error No module address defined
#endif


#endif /* CONFIG_H_ */
