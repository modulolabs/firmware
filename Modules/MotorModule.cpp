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


const char *ModuloDeviceType = "co.modulo.motor";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Motor";
const char *ModuloDocURL = "modulo.co/docs/motor";

enum FunctionCode {
	FunctionSetValue,
    FunctionGetCurrent,
    FunctionSetEnabled,
    FunctionSetFrequency
};

/*
 * INA1 PA7 TOCC6
 * INA2 PB2 TOCC7 
 * INB1 PA1 TOCC0
 * INB2 PA2 TOCC1

 * ENABLEA PB1 
 * ENABLEB PA3 TOCC2
 * SENSEA PA0 ADC0
 * SENSEB PB0 ADC11
 */


volatile uint8_t *PORTS[] = {&PORTA, &PORTB, &PORTA, &PORTA};
volatile uint8_t *DDRS[]  = {&DDRA, &DDRB, &DDRA, &DDRA};
uint8_t MASKS[] = {_BV(7), _BV(2), _BV(1), _BV(2)};

volatile uint8_t *ENABLE_PORT = &PORTB;
volatile uint8_t *ENABLE_DDR = &DDRB;
uint8_t ENABLE_MASK = _BV(1);

#define FAULT_PORT PORTB
#define FAULT_DDR DDRB
#define FAULT_PUE PUEB
#define FAULT_PIN 0

PWM pwm[] = {PWM(2,6),  // Timer 2, output compare 6
			 PWM(2,7),  // Timer 2, output compare 7
			 PWM(1,0),  // Timer 1, output compare 0
			 PWM(1,1)}; // Timer 1, output compare 1

void ModuloReset() {
	*ENABLE_PORT &= ~ENABLE_MASK;
	*ENABLE_DDR |= ENABLE_MASK;
	 
    for (int i=0; i < 4; i++) {
        pwm[i].SetValue(0);
        pwm[i].SetCompareEnabled(true);
    }
}

void _setValue(uint8_t channel, uint16_t value) {
    if (channel >= 4) {
        return;
    }
    pwm[channel].SetValue(value);
}

void _setEnabled(bool enable) {
    if (enable) {
		*ENABLE_PORT |= ENABLE_MASK;
    } else {
		*ENABLE_PORT &= ~ENABLE_MASK;
    }
}

void _setFrequency(uint8_t channel, uint16_t frequency) {
    if (channel >= 4) {
        return;
    }
    pwm[channel].SetFrequency(frequency);
}


volatile bool _analogReadStarted = false;

static void _setAnalogInput(uint8_t channel) {
    // Set the channel
    ADMUXA = channel;
}

static void _startAnalogRead() {
    // enable, start conversion, and 64x prescaler
    ADCSRA |= _BV(ADEN) | _BV(ADSC) | _BV(ADPS1) | _BV(ADPS2);

    _analogReadStarted = true;
}

static uint16_t _completeAnalogRead() {
    if (!_analogReadStarted) {
        return 0;
    }
    _analogReadStarted = false;

    // Wait for conversion to complete
    while (ADCSRA & _BV(ADSC)) {
    }
    
    // Must read ADCL before ADCH
    uint8_t low = ADCL;
    uint16_t high = ADCH;
    
    return (high << 8) | low;
}

uint16_t _getCurrent(uint8_t channel) {
    if (channel < 2) {
        _setAnalogInput(0); // ADC0
    } else {
        _setAnalogInput(11); // ADC11
    }
    _startAnalogRead();
    return _completeAnalogRead();
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch ((FunctionCode)buffer.GetCommand()) {
    	case FunctionSetValue:
            if (buffer.GetSize() != 3) {
                return false;
            }
            _setValue(buffer.Get<uint8_t>(0), buffer.Get<uint16_t>(1));
            return true;
        case FunctionGetCurrent:
            return buffer.GetSize() == 0;
        case FunctionSetEnabled:
            if (buffer.GetSize() != 2) {
                return false;
            }
            _setEnabled(buffer.Get<uint8_t>(1));
            return true;
        case FunctionSetFrequency:
            if (buffer.GetSize() != 3) {
                return false;
            }
            _setFrequency(buffer.Get<uint8_t>(0), buffer.Get<uint16_t>(1));
            return true;
	}
	return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    switch (writeBuffer.GetCommand()) {
        case FunctionGetCurrent:
            buffer->AppendValue<uint16_t>(_getCurrent(0));
            buffer->AppendValue<uint16_t>(_getCurrent(2));
            return true;
    }
    return false;
}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
}

int main(void)
{
	ModuloInit(&DDRA, &PORTA, _BV(5));
	ClockInit();

	ModuloReset();
	
	for (int i=0; i < 4; i++) {
		*(DDRS[i]) |= MASKS[i];
		pwm[i].SetFrequency(32765/2);
	}
	
	FAULT_PUE |= _BV(FAULT_PIN);

#if 0
	_setEnabled(true);
	float i = 0.0;
	while (1) {
		float x = cos(i);
		float y = sin(i);
		
		if (x > 0) {
			_setValue(0, 0);
			_setValue(1, 65535*x);
		} else {
			_setValue(0, -65535*x);
			_setValue(1, 0);
		}
		
		if (y > 0) {
			_setValue(2, 0);
			_setValue(3, 65535*y);
		} else {
			_setValue(2, -65535*y);
			_setValue(3, 0);
		}		
		i += .5;
		
		//delayMicroseconds(1);
	}
#endif
	
	while(1) {
	}
}


#endif