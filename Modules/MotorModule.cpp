/*
 * MotorModule.cpp
 *
 * Created: 8/22/2014 9:55:37 AM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_MOTOR)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include "Modulo.h"
#include "PWM.h"
#include "Clock.h"

DEFINE_MODULO_CONSTANTS("Integer Labs", "Motor", 0, "http://www.integerlabs.net/docs/Motor");
DEFINE_MODULO_FUNCTION_NAMES("EnA,FreqA,OutA1,OutA2,EnB,FreqB,OutB1,OutB2");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeBool, ModuloDataTypeUInt16, ModuloDataTypeUInt16, ModuloDataTypeUInt16,
							 ModuloDataTypeBool, ModuloDataTypeUInt16, ModuloDataTypeUInt16, ModuloDataTypeUInt16);

enum FunctionCode {
	FunctionCodeEnA,
	FunctionCodeFreqA,
	FunctionCodeOutA1,
	FunctionCodeOutA2,
	FunctionCodeEnB,
	FunctionCodeFreqB,
	FunctionCodeOutB1,
	FunctionCodeOutB2,	
};

/*
 * INA1 PA2 TOCC1
 * INA2 PA1 TOCC0
 * INB1 PB2 TOCC7
 * INB2 PA7 TOCC6
 * ENABLEA PB1 
 * ENABLEB PA3 TOCC2
 */

#define INA1_PORT PORTA
#define INA2_PORT PORTA
#define INB1_PORT PORTB
#define INB2_PORT PORTA
#define ENABLEA_PORT PORTB
#define ENABLEB_PORT PORTA

#define INA1_DDR DDRA
#define INA2_DDR DDRA
#define INB1_DDR DDRB
#define INB2_DDR DDRA
#define ENABLEA_DDR DDRB
#define ENABLEB_DDR DDRA

#define INA1_MASK _BV(2)
#define INA2_MASK _BV(1)
#define INB1_MASK _BV(2)
#define INB2_MASK _BV(7)
#define ENABLEA_MASK _BV(1)
#define ENABLEB_MASK _BV(3)

PWM pwmA(1);
PWM pwmB(1);

void _ReadModuloValue(uint8_t functionID, ModuloBuffer *buffer) {
	
}

void SetEnable(volatile uint8_t *PORT, uint8_t mask, bool value) {
	if (value) {
		*PORT |= mask;
	} else {
		*PORT &= ~mask;
	}
}

void _WriteModuloValue(uint8_t functionID, const ModuloBuffer &buffer)
{
	switch ((FunctionCode)functionID) {
		case FunctionCodeEnA:
			SetEnable(&ENABLEA_PORT, ENABLEA_MASK, buffer.Get<bool>());
			break;
		case FunctionCodeFreqA:
			pwmA.SetFrequency(buffer.Get<uint16_t>());
			break;
		case FunctionCodeOutA1:
			pwmA.SetOddDutyCycle(buffer.Get<uint16_t>()/65535.0);
			SetEnable(&ENABLEA_PORT, ENABLEA_MASK, true);
			break;
		case FunctionCodeOutA2:
			pwmA.SetEvenDutyCycle(buffer.Get<uint16_t>()/65535.0);
			SetEnable(&ENABLEA_PORT, ENABLEA_MASK, true);
			break;		
		case FunctionCodeEnB:
			SetEnable(&ENABLEB_PORT, ENABLEB_MASK, buffer.Get<bool>());
			break;
		case FunctionCodeFreqB:
			pwmB.SetFrequency(buffer.Get<uint16_t>());
			break;
		case FunctionCodeOutB1:
			pwmB.SetOddDutyCycle(buffer.Get<uint16_t>()/65535.0);
			SetEnable(&ENABLEB_PORT, ENABLEB_MASK, true);
			break;
		case FunctionCodeOutB2:
			pwmA.SetEvenDutyCycle(buffer.Get<uint16_t>()/65535.0);
			SetEnable(&ENABLEB_PORT, ENABLEB_MASK, true);
			break;
	}
	
}

int main(void)
{
	ModuloInit(&DDRA, &PORTA, _BV(5), _ReadModuloValue, _WriteModuloValue);
	ClockInit();


	INA1_DDR |= INA1_MASK;
	INA2_DDR |= INA2_MASK;
	INB1_DDR |= INB1_MASK;
	INB2_DDR |= INB2_MASK;
	
	pwmA.EnableOutputCompare(0);
	pwmA.EnableOutputCompare(1);
	pwmB.EnableOutputCompare(6);
	pwmB.EnableOutputCompare(7);
	
	ENABLEA_DDR |= ENABLEA_MASK;
	ENABLEB_DDR |= ENABLEB_MASK;
	
	//ENABLEA_PORT |= ENABLEA_MASK;
	//ENABLEB_PORT |= ENABLEB_MASK;
	
	//ENABLEB_PORT |= ENABLEB_MASK;
	//INB1_PORT |= INB1_MASK;
	//INA2_PORT |= INA2_MASK;
	
	/*
	pwmA.SetEvenDutyCycle(.5);
	pwmA.SetOddDutyCycle(.5);
	pwmB.SetEvenDutyCycle(0);
	pwmB.SetOddDutyCycle(.5);
	*/
	
	while(1)
	{
		
		//PORTB |=  _BV(1);

		asm("nop");

	}
}


#endif