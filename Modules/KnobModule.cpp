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

/*
  LED - PA7
  A   - PB0
  B   - PB1
  SW  - PB2
  R   - PA1 - TOCC0
  G   - PA2 - TOCC1
  B   - PA3 - TOCC2
*/

#define INPUT_PINS PINB
#define INPUT_PUE PUEB
#define ENCA_PIN 0
#define ENCB_PIN 1
#define SW_PIN 2

#define LED_PORT PORTA
#define LED_DDR DDRA
#define RED_PIN 1
#define GREEN_PIN 2
#define BLUE_PIN 3
#define STATUS_PIN 7



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


PWM pwm1(1);
PWM pwm2(2);

void SetRGB(float r, float g, float b) {
    // Apply a gamma of 2.2 and set the duty cycle
    // The duty cycle is inverted because the LEDs are on when the output is low.
    pwm1.SetEvenDutyCycle(1.0-pow(r, 2.2));
    pwm1.SetOddDutyCycle(1.0-pow(g, 2.2));
    pwm2.SetEvenDutyCycle(1.0-pow(b, 2.2));
}

void SetHSV(float h, float s, float v) {
    float r, g, b;
    HSVToRGB(h, s, v, &r, &g, &b);
    SetRGB(r,g,b);
}


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

const char *ModuloDeviceType = "co.modulo.Knob";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Knob";
const char *ModuloDocURL = "modulo.co/docs/Knob";

#define FUNCTION_GET_STATE  0
#define FUNCTION_ADD_OFFSET 1
#define FUNCTION_SET_COLOR  2

volatile uint8_t _buttonState = false;
volatile uint8_t _buttonPressed = false;
volatile uint8_t _buttonReleased = false;
volatile Decoder _decoder;


bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    switch(buffer.GetCommand()) {
        case FUNCTION_GET_STATE:
            return true;
        case FUNCTION_ADD_OFFSET:
            if (buffer.GetSize() != 2) {
                return false;
            }
            _decoder.AddPositionOffset(buffer.Get<int16_t>(0));
            break;
        case FUNCTION_SET_COLOR:
            if (buffer.GetSize() != 3) {
                return false;
            }
            SetRGB(buffer.Get<uint8_t>(0)/255.,
                   buffer.Get<uint8_t>(1)/255.,
                   buffer.Get<uint8_t>(2)/255.);
            return true;
    }
    return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    bool state = _buttonState;
    bool pressed = _buttonPressed;
    bool released = _buttonReleased;
    switch(writeBuffer.GetCommand()) {
        case FUNCTION_GET_STATE:
        buffer->AppendValue<int16_t>(_decoder.GetPosition());
        buffer->AppendValue<uint8_t>(state);
        buffer->AppendValue<uint8_t>(pressed);
        buffer->AppendValue<uint8_t>(released);
        _buttonPressed = false;
        _buttonReleased = false;
        return true;
    }
    return false;
}



int main(void)
{
	//TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	//Init();
    ClockInit();
    ModuloInit(&DDRA, &PORTA, _BV(7));
    ModuloSetStatus(ModuloStatusBlinking);
    
	// Enable pullups on the A/B pins
	PUEB |= _BV(ENCA_PIN) | _BV(ENCB_PIN);

    // Set LED pins to outputs
    LED_DDR |= _BV(RED_PIN) | _BV(GREEN_PIN) | _BV(BLUE_PIN);
    
    pwm1.EnableOutputCompare(0); // Red
    pwm1.EnableOutputCompare(1); // Green
    pwm2.EnableOutputCompare(2); // Blue

    Debouncer debouncer;

    float hue = 0;
	while(1)
	{
        _decoder.Update(INPUT_PINS & _BV(ENCA_PIN), INPUT_PINS & _BV(ENCB_PIN));
        debouncer.Update(INPUT_PINS & _BV(SW_PIN));

        noInterrupts();
        if (!_buttonState and debouncer.Get()) {
            _buttonPressed = true;
        }
        if (_buttonState and !debouncer.Get()) {
            _buttonReleased = true;
        }
        _buttonState = debouncer.Get();
        interrupts();

        //_delay_ms(10);

        /*
        
        

        if (debouncer.Get() and !previousSW) {
           hue += .5;
        }
        previousSW = debouncer.Get();

        hue += decoder.GetPosition()/24.0;
        decoder.SetPosition(0);
        SetHSV(hue, 1, 1);
        */
	}
}

#endif
