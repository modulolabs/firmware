/*
 * KnobModule.cpp
 *
 * Created: 5/11/2014 10:19:44 AM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_KNOB)

#include <avr/io.h>
#include <avr/delay.h>
#include "PWM.h"
#include "Clock.h"
#include "Modulo.h"
#include "ModuloInfo.h"

DECLARE_MODULO("co.modulo.knob", 1);

/*
  LED - PA7
  A   - PB0
  B   - PB1
  SW  - PB2
  R   - PA1 - TOCC0
  G   - PA2 - TOCC1
  B   - PA3 - TOCC2
*/

#define ENC_PINS PINB
#define ENC_PUE PUEB
#define ENCA_PIN 0
#define ENCB_PIN 1

#define SW_PINS PINA
#define SW_PIN 2

#define RED_PIN 1
#define RED_DDR DDRA
#define RED_PORT PORTA

#define GREEN_PIN 7
#define GREEN_DDR DDRA
#define GREEN_PORT PORTA

#define BLUE_PIN 2
#define BLUE_DDR DDRB
#define BLUE_PORT PORTB

#define STATUS_PORT PORTA
#define STATUS_DDR DDRA
#define STATUS_PIN 5


/*
void HSVToRGB(float h, float s, float v,
              float *r, float *g, float *b) {
    *r = *g = *b = 0;

    // Put h in the range [0, 6)
    h = 6*(h - floor(h));

    float C = v*s;
    float X = C*(1-fabs(fmod(h, 2) - 1));

    switch (int(h)) {
        case 0:
            *r = C;
            *g = X;
            break;
        case 1:
            *r = X;
            *g = C;
            break;
        case 2:
            *g = C;
            *b = X;
            break;
        case 3:
            *g = X;
            *b = C;
            break;
        case 4:
            *r = X;
            *b = C;
            break;
        case 5:
            *r = C;
            *b = X;
            break;
    }
    
    float m = v-C;

    *r += m;
    *g += m;
    *b += m;
}

*/

PWM pwmRed(1,0);
PWM pwmGreen(2,6);
PWM pwmBlue(2,7);

// Input range is 0-255
void SetRGB(uint16_t r, uint16_t g, uint16_t b) {
    // Square the values to approximate a gamma curve
    // The duty cycle is inverted because the LEDs are on when the output is low.
    pwmRed.SetValue(65535-r*r);
    pwmGreen.SetValue(65535-g*g);
    pwmBlue.SetValue(65535-b*b);
}

/*
void SetHSV(float h, float s, float v) {
    float r, g, b;
    HSVToRGB(h, s, v, &r, &g, &b);
    SetRGB(r,g,b);
}
*/

class Debouncer {
public:
    // debouncePeriod in microseconds
    Debouncer(uint64_t debouncePeriod = 10000) :
        _debouncedState(0),
        _changeStartTime(0),
        _debouncePeriod(debouncePeriod) {
    }

    void Update(bool state) {
        if (state == _debouncedState) {
            _changeStartTime = micros();
            return;
        }
        if (micros() > _changeStartTime+_debouncePeriod) {
            _debouncedState = state;
            _changeStartTime = 0;
        }
    }

    bool Get() {
        return _debouncedState;
    }

private:
    bool _debouncedState;
    uint64_t _changeStartTime;
    uint64_t _debouncePeriod;
};

class Decoder {
public:

    Decoder() : _previousA(false), _position(0) {
    }

    int16_t GetPosition() volatile {
        return _position;
    }

    void AddPositionOffset(int16_t offset) volatile {
        _position += offset;
    }

    // Increment (or decrement) the position for every full quadrature cycle
    void Update(bool A, bool B) volatile {
        // At rest, encA and encB are high. Between each detent the encoder cycles
        // through the four possible states. We can therefore detect a change from
        // one detent to the next by looking for rising or falling edges on one pin
        // that occur while the other pin is low.
        if (!B) { // Only pay attention to changes in A while B is low.
            if (A and !_previousA) { // Rising edge: clockwise rotation
                _position++;
            }
            if (!A and _previousA) { // Falling edge: counterclockwise rotation
                _position--;
            }
        }
        _previousA = A;
    }

private:
    bool _previousA;
    int16_t _position;
};



#define FUNCTION_GET_BUTTON  0
#define FUNCTION_GET_POSITION 1
#define FUNCTION_ADD_OFFSET 2
#define FUNCTION_SET_COLOR  3

#define EVENT_BUTTON_CHANGED 0
#define EVENT_POSITION_CHANGED 1

volatile uint8_t _buttonState = false;
volatile uint8_t _buttonPressed = false;
volatile uint8_t _buttonReleased = false;
volatile int16_t _position = 0;
volatile bool _positionChanged = false;
volatile Decoder _decoder;


bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    switch(buffer.GetCommand()) {
        case FUNCTION_GET_BUTTON:
            return buffer.GetSize() == 0;
        case FUNCTION_ADD_OFFSET:
            if (buffer.GetSize() != 2) {
                return false;
            }
            _decoder.AddPositionOffset(buffer.Get<int16_t>(0));
            return true;
        case FUNCTION_SET_COLOR:
            if (buffer.GetSize() != 3) {
                return false;
            }
            SetRGB(buffer.Get<uint8_t>(0),
                   buffer.Get<uint8_t>(1),
                   buffer.Get<uint8_t>(2));
            return true;
    }
    return false;
}

bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
    bool state = _buttonState;
	int16_t pos = _position;
    switch(command) {
    case FUNCTION_GET_BUTTON:
        buffer->AppendValue<uint8_t>(state);
        return true;
    case FUNCTION_GET_POSITION:
        buffer->AppendValue<int16_t>(pos);
        return true;
    }
    return false;
}

void ModuloReset() {
    _decoder.AddPositionOffset(-_decoder.GetPosition());

    SetRGB(0,0,0);	
}


bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	if (_buttonPressed or _buttonReleased) {
		*eventCode = EVENT_BUTTON_CHANGED;
		*eventData = (_buttonPressed << 8) | _buttonReleased;
		return true;
	} else if (_positionChanged) {
		*eventCode = EVENT_POSITION_CHANGED;
		*eventData = _position;
		return true;
	}
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	if (eventCode == EVENT_BUTTON_CHANGED) {
		_buttonPressed = false;
		_buttonReleased = false;
	}
	if (eventCode == EVENT_POSITION_CHANGED) {
		_positionChanged = false;
	}
}

int main(void)
{

    ClockInit();
    ModuloInit(&STATUS_DDR, &STATUS_PORT, _BV(STATUS_PIN));

	SetRGB(0,0,0);
	
	pwmRed.SetFrequency(31250);
	pwmGreen.SetFrequency(31250);
	pwmBlue.SetFrequency(31250);
	
    pwmRed.SetCompareEnabled(true);
    pwmGreen.SetCompareEnabled(true);
    pwmBlue.SetCompareEnabled(true);


	ENC_PUE |= _BV(ENCA_PIN) | _BV(ENCB_PIN);
	
	RED_DDR |= _BV(RED_PIN);
	GREEN_DDR |= _BV(GREEN_PIN);
	BLUE_DDR |= _BV(BLUE_PIN);
	
    Debouncer debouncer;
	
	while(1)
	{
        _decoder.Update(ENC_PINS & _BV(ENCA_PIN), ENC_PINS & _BV(ENCB_PIN));
        debouncer.Update(SW_PINS & _BV(SW_PIN));

        noInterrupts();
		int16_t newPos = -_decoder.GetPosition();
		if (_position != newPos) {
			_positionChanged = true;
			_position = newPos;
		}
		
        if (!_buttonState and debouncer.Get()) {
            _buttonPressed = true;
        }
        if (_buttonState and !debouncer.Get()) {
            _buttonReleased = true;
        }
        _buttonState = debouncer.Get();
        interrupts();

		ModuloUpdateStatusLED();
	}
}

#endif
