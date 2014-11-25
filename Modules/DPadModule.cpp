/*
 * ButtonsModule.cpp
 *
 * Created: 5/10/2014 9:06:29 PM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_DPAD)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include <util/delay.h>

volatile uint8_t buttonsState;
volatile uint8_t buttonsPressed;
volatile uint8_t buttonsReleased;

/*
 * LED - PB1
 * Button0 - PD0
 * Button1 - PD1
 * Button2 - PD2
 * Button3 - PD3
 * Button4 - PD4
 */

 /*
#define BUTTON_STATE_REGISTER 0
#define BUTTON_PRESSED_REGISTER 1
#define BUTTON_RELEASED_REGISTER 2

DEFINE_MODULO_CONSTANTS("Integer Labs", "NavButtons", 0, "http://www.integerlabs.net/docs/NavButtons");
DEFINE_MODULO_FUNCTION_NAMES("State,Pressed,Released");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeBitfield8, ModuloDataTypeBitfield8, ModuloDataTypeBitfield8);



void _ReadModuloValue(uint8_t functionID, ModuloBuffer *buffer) {
	switch (functionID) {
		case BUTTON_STATE_REGISTER:
			buffer->Set(buttonsState);
			break;
		case BUTTON_PRESSED_REGISTER:
			buffer->Set(buttonsPressed);
			buttonsPressed = 0;
			break;
		case BUTTON_RELEASED_REGISTER:
			buffer->Set(buttonsReleased);
			buttonsReleased = 0;
			break;			
	}
}
*/

#define FUNCTION_GET_DEVICE_ID 254

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
   switch(command) {
        case FUNCTION_GET_DEVICE_ID:
            buffer->AppendValue<uint16_t>(1348);
            return true;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    return false;
}

int main(void)
{
	ClockInit();
    
	ModuloInit(&DDRB, &PORTB, _BV(0));
	//_ReadModuloValue);
	

    DDRB |= _BV(1);


    // Enable pull-ups
    PORTD = 0x1f;
	
	while(1)
	{
    /*
		uint8_t newState = (
			(PINA & _BV(0) ? 0 : _BV(0)) |
			(PINA & _BV(1) ? 0 : _BV(1)) |
			(PINA & _BV(2) ? 0 : _BV(2)) |
			(PINA & _BV(3) ? 0 : _BV(3)) |
			(PINA & _BV(7) ? 0 : _BV(4)));
			
		for (int i=0; i < 5; i++) {
			uint8_t mask = _BV(i);
			if ((newState & mask) and !(buttonsState & mask)) {
				buttonsPressed |= mask;
			}
			if (!(newState & mask) and (buttonsState & mask)) {
				buttonsReleased |= mask;
			}		
		}
		
		buttonsState = newState;
		
		ModuloSetStatus(buttonsState ? ModuloStatusOn : ModuloStatusOff);
		
		_delay_ms(1);
        */
        volatile uint8_t val = PIND ^ 0x1f;
        if (val != 0) {
            asm("nop");
            PORTB |= _BV(1);
        } else {
            PORTB &= ~_BV(1);
        }
	}
}

#endif
