/*
 * I2CMaster.h
 *
 * Created: 6/23/2014 4:16:08 PM
 *  Author: ekt
 */ 


#ifndef I2CMASTER_H_
#define I2CMASTER_H_

#include <inttypes.h>

void I2CInit(volatile uint8_t *sdaPORT, volatile uint8_t *sdaDDR, volatile uint8_t *sdaPIN, uint8_t sdaPin,
             volatile uint8_t *sclPORT, volatile uint8_t *sclDDR, volatile uint8_t *sclPIN, uint8_t sclPin);

bool I2CBegin(uint8_t address, bool read);
bool I2CWrite(uint8_t c);
unsigned char I2CRead(bool ack);
void I2CStop();


// Arduino API compatibility layer

class TwoWire {
public:
    void beginTransmission(uint8_t address);
    int write(uint8_t c);
    uint8_t endTransmission(void);


    void requestFrom(uint8_t address, uint8_t len);
    uint8_t available();
    uint8_t read();

private:
    uint8_t _bytesRemaining;
};

extern TwoWire Wire;

#endif /* I2CMASTER_H_ */