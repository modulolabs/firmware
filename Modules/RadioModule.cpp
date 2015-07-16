/*
 * RadioModule.cpp
 *
 * Created: 3/4/2015 2:20:24 PM
 *  Author: ekt
 */ 


#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_RADIO)

#include "Modulo.h"
#include "Clock.h"
#include <avr/delay.h>

const char *ModuloDeviceType = "co.modulo.radio";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "Radio Module";
const char *ModuloDocURL = "modulo.co/docs/radio";




bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	return false;
}

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
    return false;
}

void ModuloReset() {

}


bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
}

// All on Port A
#define MISO_PIN 0
#define MOSI_PIN 1
#define SS_PIN 2
#define SCK_PIN 3

void SPIInit() {
	REMAP |= _BV(SPIMAP);
	SPCR |= _BV(SPE) | _BV(MSTR);
	
	DDRA |= _BV(MOSI_PIN) | _BV(SS_PIN) | _BV(SCK_PIN);

	uint8_t status = SPSR;
	uint8_t data = SPDR;
}

uint8_t SPITransfer(uint8_t d) {

	
	SPDR = d;
	// Wait for the transfer to complete
	while (!(SPSR & _BV(SPIF)));
	

	
	// Read the data
	return SPDR;
}

void SPIWrite(uint8_t address, uint8_t value) {

	PORTA &= ~_BV(SS_PIN);
	SPITransfer(address | _BV(7));
	SPITransfer(value);
	PORTA |= _BV(SS_PIN);
}

uint8_t SPIRead(uint8_t address) {
	PORTA &= ~_BV(SS_PIN);
	SPITransfer(address);
	uint8_t result = SPITransfer(0);
	PORTA |= _BV(SS_PIN);
	return result;
}

#define RegFifo 0
#define RegOpMode 1
#define RegDataModul 2
#define RegBitrateMsb 3
#define RegBitrateLsb 4
#define RegFdevMsb 5
#define RegFdevLsb 6
#define RegFrfMsb 7
#define RegFrfMid 8
#define RegFrfLsb 9
#define RefOsc1 0xA
#define RegAfcCtrl 0xB
#define RegLowBat 0xC
#define RegListen1 0xD
#define RegListen2 0xE
#define RegListen3 0xF
#define RegListen4 0x10
#define RegVersion 0x10
#define RegPaLevel 0x11
#define RegPaRamp 0x12
#define RegOcp 0x13
#define RegLna 0x18
#define RegRxBw 0x19
#define RegAfcBw 0x1A
#define RegOokPeak 0x1B
#define RegOokAvg 0x1C
#define RegOokFix 0x1D
#define RegAfcFei 0x1E
#define RegAfcMsb 0x1F
#define RegAfcLsb 0x20
#define RegFeiMsb 0x21
#define RegFeiLsb 0x22
#define RegRssiConfig 0x23
#define RegRssiValue 0x24

#define RegDioMapping1 0x25
#define RegDioMappiong2 0x26
#define RegIrqFlags1 0x27
#define RegIrqFlags2 0x28
#define RegRssiThresh 0x29
#define RegRxTimeout1 0x2A
#define RegRexRxTimeout2 0x2B


void InitRadio() {
	SPIWrite(RegOpMode, 
	    //(1 << 6) | // Listen On
	    (4 << 2)); // Receiver Mode
	SPIWrite(RegDataModul,
		(3 << 5) | // Continuous without bit snchronizer
		(1 << 3) // OOK
	); 
	
	// Configure for 433Mhz operation.
	// Step frequency is 61Hz. 433Mhz/61 = 0x6c4ff8
	SPIWrite(RegFrfMsb, 0x6c);
	SPIWrite(RegFrfMid, 0x4f);
	SPIWrite(RegFrfLsb, 0xf8);
	
	SPIWrite(RegOokPeak,
	    0b00 << 6 | // fixed
		0b11 << 3); // 2.0db
		
}

int main(void)
{
    ClockInit();
    ModuloInit(&DDRA, &PORTA, _BV(5));

	ModuloSetStatus(ModuloStatusBlinking);
	SPIInit();

	volatile uint8_t testVal = SPIRead(3);
	InitRadio();

	while (1) {

		asm("nop");
		_delay_ms(500);
	}
}

#endif
