/*
 * IRModule.cpp
 *
 * Created: 3/23/2015 5:36:41 PM
 *  Author: ekt
 */ 

#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include <util/delay.h>
#include "IRremote/IRremote.h"
#include "IRremote/IREncoding.h"


const char *ModuloDeviceType = "co.modulo.ir";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "IR Remote";
const char *ModuloDocURL = "modulo.co/docs/irremote";

#define IR_SENSE_PIN 3
#define IR_LED_PIN 1
#define DATA_LED_PIN 0
#define STATUS_LED_PIN 2


#define FUNCTION_RECEIVE 0
#define FUNCTION_SEND 1
#define FUNCTION_CAN_SEND 2

#define EVENT_RECEIVED 0


static const int MAX_CODES_RECEIVED = 10;
volatile uint8_t _numCodesReceived = 0;
volatile IRCode _codesReceived[MAX_CODES_RECEIVED];
volatile IRCode _codeToSend;
volatile bool _hasCodeToSend = false;

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {

	switch(command) {
		case FUNCTION_RECEIVE:
			if (_numCodesReceived) {
				volatile IRCode *code = &_codesReceived[_numCodesReceived-1];
				buffer->AppendValue<uint8_t>((uint8_t)code->protocol);
				buffer->AppendValue<uint8_t>((uint32_t)code->data);
			}
			_numCodesReceived--;
			return true;
		case FUNCTION_CAN_SEND:
			buffer->AppendValue<uint8_t>(!_hasCodeToSend);
			return true;
	}

	return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_RECEIVE:
		case FUNCTION_CAN_SEND:
			return buffer.GetSize() == 0;
		case FUNCTION_SEND:
			if (_hasCodeToSend) {
				return false;
			}
			_codeToSend.protocol = buffer.Get<uint8_t>(0);
			_codeToSend.data = buffer.Get<uint32_t>(1);
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

IRrecv irrecv;
IRsend send;
decode_results irDecodeResults;

void SendCode() {

}

void ReceiveCode() {

}

int main(void)
{
	ClockInit();
	
	ModuloInit(&DDRA, &PORTA, _BV(STATUS_LED_PIN));

	DDRA |= _BV(DATA_LED_PIN);
	
	//send.enableIROut(38);
	//send.mark(0);
	irrecv.enableIRIn();

	while(1)
	{
		if (_hasCodeToSend) {
			SendCode();
		}
		if (irrecv.decode(&irDecodeResults)) {
			ModuloSetStatus(ModuloStatusOn);
			asm("nop");
			ReceiveCode();
			delay(100);
			ModuloSetStatus(ModuloStatusOff);
			irrecv.resume();
		}
		bool val = !(PINA & _BV(IR_SENSE_PIN));
		if (val) {
			PORTA |= _BV(DATA_LED_PIN);
			asm("nop");
		} else {
			PORTA &= ~_BV(DATA_LED_PIN);
			asm("nop");
		}
		
		//send.sendRC5(12345, 16);
		//_delay_ms(100);
	}
}

#endif
