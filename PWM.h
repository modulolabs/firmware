/*
 * Timer.h
 *
 * Created: 6/18/2014 9:49:20 AM
 *  Author: ekt
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>

// Manages a hardware timer for PWM waveform generation
class PWM {
public:

	// The clock select determines whether the timer clock is
	// enabled and how much to divide the system clock by to
	// get the timer clock.
	enum ClockSelect {
		ClockSelectNone = 0,      // Timer disabled
		ClockSelect1    = 1,      // Use the system clock
		ClockSelect8    = 2,      // System clock/8
		ClockSelect64   = 3,      // System clock/64
		ClockSelect256  = 4,      // System clock/256
		ClockSelect1024 = 5,      // System clock/1024
	};
	
	// Construct a PWM object for the specified timer.
	// Currently the 16 bit timers, 1 and 2, are supported.
	explicit PWM(uint8_t timerNum);
	
	// Set the PWM frequency. Defaults to 31250 Hz which gives
	// an 8 bit resolution with an 8Mhz system clock.
	void SetFrequency(float freq);
	
	// Set the duty cycle for odd numbered channels that are mapped
	// to this timer. The value will be clamped between 0 and 1.
	void SetOddDutyCycle(float dutyCycle);
	
	// Set the duty cycle for even numbered channels that are mapped
	// to this timer. The value will be clamped between 0 and 1.
	void SetEvenDutyCycle(float dutyCycle);

	// Configure the specified timer output compare channel (TOCCN) to
	// use this timer. Also sets the bit for this channel in the channel
	// output enable register.
	void EnableOutputCompare(uint8_t channel);
	
	// Disabled the output compare for the specified channel.
	void DisableOutputCompare(uint8_t channel);
	
	void SetOutputCompareEnable(uint8_t channel, bool enabled);
	
private:
	// Set the clock select bits for this timer.
	void _SetClockSelect(ClockSelect cs);
	
	// Set the output compare value for any even numbered output
	// compare channels that are using this timer.
	void _SetOddCompareValue(uint16_t value);
	
	// Set the output comapre value for any odd numbered output
	// compare channels that are using this timer.
	void _SetEvenCompareValue(uint16_t value);
	
	// Set the waveform generation mode and channel comparison modes
	void _SetMode(uint8_t waveformGenerationMode,
		uint8_t compareModeA,
		uint8_t compareModeB);
	
	// Set the top value, which is the maximum value the timer will reach
	// before being reset to 0.
	void _SetTop(uint16_t ticks);
	
private:
	uint8_t _timerNum;
	uint16_t _topValue;
	float _evenDutyCycle, _oddDutyCycle;
};

#endif /* TIMER_H_ */