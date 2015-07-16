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


#define MODULE_TYPE_DPAD 1
#define MODULE_TYPE_KNOB 2
#define MODULE_TYPE_GPIO 3
#define MODULE_TYPE_RTC 4
#define MODULE_TYPE_OLED 5
#define MODULE_TYPE_THERMOCOUPLE 6
#define MODULE_TYPE_MOTOR 7
#define MODULE_TYPE_RADIO 8
#define MODULE_TYPE_JOYSTICK 9
#define MODULE_TYPE_IR 10
#define MODULE_TYPE_IMU 11

#define MODULE_TYPE MODULE_TYPE_IR

#ifndef MODULE_TYPE
#error No module type defined
#endif

#if MODULE_TYPE == 0
#error Invalid module type
#endif

#endif /* CONFIG_H_ */
