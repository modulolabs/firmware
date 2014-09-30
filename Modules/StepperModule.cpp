/*
 * StepperModule.cpp
 *
 * Created: 8/5/2014 11:33:26 AM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_STEPPER)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include "Modulo.h"
#include "PWM.h"
#include "Clock.h"

DEFINE_MODULO_CONSTANTS("Integer Labs", "Driver", 0, "http://www.integerlabs.net/docs/NavButtons");
DEFINE_MODULO_FUNCTION_NAMES("Speed,Steps");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeInt16, ModuloDataTypeInt16);

PWM pwm1(1);

volatile uint8_t *in1Port = &PORTA;
volatile uint8_t *in2Port = &PORTA;
volatile uint8_t *in3Port = &PORTA;
volatile uint8_t *in4Port = &PORTB;
volatile uint8_t *en1Port = &PORTA;
volatile uint8_t *en2Port = &PORTA;
volatile uint8_t *en3Port = &PORTB;
volatile uint8_t *en4Port = &PORTB;
	
uint8_t in1Mask = _BV(0);
uint8_t in2Mask = _BV(2);
uint8_t in3Mask = _BV(7);
uint8_t in4Mask = _BV(1);
uint8_t en1Mask = _BV(1);
uint8_t en2Mask = _BV(3);
uint8_t en3Mask = _BV(2);
uint8_t en4Mask = _BV(0);

float stepsPerRevolution = 200;
float stepperSpeed = 10; // RPM
float stepperRotationRequested = 1e9;
float stepperRotationCompleted = 0;
float power = 1;

float stepperPhase = 0;  // radians
unsigned long lastUpdateTime = 0;

void InitStepper() {
	pwm1.EnableOutputCompare(2);
	pwm1.EnableOutputCompare(7);	
}

void SetCoilA(float amount) {
	if (amount > 0) {
		pwm1.SetEvenDutyCycle(power*amount);
		*in1Port |=  in1Mask;
		*in2Port &= ~in2Mask;
	} else {
		pwm1.SetEvenDutyCycle(power*-amount);
		*in1Port &= ~in1Mask;
		*in2Port |=  in2Mask;		
	}
}

void SetCoilB(float amount) {
	if (amount > 0) {
		pwm1.SetOddDutyCycle(power*amount);
		*in3Port |=  in3Mask;
		*in4Port &= ~in4Mask;
	} else {
		pwm1.SetOddDutyCycle(power*-amount);
		*in3Port &= ~in3Mask;
		*in4Port |=  in4Mask;		
	}
}




void UpdateStepper() {
	// dt is the number of seconds that have elapsed since the last update.
	unsigned long now = micros();
	float dt = (now-lastUpdateTime)*1e-6;
	lastUpdateTime = now;
	
	// The change in angle is the speed times the time delta.
	float dAngle = dt*stepperSpeed/60;
	
	// Clamp the angle change to the magnitude of the requested rotation
	if (dAngle > fabs(stepperRotationRequested)) {
		dAngle = fabs(stepperRotationRequested);
	}
	
	// Apply the requested rotation sign to the angle delta.
	if (stepperRotationRequested < 0) {
		dAngle *= -1;
	}
	
	// Transfer the angle delta from requested to completed
	stepperRotationRequested -= dAngle;
	stepperRotationCompleted += dAngle;
	
	//  Determine the new phase angle. Each step is pi/2 radians.
	stepperPhase += dAngle*stepsPerRevolution*(M_PI/2);
	
	// Clamp the phase angle between +pi and -pi
	while (stepperPhase > M_PI) {
		stepperPhase -= 2*M_PI;
	}
	while (stepperPhase < -M_PI) {
		stepperPhase += 2*M_PI;
	}
	
	// Set the coil currents to the sin/cos of the phase angle.
	SetCoilA(sin(stepperPhase));
	SetCoilB(cos(stepperPhase));
}

int main(void)
{
	//TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	ClockInit();

	// Enable the LED
	DDRA |= _BV(5);
	PORTA |= _BV(5);	

	DDRA |= 0xF | _BV(7);
	DDRB |= _BV(0) | _BV(1) | _BV(2);

	*en1Port |= en1Mask;
	*en2Port |= en2Mask;
	*en3Port |= en3Mask;
	*en4Port |= en4Mask;

	InitStepper();

	while(1)
	{
		UpdateStepper();
	}
}

float FastSin(float x) {
	while (x > M_PI) {
		x -= 2*M_PI;
	}
	while (x < -M_PI) {
		x += 2*M_PI;
	}
	
	// First term: x
	float result = x;
	
	// Second term: -x**3/3!
	float numerator = x*x*x;
	float denominator = 3*2;
	result -= numerator/denominator;
	
	// Third term: + x**5/5!
	numerator *= x*x;
	denominator *= 4*5;
	result += numerator/denominator;
	
	// Fourth term: -x**7/7!
	numerator *= x*x;
	denominator *= 6*7;
	result -= numerator/denominator;
	
	// Fifth term: +x**9/9!
	numerator *= x*x;
	denominator *= 8*9;
	result += numerator/denominator;
	
	// Sixth term: -x**11/11!
	numerator *= x*x;
	denominator *= 10*11;
	result -= numerator/denominator;
	
	// Seventh's term +x**13/13!
	numerator *= x*x;
	denominator *= 12*13;
	result += numerator/denominator;
	
	return result;
}

#endif
