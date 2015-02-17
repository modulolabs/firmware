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

volatile uint8_t buttonsState = 0;
volatile uint8_t buttonsPressed = 0;
volatile uint8_t buttonsReleased = 0;

const char *ModuloDeviceType = "co.modulo.dpad";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "DPad Module";
const char *ModuloDocURL = "modulo.co/docs/dpad";

#define FUNCTION_GET_BUTTONS 0

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
   switch(command) {
        case FUNCTION_GET_BUTTONS:
            uint8_t state = buttonsState;
            buffer->AppendValue<uint8_t>(state);
            return true;
    }
    return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
    if (buffer.GetCommand() == FUNCTION_GET_BUTTONS) {
        return buffer.GetSize() == 0;
    }
    return false;
}

void ModuloReset() {

}

int main(void)
{
	ClockInit();
    
	ModuloInit(&DDRA, &PORTA, _BV(7));

    // The buttons are on pins A0, A1, A2, A3, B0
    PUEA |= 0xF;
    PUEB |= 1;

	while(1)
	{
        noInterrupts();


        uint8_t newState = (PINA & 0xF) | ((PINB & 1) << 4);
	    newState ^= 0x1F;

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
		interrupts();


		//ModuloSetStatus(buttonsState ? ModuloStatusOn : ModuloStatusOff);
		
		_delay_ms(1);
	}
}

#endif
