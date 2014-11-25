/*
 * RTCModule.cpp
 *
 * Created: 6/20/2014 2:43:13 PM
 *  Author: ekt
 */ 

#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_RTC)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include <avr/sleep.h>
#include "Modulo.h"
#include "Clock.h"
#include "I2CMaster.h"

namespace {
    struct RTC_Calendar {

        uint8_t seconds;
        uint8_t minutes;
        uint8_t hours;
        uint8_t days;
        uint8_t weekdays;
        uint8_t months;
        uint8_t years;
        bool clockSet;
        bool battLow;
    };
}

RTC_Calendar calendar;
ModuloVariable<float> _temperature;

static uint8_t _convertFromBCD(uint8_t bcd) {
    uint8_t low = bcd & 0b1111;
    uint8_t high = (bcd >> 4) & 0b1111;
    return high*10 + low;
}

static uint8_t _convertToBCD(uint8_t value) {
    return ((value/10) << 4) + (value % 10);
}

static void _readRegisters() {
    I2CBegin(104, false);
    I2CWrite(2); // Start at register 2
    I2CStop();

    uint8_t data[8];
    I2CBegin(104, true);
    for (int i=0; i < 8; i++) {
        data[i] = I2CRead(true);
    }
	I2CStop();

    calendar.battLow = data[0] & _BV(2);
    calendar.clockSet = !(data[1] & 0x80);
    calendar.seconds = _convertFromBCD(data[1] & 0b1111111);
	calendar.minutes = _convertFromBCD(data[2]);
    calendar.hours = _convertFromBCD(data[3]);
    calendar.days = _convertFromBCD(data[4]);
    calendar.weekdays = data[5];
    calendar.months = _convertFromBCD(data[6]);
    calendar.years = _convertFromBCD(data[7]);
    
}

static void _writeRegisters() {
    uint8_t data[8];

    data[0] = 0; // Enable battery switch-over
    data[1] = _convertToBCD(calendar.seconds);
    data[2] = _convertToBCD(calendar.minutes);
    data[3] = _convertToBCD(calendar.hours);
    data[4] = _convertToBCD(calendar.days);
    data[5] = _convertToBCD(calendar.weekdays);
    data[6] = _convertToBCD(calendar.months);
    data[7] = _convertToBCD(calendar.years);

    I2CBegin(104, false);
    I2CWrite(2); // Start at register 2
    for (int i=0; i < 8; i++) {
        I2CWrite(data[i]);
    }
    I2CStop();
}


ISR(ADC_vect)
{
	
	asm("nop");
}


uint16_t ReadADC(uint8_t channel)
{
#if defined(CPU_TINYX41)
	ADMUXA = channel;
#elif defined(CPU_TINYX8)
    ADMUX = channel;
#endif

	//ADMUXB = 4 << REFS0;
	ADCSRA = _BV(ADEN) | _BV(ADIE);
	set_sleep_mode(SLEEP_MODE_ADC);
	sleep_enable();
	sleep_cpu();
	
	// Wait for the conversion to complete
	while (ADCSRA & _BV(ADSC)) {
		sleep_cpu();
	}
	
	sleep_disable();
	sei();
	//ADCSRA &= ~_BV(ADIE);
	
	// Must read ADCL before ADCH;
	uint16_t adcResult = ADCL;
	adcResult |= (ADCH << 8);
	return adcResult;
}
	

#define THERMISTORNOMINAL 10000      
// temp. for nominal resistance (almost always 25 C)
#define TEMPERATURENOMINAL 25   
// how many samples to take and average, more takes longer
// but is more 'smooth'
#define NUMSAMPLES 500
// The beta coefficient of the thermistor (usually 3000-4000)
#define BCOEFFICIENT 3950
// the value of the 'other' resistor
#define SERIESRESISTOR 10000    

float ReadTemperature() {
	
  // take N samples in a row
  float average = 0;
  for (int i=0; i< NUMSAMPLES; i++) {
   average += ReadADC(2)/float(NUMSAMPLES);
  }
 
  // convert the value to resistance
  average = 1023 / average - 1;
  average = SERIESRESISTOR / average;

  float steinhart;
  steinhart = average / THERMISTORNOMINAL;     // (R/Ro)
  steinhart = log(steinhart);                  // ln(R/Ro)
  steinhart /= BCOEFFICIENT;                   // 1/B * ln(R/Ro)
  steinhart += 1.0 / (TEMPERATURENOMINAL + 273.15); // + (1/To)
  steinhart = 1.0 / steinhart;                 // Invert
  steinhart -= 273.15;                         // convert to C
 
  return steinhart;
}

#define FUNCTION_GET_TIME 0
#define FUNCTION_SET_TIME 1
#define FUNCTION_GET_TEMPERATURE 2
#define FUNCTION_GET_DEVICE_ID 254

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    switch(buffer.GetCommand()) {
        case FUNCTION_SET_TIME:
        case FUNCTION_GET_DEVICE_ID:
            return true;
        case FUNCTION_GET_TEMPERATURE:
        case FUNCTION_GET_TIME:
            return buffer.GetSize() == 0;
    }
    return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    switch(command) {
        case FUNCTION_GET_TIME:

            buffer->AppendValue<uint8_t>(calendar.seconds);
            buffer->AppendValue<uint8_t>(calendar.minutes);
            buffer->AppendValue<uint8_t>(calendar.hours);
            buffer->AppendValue<uint8_t>(calendar.days);
            buffer->AppendValue<uint8_t>(calendar.weekdays);
            buffer->AppendValue<uint8_t>(calendar.months);
            buffer->AppendValue<uint8_t>(calendar.years);
            buffer->AppendValue<uint8_t>(calendar.clockSet);
            buffer->AppendValue<uint8_t>(calendar.battLow);
            return true;
        case FUNCTION_GET_TEMPERATURE:
           // buffer->AppendValue<uint16_t>(ReadTemperature() * 10);
            return true;
        case FUNCTION_GET_DEVICE_ID:
            buffer->AppendValue<uint16_t>(875);
            return true;
    }
    return false;
}


int main(void)
{
	ClockInit();
	ModuloInit(&DDRA, &PORTA, _BV(1));

    I2CInit(&PORTD, &DDRD, &PIND, 1,
            &PORTD, &DDRD, &PIND, 2);

    
    /*
    calendar.years = 14;
    calendar.months = 10;
    calendar.days = 20;
    calendar.weekdays = 1;
    calendar.hours = 12;
    calendar.minutes = 21;
    _writeRegisters();
    */

	while (1) {
        _readRegisters();
		_delay_ms(100);
	}
}

#endif