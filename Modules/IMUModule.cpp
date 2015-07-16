/*
 * IMUModule.cpp
 *
 * Created: 3/28/2015 10:56:18 PM
 *  Author: ekt
 */ 



#include "Config.h"

#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IMU)

#include <avr/io.h>
#include "Modulo.h"
#include "Clock.h"
#include <util/delay.h>
#include <I2CMaster.h>
#include <avr/io.h>
#include <stdlib.h>
#include "Clock.h"

volatile int16_t gyro[3];
volatile int16_t accel[3];
volatile int16_t magnetom[3];

const char *ModuloDeviceType = "co.modulo.imu";
const uint16_t ModuloDeviceVersion = 0;
const char *ModuloCompanyName = "Integer Labs";
const char *ModuloProductName = "IMU";
const char *ModuloDocURL = "modulo.co/docs/imu";

#define FUNCTION_GET_ACCEL 0
#define FUNCTION_GET_GYRO 1
#define FUNCTION_GET_MAG 2

bool ModuloRead(uint8_t command, const ModuloWriteBuffer &writeBuffer, ModuloReadBuffer *buffer) {
	
	switch(command) {
		case FUNCTION_GET_ACCEL:
		buffer->AppendValue<int16_t>((int16_t)accel[0]);
		buffer->AppendValue<int16_t>((int16_t)accel[1]);
		buffer->AppendValue<int16_t>((int16_t)accel[2]);
		return true;
		case FUNCTION_GET_GYRO:
		buffer->AppendValue<int16_t>((int16_t)gyro[0]);
		buffer->AppendValue<int16_t>((int16_t)gyro[1]);
		buffer->AppendValue<int16_t>((int16_t)gyro[2]);
		return true;
		case FUNCTION_GET_MAG:
		buffer->AppendValue<int16_t>((int16_t)magnetom[0]);
		buffer->AppendValue<int16_t>((int16_t)magnetom[1]);
		buffer->AppendValue<int16_t>((int16_t)magnetom[2]);
		return true;
	}
	return false;
}

bool ModuloWrite(const ModuloWriteBuffer &buffer) {
	switch (buffer.GetCommand()) {
		case FUNCTION_GET_ACCEL:
		case FUNCTION_GET_GYRO:
		case FUNCTION_GET_MAG:
		return buffer.GetSize() == 0;
	}
	return false;
}

void ModuloReset() {

}

bool ModuloGetEvent(uint8_t *eventCode, uint16_t *eventData) {
	/*
	if (buttonPressed or buttonReleased) {
		*eventCode = EVENT_BUTTON_CHANGED;
		*eventData = (buttonPressed << 8) | buttonReleased;
		return true;
	} else if (positionChanged) {
		*eventCode = EVENT_POSITION_CHANGED;
		*eventData = (hPos << 8) | (vPos);
		return true;
	}
	*/
	return false;
}

void ModuloClearEvent(uint8_t eventCode, uint16_t eventData) {
/*
	if (eventCode == EVENT_BUTTON_CHANGED) {
		buttonPressed = false;
		buttonReleased = false;
	}
	if (eventCode == EVENT_POSITION_CHANGED) {
		positionChanged = false;
	}
*/
}

/* This file is part of the Razor AHRS Firmware */

// I2C code to read the sensors


// Sensor I2C addresses
#define ACCEL_ADDRESS ((int) 0x53) // 0x53 = 0xA6 / 2
#define MAGN_ADDRESS  ((int) 0x1E) // 0x1E = 0x3C / 2
#define GYRO_ADDRESS  ((int) 0x68) // 0x68 = 0xD0 / 2

#define WIRE_SEND(b) Wire.write(b)
#define WIRE_RECEIVE() Wire.read()

void Accel_Init()
{
	Wire.beginTransmission(ACCEL_ADDRESS);
	WIRE_SEND(0x2D);  // Power register
	WIRE_SEND(0x08);  // Measurement mode
	Wire.endTransmission();
	delay(5);
	Wire.beginTransmission(ACCEL_ADDRESS);
	WIRE_SEND(0x31);  // Data format register
	WIRE_SEND(0x08);  // Set to full resolution
	Wire.endTransmission();
	delay(5);
	
	// Because our main loop runs at 50Hz we adjust the output data rate to 50Hz (25Hz bandwidth)
	Wire.beginTransmission(ACCEL_ADDRESS);
	WIRE_SEND(0x2C);  // Rate
	WIRE_SEND(0x09);  // Set to 50Hz, normal operation
	Wire.endTransmission();
	delay(5);
}

// Reads x, y and z accelerometer registers
void Read_Accel()
{
	int i = 0;
	uint8_t buff[6];
	
	Wire.beginTransmission(ACCEL_ADDRESS);
	WIRE_SEND(0x32);  // Send address to read from
	Wire.endTransmission();
	
	//Wire.beginTransmission(ACCEL_ADDRESS);
	Wire.requestFrom(ACCEL_ADDRESS, 6);  // Request 6 bytes
	while(Wire.available())  // ((Wire.available())&&(i<6))
	{
		buff[i] = WIRE_RECEIVE();  // Read one byte
		i++;
	}
	//Wire.endTransmission();
	
	if (i == 6)  // All bytes received?
	{
		// No multiply by -1 for coordinate system transformation here, because of double negation:
		// We want the gravity vector, which is negated acceleration vector.
		noInterrupts();
		accel[0] = (((int) buff[1]) << 8) | buff[0];  // X axis (internal sensor y axis)
		accel[1] = (((int) buff[3]) << 8) | buff[2];  // Y axis (internal sensor x axis)
		accel[2] = (((int) buff[5]) << 8) | buff[4];  // Z axis (internal sensor z axis)
		interrupts();
		
	}
}

void Magn_Init()
{
	Wire.beginTransmission(MAGN_ADDRESS);
	WIRE_SEND(0x02);
	WIRE_SEND(0x00);  // Set continuous mode (default 10Hz)
	Wire.endTransmission();
	delay(5);

	Wire.beginTransmission(MAGN_ADDRESS);
	WIRE_SEND(0x00);
	WIRE_SEND(0b00011000);  // Set 50Hz
	Wire.endTransmission();
	delay(5);
}

#define HW__VERSION_CODE 10724

void Read_Magn()
{
	int i = 0;
	uint8_t buff[6];
	
	Wire.beginTransmission(MAGN_ADDRESS);
	WIRE_SEND(0x03);  // Send address to read from
	Wire.endTransmission();
	
	Wire.requestFrom(MAGN_ADDRESS, 6);  // Request 6 bytes
	while(Wire.available())  // ((Wire.available())&&(i<6))
	{
		buff[i] = WIRE_RECEIVE();  // Read one byte
		i++;
	}
	
	if (i == 6)  // All bytes received?
	{
		noInterrupts();
		
		// 9DOF Razor IMU SEN-10125 using HMC5843 magnetometer
		#if HW__VERSION_CODE == 10125
		// MSB byte first, then LSB; X, Y, Z
		magnetom[0] = -1 * ((((int) buff[2]) << 8) | buff[3]);  // X axis (internal sensor -y axis)
		magnetom[1] = -1 * ((((int) buff[0]) << 8) | buff[1]);  // Y axis (internal sensor -x axis)
		magnetom[2] = -1 * ((((int) buff[4]) << 8) | buff[5]);  // Z axis (internal sensor -z axis)
		// 9DOF Razor IMU SEN-10736 using HMC5883L magnetometer
		#elif HW__VERSION_CODE == 10736
		// MSB byte first, then LSB; Y and Z reversed: X, Z, Y
		magnetom[0] = -1 * ((((int) buff[4]) << 8) | buff[5]);  // X axis (internal sensor -y axis)
		magnetom[1] = -1 * ((((int) buff[0]) << 8) | buff[1]);  // Y axis (internal sensor -x axis)
		magnetom[2] = -1 * ((((int) buff[2]) << 8) | buff[3]);  // Z axis (internal sensor -z axis)
		// 9DOF Sensor Stick SEN-10183 and SEN-10321 using HMC5843 magnetometer
		#elif (HW__VERSION_CODE == 10183) || (HW__VERSION_CODE == 10321)
		// MSB byte first, then LSB; X, Y, Z
		magnetom[0] = (((int) buff[0]) << 8) | buff[1];         // X axis (internal sensor x axis)
		magnetom[1] = -1 * ((((int) buff[2]) << 8) | buff[3]);  // Y axis (internal sensor -y axis)
		magnetom[2] = -1 * ((((int) buff[4]) << 8) | buff[5]);  // Z axis (internal sensor -z axis)
		// 9DOF Sensor Stick SEN-10724 using HMC5883L magnetometer
		#elif HW__VERSION_CODE == 10724
		// MSB byte first, then LSB; Y and Z reversed: X, Z, Y
		magnetom[0] = (((int) buff[0]) << 8) | buff[1];         // X axis (internal sensor x axis)
		magnetom[1] = ((((int) buff[2]) << 8) | buff[3]);  // Y axis (internal sensor -y axis)
		magnetom[2] = ((((int) buff[4]) << 8) | buff[5]);  // Z axis (internal sensor -z axis)
		#endif
		
		interrupts();
	}
}

void Gyro_Init()
{
	// Power up reset defaults
	Wire.beginTransmission(GYRO_ADDRESS);
	WIRE_SEND(0x3E);
	WIRE_SEND(0x80);
	Wire.endTransmission();
	delay(5);
	
	// Select full-scale range of the gyro sensors
	// Set LP filter bandwidth to 42Hz
	Wire.beginTransmission(GYRO_ADDRESS);
	WIRE_SEND(0x16);
	WIRE_SEND(0x1B);  // DLPF_CFG = 3, FS_SEL = 3
	Wire.endTransmission();
	delay(5);
	
	// Set sample rato to 50Hz
	Wire.beginTransmission(GYRO_ADDRESS);
	WIRE_SEND(0x15);
	WIRE_SEND(0x0A);  //  SMPLRT_DIV = 10 (50Hz)
	Wire.endTransmission();
	delay(5);

	// Set clock to PLL with z gyro reference
	Wire.beginTransmission(GYRO_ADDRESS);
	WIRE_SEND(0x3E);
	WIRE_SEND(0x00);
	Wire.endTransmission();
	delay(5);
}

// Reads x, y and z gyroscope registers
void Read_Gyro()
{
	int i = 0;
	uint8_t buff[6];
	
	Wire.beginTransmission(GYRO_ADDRESS);
	WIRE_SEND(0x1D);  // Sends address to read from
	Wire.endTransmission();
	
	Wire.requestFrom(GYRO_ADDRESS, 6);  // Request 6 bytes
	while(Wire.available())  // ((Wire.available())&&(i<6))
	{
		buff[i] = WIRE_RECEIVE();  // Read one byte
		i++;
	}
	
	if (i == 6)  // All bytes received?
	{
		noInterrupts();
		gyro[0] = ((((int) buff[0]) << 8) | buff[1]);    // X axis (internal sensor -y axis)
		gyro[1] = ((((int) buff[2]) << 8) | buff[3]);    // Y axis (internal sensor -x axis)
		gyro[2] = ((((int) buff[4]) << 8) | buff[5]);    // Z axis (internal sensor -z axis)
		interrupts();
	}
}

static const int UPDATE_RATE = 20;

int main(void)
{
	ClockInit();
	
	ModuloInit(&DDRA, &PORTA, _BV(5));
	
	I2CInit(&PORTA, &DDRA, &PINA, 2,
		&PORTA, &DDRA, &PINA, 3);
		
	Accel_Init();
	Gyro_Init(); 
	Magn_Init();
	
	unsigned long previousUpdate = 0;
	
	while(1)
	{
		/*
		unsigned long t = millis();
		if (t < previousUpdate) {
			// This only happens when millis() rolls over.
			previousUpdate = t;
		}
		if (t < previousUpdate+UPDATE_RATE) {
			continue;
		}
		previousUpdate = t;
		
		*/
		
		Read_Accel();
		Read_Gyro();
		Read_Magn();
		
		//delay(10);
		
		/*
		if (gyro[0] > 10) {
			ModuloSetStatus(ModuloStatusOn);
		} else {
			ModuloSetStatus(ModuloStatusOff);
		}
		*/
	}
}

#endif
