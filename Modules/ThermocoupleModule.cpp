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

#define INVALID_TEMP 0xFFFF
volatile int16_t currentTemp = INVALID_TEMP;
 
const char *ModuloDeviceType = "co.modulo.thermocouple";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Thermocouple";
const char *ModuloDocURL = "modulo.co/docs/Thermocouple";

#define AREF_1_1V  1
#define AREF_2_2V  2
#define AREF_4_096V  3

uint16_t AnalogRead(uint8_t channel, uint8_t aref)
{
    // Set the channel and use the AVCC reference
    ADMUXA = channel;
    ADMUXB = (aref << REFS0); // Select the voltage reference
    ADCSRA |= 3; // 128x prescaler
    //ADMUXB |= (1 << GSEL0); // 100x Gain




    // Enable the ADC, and start conversion
    ADCSRA |= _BV(ADEN) | _BV(ADSC);

    // Wait until the conversion has completed
    while (ADCSRA & _BV(ADSC))
        ;

    // Discard the first result since the channel or reference voltage
    // may have changed.
    // Start the conversion again.
    ADCSRA |= _BV(ADSC);

    // Wait until the conversion has completed
    while (ADCSRA & _BV(ADSC))
        ;

    // Must read low before high
    volatile uint8_t low = ADCL;
    volatile uint8_t high = ADCH;

    return (high << 8) | low;
}

uint16_t ReadTemp() {
    // Disable the digital input on channel 3 to save power
    DIDR0 |= _BV(3);

    // Use the smallest voltage reference that gives us a value that's within range
    // so we can get higher resolution readings for low temperatures.

    // First try reading with a 1.1V reference.
    // This range covers 0C to 218C, with a .213C resolution
    uint16_t temp = AnalogRead(3, AREF_1_1V);
    if (temp < 1023) {
        // Scaling factor is 1.1*10/(1024*.005) = 2.148375 ~= 64/30.
        temp *= 64;
        temp /= 30;
        return temp;
    }

    // Now try reading with a 2.2V reference.
    // This range covers 218C to 436C with a .426C resolution
    temp = AnalogRead(3, AREF_2_2V);
    if (temp < 1023) {
        // Scaling factor is 2.2*10/(1024*.005) = 4.296875 ~= 64/15
        temp *= 64;
        temp /= 15;
        return temp;
    }

    // Finally, read using a 4.096V reference
    // This range covers 436C to 817.6C with a .8C resolution
    temp = AnalogRead(3, AREF_4_096V);

    // An overrange value here indicates that the maximum temperature
    // has been exceeded or no probe is connected.
    if (temp == 1023) {
        return INVALID_TEMP;
    }

    // Scaling factor is 4.096*10/(1024*.005) = 8
    return temp*8;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    if (buffer.GetCommand() == 0) {
        return true;
    }

    return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    if (command == 0) {
        buffer->AppendValue<uint16_t>(currentTemp);
        return true;
    }
    return false;
}



int main(void)
{
	ClockInit();
	ModuloInit(&DDRA, &PORTA, _BV(7));
	
	while(1)
	{
        // Read the new temperature
        uint16_t newTemp = ReadTemp();

        // Disable interrupts when storing the new temperature
        noInterrupts();
        currentTemp = newTemp;
        interrupts();
	

		asm("nop");
		_delay_ms(500);
	}
}

#endif
