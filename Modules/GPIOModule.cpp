#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_GPIO)

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#include "Clock.h"
#include "PWM.h"
#include "Modulo.h"

const char *ModuloDeviceType = "co.modulo.io";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "IO Module";
const char *ModuloDocURL = "modulo.co/docs/io";

/*
GPIO 0 - PB2 - PCINT10 - ADC8 - TOCC7
GPIO 1 - PA2 - PCINT2 - ADC2 - TOCC1
GPIO 2 - PA1 - PCINT1 - ADC1 - TOCC0
GPIO 3 - PA0 - PCINT0 - ADC0
GPIO 4 - PA3 - PCINT3 - ADC3 - TOCC2
GPIO 5 - PB0 - PCINT8 - ADC11
GPIO 6 - PB1 - PCINT9 - ADC10
GPIO 7 - PA7 - PCINT7 - ADC7
*/



static volatile uint8_t *GPIO_PORT[] = {
	&PORTB,
	&PORTA,
	&PORTA,
	&PORTA,
	&PORTA,
	&PORTB,
	&PORTB,
	&PORTA
};

static volatile uint8_t *GPIO_PUE[] = {
    &PUEB,
    &PUEA,
    &PUEA,
    &PUEA,
    &PUEA,
    &PUEB,
    &PUEB,
    &PUEA
};


static volatile uint8_t *GPIO_DDR[] = {
	&DDRB,
	&DDRA,
	&DDRA,
	&DDRA,
	&DDRA,
	&DDRB,
	&DDRB,
	&DDRA	
};

static volatile uint8_t *GPIO_PIN[] = {
	&PINB,
	&PINA,
	&PINA,
	&PINA,
	&PINA,
	&PINB,
	&PINB,
	&PINA
};

static const int8_t GPIO_TIMER_COMPARE[] = {7, 1, 0, -1, 2, -1, -1, -1};

static const uint8_t GPIO_PIN_NUMBER[] = {2, 2, 1, 0, 3, 0, 1, 7};

static const uint8_t GPIO_MASK[] = {
	_BV(2), _BV(2), _BV(1), _BV(0), _BV(3), _BV(0), _BV(1), _BV(7)
};

static const uint8_t GPIO_ADC[] = {
	8, 2, 1, 0, 3, 11, 10, 7
};

static volatile uint16_t _pwmValues[8] = {0,0,0,0,0,0,0,0};

static volatile bool _analogReadStarted = false;

// The first two PWMs are on Timer1, the second two are on Timer2
// The output compares are 2, 1, 0, and 7.
PWM pwm7(2/*timer*/, 7 /*channel*/);
PWM pwm1(1/*timer*/,1 /*channel*/);
PWM pwm0(2/*timer*/, 0 /*channel*/);
PWM pwm2(1/*timer*/, 2 /*channel*/);

PWM *pwms[8] = {&pwm7, &pwm1, &pwm0, NULL, &pwm2, NULL, NULL, NULL};

void _setPWMFrequency(uint8_t pin, uint16_t frequency) {
    if (pwms[pin]) {
        pwms[pin]->SetFrequency(frequency);
    }
}

void _setDirection(uint8_t pin, bool output) {
    if (pin >= 8) {
        return;
    }
    if (output) {
        *(GPIO_DDR[pin]) |= GPIO_MASK[pin];
    } else {
        *(GPIO_DDR[pin]) &= ~GPIO_MASK[pin];
    }
}

void _setPWMValue(uint8_t pin, uint16_t value) {
    if (pin >= 8) {
        return;
    }

    _pwmValues[pin] = value;

    if (pwms[pin]) {
        pwms[pin]->SetValue(value);
        pwms[pin]->SetCompareEnabled(true);
    }

    _setDirection(pin, true);
}


void _setDirections(uint8_t dir) {
	for (int i=0; i < 8; i++) {
        _setDirection(i, dir & _BV(i));
	}
}

static void _setDigitalOutputs(uint8_t values) {
	for (int i=0; i < 8; i++) {
		if (values & _BV(i)) {
			*(GPIO_PORT[i]) |= GPIO_MASK[i];
		} else {
			*(GPIO_PORT[i]) &= ~GPIO_MASK[i];
		}
	}
}

static bool _setDigitalOutput(volatile uint8_t pin, volatile bool value) {
    if (pin >= 8) {
        return false;
    }
    if (value) {
        *GPIO_PORT[pin] |= GPIO_MASK[pin];
    } else {
        *GPIO_PORT[pin] &= ~GPIO_MASK[pin];
    }

    if (pwms[pin]) {
        pwms[pin]->SetCompareEnabled(false);
    }

    _setDirection(pin, true);
    return true;
}

static void _setPullup(uint8_t pin, bool value) {
    if (pin >= 8) {
        return;
    }
    if (value) {
        *GPIO_PUE[pin] |= GPIO_MASK[pin];
    } else {
        *GPIO_PUE[pin] &= ~GPIO_MASK[pin];
    }
}

static void _setPullups(uint8_t pullups) {
    for (int i=0; i < 8; i++) {
        _setPullup(i, pullups & _BV(i));
    }

}

static uint8_t _getDigitalInputs() {
	uint8_t value = 0;
	for (int i=0; i < 8; i++) {
		if (*GPIO_PIN[i] & GPIO_MASK[i]) {
			value |= _BV(i);
		}
	}
	return value;
}

static bool _getDigitalInput(uint8_t pin) {
    if (pin >= 8) {
        return false;
    }
    _setDirection(pin, false);

    return (*GPIO_PIN[pin] & GPIO_MASK[pin]);
}


static void _setAnalogInput(uint8_t pin) {
    _setDirection(pin, false);

	// Set the channel
	ADMUXA = GPIO_ADC[pin];
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


void _updateSoftPWM() {
    uint32_t softPWMFrequency = 50;
    volatile uint32_t t = micros()*softPWMFrequency*655/10000;
    t &= 0xFFFF;
    for (int i=0; i < 8; i++) {
		if (pwms[i]) {
			continue;
		}
        if (_pwmValues[i] == 0 or _pwmValues[i] == 0xFFFF) {
            continue;
        }

        if (t < _pwmValues[i]) {
            *GPIO_PORT[i] |= GPIO_MASK[i];
        } else {
            *GPIO_PORT[i] &= ~GPIO_MASK[i];
        }
    }
}


enum GPIOCommands {
    FUNCTION_GET_DIGITAL_INPUT,
    FUNCTION_GET_DIGITAL_INPUTS,
    FUNCTION_GET_ANALOG_INPUT,
    FUNCTION_SET_DATA_DIRECTION,
    FUNCTION_SET_DATA_DIRECTIONS,
    FUNCTION_SET_DIGITAL_OUTPUT,
    FUNCTION_SET_DIGITAL_OUTPUTS,
    FUNCTION_SET_PWM_OUTPUT,
    FUNCTION_SET_PULLUP,
    FUNCTION_SET_PULLUPS,
    FUNCTION_SET_DEBOUNCE,
    FUNCTION_SET_DEBOUNCES,
    FUNCTION_SET_PWM_FREQUENCY
};

bool ModuloWrite(const ModuloWriteBuffer &buffer)
{
    switch (buffer.GetCommand()) {
        case FUNCTION_GET_DIGITAL_INPUT:
            return buffer.GetSize() == 1;
        case FUNCTION_GET_DIGITAL_INPUTS:
            return true;
        case FUNCTION_GET_ANALOG_INPUT:
            if (buffer.GetSize() != 2) {
                return false;
            }
            _setAnalogInput(buffer.Get<uint8_t>(0));
            _startAnalogRead();
            return true;
        case FUNCTION_SET_DATA_DIRECTION:
            if (buffer.GetSize() != 2) {
                return false;
            }
            //_setDataDirection(buffer.Get<uint8_t>(0),
            //                  buffer.Get<uint8_t>(1));
            return true;
        case FUNCTION_SET_DATA_DIRECTIONS:
            if (buffer.GetSize() != 1) {
                return false;
            }
            //_setDataDirections(buffer.Get<uint8_t>(0));
            return true;
        case FUNCTION_SET_DIGITAL_OUTPUT:
            if (buffer.GetSize() != 2) {
                return false;
            }
            {
                volatile uint8_t pin = buffer.Get<uint8_t>(0);
                volatile uint8_t value = buffer.Get<uint8_t>(1);
                _setDigitalOutput(pin, value);                 
            }
            return true;
        case FUNCTION_SET_DIGITAL_OUTPUTS:
            if (buffer.GetSize() != 1) {
                return false;
            }
            _setDigitalOutputs(buffer.Get<uint8_t>(0));
            return true;
        case FUNCTION_SET_PWM_OUTPUT:
            if (buffer.GetSize() != 3) {
                return false;
            }
            _setPWMValue(buffer.Get<uint8_t>(0), buffer.Get<uint16_t>(1));
            return true;
        case FUNCTION_SET_PULLUP:
            if (buffer.GetSize() != 2) {
                return false;
            }
            _setPullup(buffer.Get<uint8_t>(0), buffer.Get<uint8_t>(1));
            return true;
        case FUNCTION_SET_PULLUPS:
            if (buffer.GetSize() != 1) {
                return false;
            }
            _setPullups(buffer.Get<uint8_t>(0));
            return true;
        case FUNCTION_SET_DEBOUNCE:
            if (buffer.GetSize() != 2) {
                return false;
            }
            //_setDebounce(buffer.Get<uint8_t>(0), buffer.Get<uint8_t>(1));
            return true;
        case FUNCTION_SET_DEBOUNCES:
            if (buffer.GetSize() != 1) {
                return false;
            }
            //_setDebounces(buffer.Get<uint8_t>(0));
            return true;
        case FUNCTION_SET_PWM_FREQUENCY:
            if (buffer.GetSize() != 3) {
                return false;
            }
            _setPWMFrequency(buffer.Get<uint8_t>(0), buffer.Get<uint8_t>(1));
            return true;
    }
    return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    switch (command) {
        case FUNCTION_GET_DIGITAL_INPUT:
            buffer->AppendValue(_getDigitalInput(writeBuffer.Get<uint8_t>(0)));
            return true;
        case FUNCTION_GET_DIGITAL_INPUTS:
            buffer->AppendValue(_getDigitalInputs());
            return true;
        case FUNCTION_GET_ANALOG_INPUT:
            buffer->AppendValue(_completeAnalogRead());
            return true;
    }
    return false;
}

void ModuloReset() {
    _setDirections(0);
    _setPullups(0);
	for (int i=0; i < 8; i++) {
		if (pwms[i]) {
			pwms[i]->SetCompareEnabled(false);
		}
	}
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}


uint16_t capSense(uint8_t pin) {
    // Make the pin an input and enable the pullup resistor
    // This charges the external capacitor to VCC
    *(GPIO_PUE[pin]) |= GPIO_MASK[pin];
    _delay_ms(1);

    // Do an A/D conversion with 0V as the input.
    // This charges the internal sample/hold capacitor to 0V
    ADMUXA = 14;
    ADMUXB = 0;
    _startAnalogRead();
    _completeAnalogRead();

    // Disable the pullup
    *(GPIO_PUE[pin]) &= ~GPIO_MASK[pin];

    // Do an A/D conversion from the input pin. This connects
    // the external and internal capacitors in parallel, so the
    // voltage will be proportional to the external capacitance
    _setAnalogInput(pin);
    _startAnalogRead();
    volatile uint16_t result = _completeAnalogRead();

    asm("nop");
    return result;

}

int main(void)
{
	ClockInit();
	ModuloInit(&DDRA, &PORTA, _BV(5));

	while(1) {
        _updateSoftPWM();
/*
        if (capSense(0) > 750) {
            ModuloSetStatus(ModuloStatusOn);
        } else {
            ModuloSetStatus(ModuloStatusOff);
        }
*/
	}
}

#endif