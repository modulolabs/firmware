/*
 * RTCModule.cpp
 *
 * Created: 6/20/2014 2:43:13 PM
 *  Author: ekt
 */ 

#include "Setup.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_RTC)

#include <avr/io.h>
#include <avr/delay.h>
#include <avr/interrupt.h>
#include "TwoWire.h"
#include "I2CMaster.h"


/*
  LED - PA0
  A   - GPIO 4 - PA3
  COM - GPIO 3 - PA7
  B   - GPIO 2 - PB2
*/


class I2CMaster {
public:

	I2CMaster() {
		PUEB |= _BV(1) | _BV(0);
	}
	
	void SetSDA(bool high) {
		if (high) {
			DDRB |= _BV(1);
		} else {
			DDRB &= ~_BV(1);
		}
	}
	bool GetSDA() {
		return PORTB & _BV(1);
	}
	
	void SetSCL(bool high) {
		if (high) {
			DDRB |= _BV(0);
			
			// Client may clock stretch. Wait for the line to actually go high
			//while (!(PORTB & _BV(0)))  {
			//}
		} else {
			DDRB &= ~_BV(0);
		}
	}
	
	void Start() {
		// Start condition.
		// High->Low on SDA while SCL is high
		SetSCL(true);
		SetSDA(false);
	}
	
	void RepeatedStart() {
		SetSCL(false);
		SetSDA(true);
		
		Start();
	}
	
	void Stop() {
		// Stop condition
		// Low->High on SDA while SCL is high
		SetSCL(true);
		SetSDA(true);
	}
	
	bool SendByte(uint8_t data) {
		for (int i=8; i >= 0; i--) {
			SetSCL(false);
			SetSDA(data & _BV(i));
			SetSCL(true);
		}

		// Get ACK/NACK
		SetSCL(false);
		SetSDA(true);
		SetSCL(true);
		bool ack = !GetSDA();
		return ack;
	}
	
	bool SendAddress(uint8_t address, bool read) {
		return SendByte(address << 1 | read);
	}
};

static void _OnDataReceived(uint8_t *data, uint8_t numBytes) {

}

static void _OnDataRequested(uint8_t *data, uint8_t* numBytes, uint8_t maxLength) {

}

struct RTC_Registers {
	// Register 0
	int seconds: 4;
	int tenSeconds : 4;

	
	// Register 1
	int minutes: 4;
	int tenMinutes:4;

	
	// Register 2
	bool hour: 4;
	bool tenHour: 4;
	
	// Register 3
	uint8_t day;
	
	// Register 4
	int date: 4;
	int tenDate: 4;

	
	// Register 5
	int month: 4;
	int tenMonth: 3;
	bool century: 1;
	
	// Register 6
	int year: 4;
	int tenYear: 4;

};

struct RTC_Calendar {
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
};

RTC_Calendar calendar;
RTC_Registers registers;
uint8_t i2cAddress = 0x68;

void ReadRegisters() {
	
	I2C_Start();
	I2C_Write(i2cAddress << 1);
	I2C_Write(0); // word address
	I2C_Stop();
	
	_delay_ms(10);
	I2C_Start();
	I2C_Write(i2cAddress << 1 | 1);
	uint8_t *data = (uint8_t*)(&registers);
	for (int i=0; i < sizeof(registers); i++) {
		data[i] = I2C_Read(i != sizeof(registers)-1);
	}
	I2C_Stop();
	
	calendar.year = 2000 + 100*registers.century + registers.tenYear*10 + registers.year;
	calendar.month = registers.tenMonth*10 + registers.month;
	calendar.date = registers.tenDate*10 + registers.date;
	calendar.day = registers.day;
	calendar.hours = registers.tenHour*10 + registers.hour;
	calendar.minutes = registers.tenMinutes*10 + registers.minutes;
	calendar.seconds = registers.tenSeconds*10 + registers.seconds;
}

void WriteRegisers() {
	registers.seconds = calendar.seconds % 10;
	registers.tenSeconds = calendar.seconds / 10;
	registers.minutes = calendar.minutes % 10;
	registers.tenMinutes = calendar.minutes / 10;
	registers.hour = calendar.hours % 10;
	registers.tenHour = calendar.hours / 10;
	registers.day = calendar.day;
	registers.date = calendar.date % 10;
	registers.tenDate = calendar.date / 10;
	registers.month = calendar.month % 10;
	registers.tenMonth = calendar.month / 10;
	registers.year = calendar.year % 10;
	registers.tenYear = (calendar.year / 10) % 10;
	registers.century = (calendar.year / 100) - 20;
	
	I2C_Start();
	I2C_Write(i2cAddress << 1);
	I2C_Write(0); // word address
	/*
	I2C_Stop();
	
	_delay_ms(10);
	I2C_Start();
	I2C_Write(i2cAddress << 1);
	*/
	
	uint8_t *data = (uint8_t*)(&registers);
	for (int i=0; i < sizeof(registers); i++) {
		I2C_Write(data[i]);
	}
	I2C_Stop();
	
	
}

int main(void)
{
	TwoWireInit(MODULE_ADDRESS, _OnDataReceived, _OnDataRequested);
	Init();
	
	//I2CMaster i2c;
	I2C_Init();
	
#if 0
	calendar.year = 2014;
	calendar.month = 6;
	calendar.date = 24;
	calendar.day = 2;
	calendar.hours = 10;
	calendar.minutes = 49;
	calendar.seconds = 0;
	
	
	WriteRegisers();
#endif
	
	while (1) {
		ReadRegisters();
		_delay_ms(100);
	}
}

#endif