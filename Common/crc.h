/*
 * crc.h
 *
 * Created: 10/26/2015 10:02:18 AM
 *  Author: ekt
 */ 


#ifndef CRC_H_
#define CRC_H_

#include <inttypes.h>

uint8_t crc_update(uint8_t inCrc, uint8_t inData);

#endif /* CRC_H_ */