/*
 * Timer.h
 *
 * Created: 6/18/2014 9:49:20 AM
 *  Author: ekt
 */ 


#ifndef TIMER_H_
#define TIMER_H_

#include <stdint.h>
#include <avr/io.h>

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
	explicit PWM(uint8_t timerNum, uint8_t compareChannel);
	
	// Set the PWM frequency. Defaults to 31250 Hz which gives
	// an 8 bit resolution with an 8Mhz system clock.
	void SetFrequency(uint32_t frequency);

	// Set the value to use for this PWM.
	void SetValue(uint16_t value);

	// Set the value to use for this PWM without scaling to the top value
	void SetRawValue(uint16_t value) {
		bool odd = _channel % 2;

		switch(_timerNum) {
			case 1:
				if (odd) {
    				OCR1A = value;
				} else {
					OCR1B = value;
				}
				break;
			case 2:
				if (odd) {
    				OCR2A = value;
				} else {
             		OCR2B = value;   
				}
				break;
		}
	}

	// Get the top value based on the current frequency setting
	uint16_t GetTopValue() {
		return _topValue;
	}

    // Sets or clears the bit for this channel in the channel
	// output enable register.
	void SetCompareEnabled(bool enabled);
	
private:
	// Set the clock select bits for this timer.
	void _SetClockSelect(ClockSelect cs);
	
	// Set the waveform generation mode and channel comparison modes
	void _SetMode(uint8_t waveformGenerationMode,
		uint8_t compareModeA,
		uint8_t compareModeB);
	
	// Set the top value, which is the maximum value the timer will reach
	// before being reset to 0.
	void _SetTop(uint16_t ticks);
	
private:
	uint8_t _timerNum;
    uint8_t _channel;
	uint16_t _topValue;
};

#endif /* TIMER_H_ */