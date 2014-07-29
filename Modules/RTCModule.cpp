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
#include "Modulo.h"
#include "Timer.h"
#include "I2CMaster.h"

#define TEMPERATURE_REGISTER 7
#define YEAR_REGISTER 6
#define MONTH_REGISTER 5
#define DATE_REGISTER 4
#define DAY_REGISTER 3
#define HOUR_REGISTER 2
#define MINUTE_REGISTER 1
#define SECOND_REGISTER 0

DEFINE_MODULO_CONSTANTS("Integer Labs", "RTC", 0, "http://www.integerlabs.net/docs/RTC");
DEFINE_MODULO_FUNCTION_NAMES("Seconds,Minutes,Hours,Day,Date,Month,Year,Temp");
DEFINE_MODULO_FUNCTION_TYPES(ModuloDataTypeUInt8, ModuloDataTypeUInt8, ModuloDataTypeUInt8, ModuloDataTypeUInt8, ModuloDataTypeUInt8, ModuloDataTypeUInt8,
	ModuloDataTypeUInt16, ModuloDataTypeFloat);


struct RTC_Registers {
	// Register 0
	unsigned int seconds: 4;
	unsigned int tenSeconds : 4;
	
	// Register 1
	unsigned int minutes: 4;
	unsigned int tenMinutes:3;
	bool minutesPadding:1;

	// Register 2
	unsigned int hour: 4;
	bool tenHour: 1;	
	bool twentyHour: 1;
	bool twelveHourMode: 1;
    bool hourPadding:1;
	
	// Register 3
	unsigned int day:3;
	bool dayPadding:5;
	
	// Register 4
	unsigned int date: 4;
	unsigned int tenDate: 2;
	unsigned int datePadding: 2;

	// Register 5
	unsigned int month: 4;
	unsigned int tenMonth: 1;
	unsigned int monthPadding:2;
	bool century: 1;

	// Register 6
	unsigned int year: 4;
	unsigned int tenYear: 4;
};

struct RTC_Calendar {
	uint16_t year;
	uint8_t month;
	uint8_t date;
	uint8_t day;
	uint8_t hours;
	uint8_t minutes;
	uint8_t seconds;
	float temperature;
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
	for (unsigned int i=0; i < sizeof(registers); i++) {
		data[i] = I2C_Read(i != sizeof(registers)-1);
	}
	I2C_Stop();
	
	calendar.year = 100*(registers.century+20) + registers.tenYear*10 + registers.year;
	calendar.month = registers.tenMonth*10 + registers.month;
	
	calendar.date = registers.tenDate*10 + registers.date;
	calendar.day = registers.day;
	calendar.hours = registers.tenHour*10 + registers.hour;
	calendar.minutes = registers.tenMinutes*10 + registers.minutes;
	calendar.seconds = registers.tenSeconds*10 + registers.seconds;
	
	I2C_Start();
	I2C_Write(i2cAddress << 1);
	I2C_Write(0x11); // word address of temperature upper byte
	I2C_Stop();
	
	I2C_Start();
	I2C_Write(i2cAddress << 1 | 1);
	int8_t tempHigh = I2C_Read(true);
	uint8_t tempLow = I2C_Read(false);
	I2C_Stop();
	calendar.temperature = float(tempHigh) + tempLow/4.0;
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
	for (unsigned int i=0; i < sizeof(registers); i++) {
		I2C_Write(data[i]);
	}
	I2C_Stop();
}


void _ReadModuloValue(uint8_t functionID, ModuloBuffer *buffer) {
	switch (functionID) {
		case TEMPERATURE_REGISTER:
			buffer->Set(calendar.temperature);
			break;
		case YEAR_REGISTER:
			buffer->Set(calendar.year);
			break;
		case MONTH_REGISTER:
			buffer->Set(calendar.month);
			break;
		case DATE_REGISTER:
			buffer->Set(calendar.date);
			break;
		case DAY_REGISTER:
			buffer->Set(calendar.day);
			break;
		case HOUR_REGISTER:
			buffer->Set(calendar.hours);
			break;
		case MINUTE_REGISTER:
			buffer->Set(calendar.minutes);
			break;
		case SECOND_REGISTER:
			buffer->Set(calendar.seconds);
			break;
	}
}

int main(void)
{
	TimerInit();
	ModuloInit(&DDRA, &PORTA, _BV(1), _ReadModuloValue);
	
	//I2CMaster i2c;
	I2C_Init();
	
#if 0
	calendar.year = 2014;
	calendar.month = 7;
	calendar.date = 28;
	calendar.day = 1;
	calendar.hours = 19;
	calendar.minutes = 25;
	calendar.seconds = 40;
	
	
	WriteRegisers();
#endif
	
	while (1) {
		ReadRegisters();
		_delay_ms(100);
	}
}

#endif