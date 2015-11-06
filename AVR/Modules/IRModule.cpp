/*
 * IR3Module.cpp
 *
 * Created: 10/21/2015 2:33:20 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR)


#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include "IR.h"
#include "TwoWire.h"
#include "Modulo.h"
#include "ModuloInfo.h"

#include <avr/io.h>
#include <util/delay.h>

DECLARE_MODULO("co.modulo.ir", 1);

#define STATUS_LED_PIN 5
#define STATUS_PORT PORTA
#define STATUS_DDR DDRA

#define FUNCTION_RECEIVE 0
#define FUNCTION_GET_READ_SIZE 1
#define FUNCTION_CLEAR 2
#define FUNCTION_SET_SEND_DATA 3
#define FUNCTION_SEND 4
#define FUNCTION_IS_IDLE 5
#define FUNCTION_SET_BREAK_LENGTH 6

volatile uint8_t receivedData[IR_BUFFER_SIZE];
volatile uint8_t receivedLen = 0;

uint8_t sendData[IR_BUFFER_SIZE];
volatile uint8_t sendLen = 0;

uint8_t nextReadOffset = 0;
uint8_t nextReadLen = 0;

int count = 0;
bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
	switch (command) {
		case FUNCTION_RECEIVE:
			{
				for (int i=0; i < nextReadLen; i++) {
					uint8_t val = receivedData[nextReadOffset++];
					buffer->AppendValue<uint8_t>(val);
				}
				return true;
			}
			return true;
		case FUNCTION_GET_READ_SIZE:
			buffer->AppendValue<uint8_t>((uint8_t)receivedLen);
			return true;
		case FUNCTION_IS_IDLE:
			buffer->AppendValue<uint8_t>(sendLen == 0 and IRIsIdle());
			return true;
		break;
	}
	return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_RECEIVE:
			if (buffer.GetSize() == 2) {
				nextReadOffset = buffer.Get<uint8_t>(0);
				nextReadLen = buffer.Get<uint8_t>(1);
				return true;
			}
			return true;
		case FUNCTION_GET_READ_SIZE:
		case FUNCTION_IS_IDLE:
			return (buffer.GetSize() == 0);
		case FUNCTION_CLEAR:
			receivedLen = 0;
			return true;
		case FUNCTION_SET_SEND_DATA:
			if (buffer.GetSize() < 2) {
				return false;
			}
			{
				
				int offset = buffer.Get<uint8_t>(0);
				for (int i=0; i+1 < buffer.GetSize() and (i+offset) < IR_BUFFER_SIZE; i++) {
					sendData[i+offset] = buffer.Get<uint8_t>(i+1);
				}
				return true;
			}
		case FUNCTION_SEND:
			if (buffer.GetSize() != 1) {
				return false;
			}
			sendLen = buffer.Get<uint8_t>(0);
			return true;
		case FUNCTION_SET_BREAK_LENGTH:
			IRSetBreakLength(buffer.Get<uint16_t>(0));
			return true;
	}
		
	return false;
}

void ModuloReset() {
	IRSetBreakLength(512);
}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {

}

void onIRReadComplete(uint8_t *data, uint8_t len) {
	if (receivedLen != 0) {
		// If the previous code wasn't read yet, drop this one
		return;
	}
	
	receivedLen = len;
	for (int i=0; i < len; i++) {
		receivedData[i] = data[i];
	}
}

extern uint8_t _end;
extern uint8_t __stack;

void StackPaint(void) __attribute__((naked)) __attribute__((section (".init1")));

void StackPaint(void)
{
#if 0
    uint8_t *p = &_end;

    while(p <= &__stack)
    {
        *p = STACK_CANARY;
        p++;
    }
#else
    __asm volatile ("    ldi r30,lo8(_end)\n"
                    "    ldi r31,hi8(_end)\n"
                    "    ldi r24,lo8(0xc5)\n" /* STACK_CANARY = 0xc5 */
                    "    ldi r25,hi8(__stack)\n"
                    "    rjmp .cmp\n"
                    ".loop:\n"
                    "    st Z+,r24\n"
                    ".cmp:\n"
                    "    cpi r30,lo8(__stack)\n"
                    "    cpc r31,r25\n"
                    "    brlo .loop\n"
                    "    breq .loop"::);
#endif
}

int main(void)
{
	
	ClockInit();
	
	ModuloInit(&STATUS_DDR, &STATUS_PORT, _BV(STATUS_LED_PIN),
		false /*useTwoWireInterupt*/);

	IRInit();

	while(1)
	{
		TwoWireUpdate();

		if (sendLen) {
			cli();
			if (IRIsIdle()) {
				IRSend(sendData, sendLen);
				sendLen = 0;
			}
			sei();
		}

		if (IRIsIdle()) {
			ModuloUpdateStatusLED();
		} else {
			if (IRGetActivity()) {
				STATUS_PORT |= _BV(STATUS_LED_PIN);
			} else {
				STATUS_PORT &= ~_BV(STATUS_LED_PIN);
			}
		}
	}
}


#endif
