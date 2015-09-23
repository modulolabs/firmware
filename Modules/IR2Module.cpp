/*
 * IR2Module.cpp
 *
 * Created: 9/12/2015 9:16:55 AM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR2)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include <util/delay.h>
#include "IR2/IR2.h"



const char *ModuloDeviceType = "co.modulo.ir";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Modulo Labs";
const char *ModuloProductName = "IR Remote";
const char *ModuloDocURL = "modulo.co/docs/irremote";

#define IR_SENSE_PIN 0
#define IR_LED_PIN 1
#define STATUS_LED_PIN 5

#define FUNCTION_RECEIVE 0
#define FUNCTION_SEND 1
#define FUNCTION_CAN_SEND 2

#define EVENT_RECEIVED 0


uint32_t receivedCode = 0;
int8_t receivedProtocol = -1;

bool _hasCodeToSend = false;

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {

	switch(command) {
		case FUNCTION_RECEIVE:
			if (receivedProtocol != -1) {
				buffer->AppendValue<uint8_t>(receivedProtocol);
				buffer->AppendValue<uint32_t>(receivedCode);
				receivedProtocol = -1;
				return true;
			}
			return false;
		case FUNCTION_CAN_SEND:
			buffer->AppendValue<uint8_t>(!_hasCodeToSend);
			return true;
	}

	return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_RECEIVE:
		return false;
		case FUNCTION_CAN_SEND:
		return buffer.GetSize() == 0;
		case FUNCTION_SEND:
		if (_hasCodeToSend) {
			return false;
		}
		/*
		_codeToSend.protocol = buffer.Get<uint8_t>(0);
		_codeToSend.data = buffer.Get<uint32_t>(1);
		*/
		return true;
	}

	return false;
}

void ModuloReset() {

}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	#if 0
	if (buttonPressed or buttonReleased) {
		*eventCode = EVENT_BUTTON_CHANGED;
		*eventData = (buttonPressed << 8) | buttonReleased;
		return true;
	} else if (positionChanged) {
		*eventCode = EVENT_POSITION_CHANGED;
		*eventData = (hPos << 8) | (vPos);
		return true;
	}
	#endif
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
	#if 0
	if (eventCode == EVENT_BUTTON_CHANGED) {
		buttonPressed = false;
		buttonReleased = false;
	}
	if (eventCode == EVENT_POSITION_CHANGED) {
		positionChanged = false;
	}
	#endif
}


void SendCode() {

	_hasCodeToSend = false;
}

void ReceiveCode() {
	
}

int main(void)
{
	ClockInit();
	
	ModuloInit(&DDRA, &PORTA, _BV(STATUS_LED_PIN));

	IR2ReceiveEnable();

	//uint8_t mask = 0b10001110;
	
	//DDRA |= mask;
	//PORTA |= mask;
	
	//PORTA &= ~_BV(2);
	
	//send.enableIROut(38);
	//send.mark(1);
	//while (1) {
	
	//}

	while(1)
	{
		if (IR2IsReceiveComplete()) {
			if (receivedProtocol == -1) {
				if (IR2Decode(&receivedProtocol, &receivedCode)) {
					asm("nop");
				}
			}
			IR2ReceiveEnable();
		}
		
		bool val = !(PINA & _BV(IR_SENSE_PIN));
		if (val) {
			PORTA |= _BV(STATUS_LED_PIN);
			asm("nop");
		} else {
			PORTA &= ~_BV(STATUS_LED_PIN);
			asm("nop");
		}
		
		
		//send.sendRC5(12345, 16);
		//_delay_ms(100);
	}
}

#endif
