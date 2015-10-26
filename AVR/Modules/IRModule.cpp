/*
 * IR3Module.cpp
 *
 * Created: 10/21/2015 2:33:20 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include "ADC.h"
#include <util/delay.h>
#include "IR.h"
#include "TwoWire.h"


const char *ModuloDeviceType = "co.modulo.ir";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Modulo Labs";
const char *ModuloProductName = "IR Remote";
const char *ModuloDocURL = "modulo.co/docs/irremote";

#define IR_SENSE_PIN 0
#define IR_LED_PIN 1
#define STATUS_LED_PIN 5

uint32_t receivedCode = 0;
int8_t receivedProtocol = -1;

bool _hasCodeToSend = false;

#define FUNCTION_GET_DATA 0
#define FUNCTION_GET_READ_SIZE 1
#define FUNCTION_CLEAR 2

uint8_t getDataMaxLen = 0;

volatile uint8_t receivedData[IR_BUFFER_SIZE];
volatile uint8_t receivedLen;

uint8_t nextReadOffset = 0;
uint8_t nextReadLen = 0;

int count = 0;
bool ModuloRead(uint8_t command, ModuloReadBuffer *buffer) {
	switch (command) {
		case FUNCTION_GET_DATA:
			{
				for (int i=0; i < 16; i++) {
					uint8_t val = receivedData[nextReadOffset++];
					buffer->AppendValue<uint8_t>(val);
				}
				return true;
			}
			return true;
		case FUNCTION_GET_READ_SIZE:
			buffer->AppendValue<uint8_t>((uint8_t)receivedLen);
			return true;
		break;
	}
	return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_GET_DATA:
			if (buffer.GetSize() == 2) {
				nextReadOffset = buffer.Get<uint8_t>(0);
				nextReadLen = buffer.Get<uint8_t>(1);
				return true;
			}
			break;
		case FUNCTION_GET_READ_SIZE:
			return (buffer.GetSize() == 0);
		case FUNCTION_CLEAR:
			receivedLen = 0;
			return true;
	}
		
	return false;
}

void ModuloReset() {

}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {

	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {

}


void SendCode() {

}

void ReceiveCode() {
	
}


static void sendMark() {
	// Ouput compares 0, 1, 2, 6, and 7 are all connected to the IR LED
	TOCPMCOE = _BV(0) | _BV(1) | _BV(2) | _BV(6) | _BV(7);

}

static void sendSpace() {
	// Sends an IR space for the specified number of microseconds.
	// A space is no output, so the PWM output is disabled.
	TOCPMCOE = 0;
}

static void configurePWM(int khz) {
	// Configure Timer1 for IR output waveform generation at the specified frequency.
	uint8_t modeA = 2; // Clear on compare match
	uint8_t modeB = 2; // Clear on compare match
	uint8_t wgm = 14;  // Fast PWM with TOP=ICR1
	uint8_t cs = 1;    // No prescaler

	TCCR1A =  (modeA << COM1A0) | (modeB << COM1B0) | (wgm & 0b11);
	TCCR1B =  ((wgm >> 2) << WGM12) | cs;
 
	// Top = F_CPU/(F*N)-1 where
	//    F is the desired frequency
	//    N is the clock prescaler (1, 8, 64, 256, or 1024

	volatile long topValue = F_CPU/khz/1000 - 1;

	// ICR1 is the top value.
	ICR1 = topValue;
	
	// OCR1 is the compare value.
	OCR1A = topValue/4;
	OCR1B = topValue/4;
	
	// We use multiple output pins to drive the IR LED. This lets us get more
	// led drive current than would be possible using a single output pin.
	
	// Set the DDR bits for the output pins on PA1, PA2, PA3, PA7, and PB2.
	DDRA |= _BV(1) | _BV(2) | _BV(3) | _BV(7);
	DDRB |= _BV(2);
	
	// Set the output compare MUX to OCR1A/OCR1B for each output pin
	// The outputs are TOCC0, TOCC1, TOCC2, TOCC6, and TOCC7
	TOCPMSA0 |= _BV(TOCC0S0) | _BV(TOCC1S0) | _BV(TOCC2S0);
	TOCPMSA1 |= _BV(TOCC6S0) | _BV(TOCC7S0);
}

int main(void)
{
	ClockInit();
	
	ModuloInit(&DDRA, &PORTA, _BV(STATUS_LED_PIN),
		false /*useTwoWireInterupt*/);

	configurePWM(38);

	
	while (1) {
		sendSpace();
		_delay_ms(1);
		sendMark();	
		_delay_ms(1);
	}
	

	IR3ReceiveEnable();

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
		TwoWireUpdate();
		
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

#endif
