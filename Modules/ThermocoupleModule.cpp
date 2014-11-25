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

volatile int16_t tempA = 0;
volatile int16_t tempB = 0;
 
DEFINE_MODULO_CONSTANTS("Integer Labs", "Thermocouple", 0, "http://www.integerlabs.net/docs/NavButtons");
DEFINE_MODULO_FUNCTION_NAMES("Temp1,InternalTemp1,Fault1,Temp2,InternalTemp2,Fault2");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeFloat, ModuloDataTypeFloat, ModuloDataTypeBitfield8,
						     ModuloDataTypeFloat, ModuloDataTypeFloat, ModuloDataTypeBitfield8);
	


uint16_t AnalogRead(uint8_t channel)
{
    // Set the channel and use the AVCC reference
    ADMUX =  channel;

    // Select 128x prescalar, enable the ADC, and start conversion
    ADCSRA |= _BV(ADPS2) | _BV(ADPS1) | _BV(ADPS0);
    ADCSRA |= _BV(ADEN) | _BV(ADSC);

    // Wait until the conversion has completed
    while (ADCSRA & _BV(ADSC))
        ;

    // Must read low before high
    volatile uint8_t low = ADCL;
    volatile uint8_t high = ADCH;

    return (high << 8) | low;
}

void UpdateTemp() {
    uint16_t newTempA = AnalogRead(6)*1.1*200*10/1024;
    uint16_t newTempB = AnalogRead(7)*1.1*200*10/1024;
    noInterrupts();
    tempA = newTempA;
    tempB = newTempB;
    interrupts();
}

#define FUNCTION_TEMPA 0
#define FUNCTION_TEMPB 1

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    switch (command) {
        case FUNCTION_TEMPA:
            buffer->AppendValue<uint16_t>(tempA);
            break;
        case FUNCTION_TEMPB:
            buffer->AppendValue<uint16_t>(tempB);
            break;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    if (buffer.GetCommand() < 3) {
        return true;
    }

    return false;
}

int main(void)
{
    // Discard the first results, then update the temperatures
    UpdateTemp();
    UpdateTemp();

	ClockInit();
	ModuloInit(&DDRA, &PORTB, _BV(1));
	
	while(1)
	{
		UpdateTemp();

		asm("nop");
		_delay_ms(500);
	}
}

#endif
