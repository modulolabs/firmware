#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_GPIO)

#include <avr/io.h>
#include <avr/interrupt.h>

#include "Clock.h"
#include "PWM.h"
#include "Modulo.h"

/*
GPIO 0 - PA3 - PCINT3 - ADC3 - TOCC2
GPIO 1 - PA2 - PCINT2 - ADC2 - TOCC1
GPIO 2 - PA1 - PCINT1 - ADC1 - TOCC0
GPIO 3 - PB2 - PCINT10 - ADC8 - TOCC7
GPIO 4 - PA7 - PCINT7 - ADC7
GPIO 5 - PB0 - PCINT8 - ADC11
GPIO 6 - PB1 - PCINT9 - ADC10
GPIO 7 - PA0 - PCINT0 - ADC0
*/

static volatile uint8_t *GPIO_PORT[] = {
	&PORTA,
	&PORTA,
	&PORTA,
	&PORTB,
	&PORTA,
	&PORTB,
	&PORTB,
	&PORTA
};

static volatile uint8_t *GPIO_PUE[] = {
    &PUEA,
    &PUEA,
    &PUEA,
    &PUEB,
    &PUEA,
    &PUEB,
    &PUEB,
    &PUEA
};


static volatile uint8_t *GPIO_DDR[] = {
	&DDRA,
	&DDRA,
	&DDRA,
	&DDRB,
	&DDRA,
	&DDRB,
	&DDRB,
	&DDRA	
};

static volatile uint8_t *GPIO_PIN[] = {
	&PINA,
	&PINA,
	&PINA,
	&PINB,
	&PINA,
	&PINB,
	&PINB,
	&PINA
};

static const uint8_t GPIO_TIMER_COMPARE[] = {2,1,0,7};

static const uint8_t GPIO_PIN_NUMBER[] = {3, 2, 1, 2, 7, 0, 1, 0};

static const uint8_t GPIO_MASK[] = {
	_BV(3), _BV(2), _BV(1), _BV(2), _BV(7), _BV(0), _BV(1), _BV(0)
};

uint8_t GPIO_ADC[] = {
	3,2,1,8,7,11,10,0
};

PWM pwmA(1);
PWM pwmB(2);

static bool _analogReadStarted = false;
uint8_t _lowToHigh;
uint8_t _highToLow;

void SetDirection(uint8_t dir) {
	for (int i=0; i < 8; i++) {
		if (dir & _BV(i)) {
			*(GPIO_DDR[i]) |= GPIO_MASK[i];
		} else {
			*(GPIO_DDR[i]) &= ~GPIO_MASK[i];
		}
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
    *GPIO_DDR[pin] |= GPIO_MASK[pin];
    return true;
}

static bool _setPullup(uint8_t pin, bool value) {
    if (pin >= 8) {
        return false;
    }
    if (value) {
        *GPIO_PUE[pin] |= GPIO_MASK[pin];
    } else {
        *GPIO_PUE[pin] &= ~GPIO_MASK[pin];
    }
    return true;
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
    *GPIO_DDR[pin] &= ~GPIO_MASK[pin];
    return (*GPIO_PIN[pin] & GPIO_MASK[pin]);
}


static void _startAnalogRead(uint8_t pin) {
	// Set the channel
	ADMUXA = GPIO_ADC[pin];

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
    uint16_t val = high << 8 | low;
			
	return (high << 8) | low;
}

void _setPWMEnable(uint8_t enableBits) {
	pwmA.SetOutputCompareEnable(GPIO_TIMER_COMPARE[0], enableBits & _BV(0));
	pwmA.SetOutputCompareEnable(GPIO_TIMER_COMPARE[1], enableBits & _BV(1));
	pwmB.SetOutputCompareEnable(GPIO_TIMER_COMPARE[2], enableBits & _BV(2));
	pwmB.SetOutputCompareEnable(GPIO_TIMER_COMPARE[3], enableBits & _BV(3));
}

void _setPWMValue(uint8_t pin, uint16_t value) {
    *GPIO_DDR[pin] |= GPIO_MASK[pin];
	switch(pin) {
		case 0:
            pwmA.SetOutputCompareEnable(GPIO_TIMER_COMPARE[0], true);
			pwmA.SetEvenDutyCycle(value/65535.0);
			break;
		case 1:
            pwmA.SetOutputCompareEnable(GPIO_TIMER_COMPARE[1], true);
			pwmA.SetOddDutyCycle(value/65535.0);
			break;
		case 2:
            pwmB.SetOutputCompareEnable(GPIO_TIMER_COMPARE[2], true);
			pwmB.SetEvenDutyCycle(value/65535.0);
			break;
		case 3:
            pwmB.SetOutputCompareEnable(GPIO_TIMER_COMPARE[3], true);
			pwmB.SetOddDutyCycle(value/65535.0);
			break;
	}
}

void _setPWMFrequencyA(uint16_t frequency) {
	pwmA.SetFrequency(frequency);
}

void _setPWMFrequencyB(uint16_t frequency) {
	pwmB.SetFrequency(frequency);
}

DEFINE_MODULO_CONSTANTS("Integer Labs", "IO", 0, "http://www.integerlabs.net/docs/NavButtons");
DEFINE_MODULO_FUNCTION_NAMES("State,Pressed,Released");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeBitfield8, ModuloDataTypeBitfield8, ModuloDataTypeBitfield8);

enum GPIOCommands {
    FUNCTION_GET_DIGITAL_INPUT,
    FUNCTION_GET_DIGITAL_INPUTS,
    FUNCTION_GET_ANALOG_INPUT,
    FUNCTION_SET_DIGITAL_OUTPUT,
    FUNCTION_SET_DIGITAL_OUTPUTS,
    FUNCTION_SET_PWM_OUTPUT,
    FUNCTION_SET_PULLUP,
    FUNCTION_SET_DEBOUNCE,
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
            _startAnalogRead(buffer.Get<uint8_t>(0));
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
        case FUNCTION_SET_DEBOUNCE:
            if (buffer.GetSize() != 2) {
                return false;
            }
            return false;
        case FUNCTION_SET_PWM_FREQUENCY:
            return false;
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

int main(void)
{
	ClockInit();
	ModuloInit(&DDRA, &PORTA, _BV(5));

	while(1) {
	}
}

#endif