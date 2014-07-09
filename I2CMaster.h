/*
 * I2CMaster.h
 *
 * Created: 6/23/2014 4:16:08 PM
 *  Author: ekt
 */ 


#ifndef I2CMASTER_H_
#define I2CMASTER_H_

void I2C_Init();
void I2C_Start();
void I2C_Stop();
unsigned char I2C_Write(unsigned char c);
unsigned char I2C_Read(unsigned char ack);

#endif /* I2CMASTER_H_ */