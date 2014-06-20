#include "Setup.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_GPIO)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include "../TwoWire.h"
#include "../PWM.h"


void StartADC(int pin) {
	// Set the channel
	// XXX: Need to perform pin to channel mapping
	ADMUXA = pin;

	// Start conversion
	ADCSRA |= _BV(ADEN) | _BV(ADSC);
}

uint16_t GetADCResult() {
	// Wait for conversion to complete
	while (ADCSRA & _BV(ADSC)) {
	}
	
	// Must read ADCL before ADCH
	uint8_t low = ADCL;
	uint16_t high = ADCH;
	
	return (high << 8) | low;
}

void SetDirectionBits(uint8_t dir) {
	
}

void SetDigitalOutputs(uint8_t values) {
	
}

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {
}

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {

}

uint8_t previousState = 0;

uint8_t forward[] = {1, 3, 0, 2};
uint8_t backward[] = {2, 0, 3, 1};

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	
	// Enable the LED
	DDRA |= _BV(5);
	PORTA |= _BV(5);
	
	// Enable drive pins
	//DDRB |= _BV(0) | _BV(1);
	//PORTB |= _BV(1);
	
#if 0
	// Configure the ADC
	ADMUXA = 8; // Read from ADC08

	// Configure a timer for the LED
#endif

	DDRA |= _BV(3) | _BV(2) | _BV(7);
	DDRB |= _BV(2);

	PWM pwm1(1);
	pwm1.EnableOutputCompare(2);
	pwm1.EnableOutputCompare(1);
	pwm1.SetOddDutyCycle(.9);
	pwm1.SetEvenDutyCycle(.1);
	
	PWM pwm2(2);
	pwm2.SetFrequency(50);
	pwm2.EnableOutputCompare(6);
	pwm2.EnableOutputCompare(7);

	pwm2.SetEvenDutyCycle(.1);
	
	while(1)
	{
		StartADC(11);
		uint16_t result = GetADCResult();
		_delay_ms(1);

		float center = .086;
		float range = .1;
		float dutyCycle = (result/1024.);
		dutyCycle = (dutyCycle-.5)*range + center;
		//dutyCycle = maxDuty*dutyCycle * minDuty*(1-dutyCycle);
		
		pwm2.SetEvenDutyCycle(dutyCycle);
	}
}

#endif