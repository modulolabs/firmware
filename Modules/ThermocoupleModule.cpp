/*
 * ThermocoupleModule.cpp
 *
 * Created: 8/1/2014 3:07:21 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_THERMOCOUPLE)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include <util/delay.h>

/*
 * LED - PA0
 * GPIO0 - PB0
 * GPIO1 - PB1
 * GPIO2 - PB2
 * GPIO5 - PA2
 */

#define TEMP1_REGISTER 0
#define INTERNAL_TEMP1_REGISTER 1
#define FAULT1_REGISTER 2
#define TEMP2_REGISTER 3
#define INTERNAL_TEMP2_REGISTER 4
#define FAULT2_REGISTER 5

DEFINE_MODULO_CONSTANTS("Integer Labs", "Thermocouple", 0, "http://www.integerlabs.net/docs/NavButtons");
DEFINE_MODULO_FUNCTION_NAMES("Temp1,InternalTemp1,Fault1,Temp2,InternalTemp2,Fault2");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeFloat, ModuloDataTypeFloat, ModuloDataTypeBitfield8,
						     ModuloDataTypeFloat, ModuloDataTypeFloat, ModuloDataTypeBitfield8);
	
#define SCK_PORT PORTB
#define SCK_DDR DDRB
#define MISO_PIN PINA
#define CS_PORT PORTB
#define CS_DDR DDRB
#define SCK_MASK _BV(2)
#define MISO_MASK _BV(7)
#define CS1_MASK _BV(0)
#define CS2_MASK _BV(1)

class Thermocouple {
public:
	Thermocouple(int csMask) : _csMask(csMask), _filterRate(.2), _fault(1) {
		
	}
	
	float GetTemp() {
		return _filteredTemp;
	}
	float GetInternalTemp() {
		return _filteredInternalTemp;
	}
	uint8_t GetFault() {
		return _fault;
	}

	void Update() {
		// CS low
		CS_PORT &= ~_csMask;
		
		// 14 bits of temperature data
		_temp = 0;
		for(int i=0; i < 14; i++) {
			_temp <<= 1;
			_temp |= _ReadBit();
		}
		
		// 1 reserved bit
		_ReadBit();
		
		bool hadFault = _fault;
		
		// 1 general fault bit
		_fault = _ReadBit();
		
		// 12 bits of internal temperature data
		_internalTemp = 0;
		for (int i=0; i < 12; i++) {
			_internalTemp <<= 1;
			_internalTemp |= _ReadBit();
		}
		
		// 1 reserved bit
		_ReadBit();
		
		// 3 more fault bits
		for (int i=0; i < 3; i++) {
			_fault <<= 1;
			_fault |= _ReadBit();
		}
		
		// CS high
		CS_PORT |= _csMask;
		
		// If we just recovered from a fault (or at startup) use 100% of the new value
		float rate = hadFault ? 1.0 : _filterRate;
		
		_filteredInternalTemp = (_internalTemp/16.0)*rate + (1.0-rate)*_filteredInternalTemp;
		if (_fault == 0) {
			_filteredTemp =  (_temp/4.0)*rate + (1.0-rate)*_filteredTemp;
		}
	}

private :
	bool _ReadBit() {
		SCK_PORT |= SCK_MASK;        // sck high
		delay_us(1);
		bool bit = !(MISO_PIN & MISO_MASK); // read from miso. inverted due to mosfet
		SCK_PORT &= ~SCK_MASK;       // sck low
		return bit;
	}
	
	uint8_t _csMask;
	int16_t _temp;
	int16_t _internalTemp;
	uint8_t _fault;
	float _filterRate;
	float _filteredTemp;
	float _filteredInternalTemp;
};

Thermocouple thermocouple1(CS1_MASK);
Thermocouple thermocouple2(CS2_MASK);
ModuloVariable<float> temp1, temp2, internalTemp1, internalTemp2;
ModuloVariable<uint8_t> fault1, fault2;

void _ReadModuloValue(uint8_t functionID, ModuloBuffer *buffer) {
	switch (functionID) {
		case TEMP1_REGISTER:
			buffer->Set(temp1.Get());
			break;
		case INTERNAL_TEMP1_REGISTER:
			buffer->Set(internalTemp1.Get());
			break;
		case FAULT1_REGISTER:
			buffer->Set(fault1.Get());
			break;
		case TEMP2_REGISTER:
			buffer->Set(temp2.Get());
			break;
		case INTERNAL_TEMP2_REGISTER:
			buffer->Set(internalTemp2.Get());
			break;
		case FAULT2_REGISTER:
			buffer->Set(fault2.Get());
			break;
	}
}




int main(void)
{
	ClockInit();
	ModuloInit(&DDRA, &PORTA, _BV(1), _ReadModuloValue);

	SCK_DDR |= SCK_MASK;
	CS_DDR |= CS1_MASK | CS2_MASK;
	
	while(1)
	{
		thermocouple1.Update();
		thermocouple2.Update();
				
		fault1.Set(thermocouple1.GetFault());
		internalTemp1.Set(thermocouple1.GetInternalTemp());
		if (not thermocouple1.GetFault()) {
			temp1.Set(thermocouple1.GetTemp());
		}
				
		fault2.Set(thermocouple2.GetFault());
		internalTemp2.Set(thermocouple2.GetInternalTemp());
		if (not thermocouple2.GetFault()) {
			temp2.Set(thermocouple2.GetTemp());
		}
		
		_delay_ms(100);
	}
}

#endif
