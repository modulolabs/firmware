/*
 * IREncoding.h
 *
 * Created: 3/30/2015 10:09:58 PM
 *  Author: ekt
 */ 


#ifndef IRENCODING_H_
#define IRENCODING_H_

#include "IRremoteInt.h"

#define PULSE_MODULATION_MAX_HEADER_LENGTH 4
#define NUM_IR_ENCODINGS 25

// Most IR Protocols use either pulse width or pulse distance encoding
// and vary primarily in the header pulses, specific timing, number of bits.
// PulseModulationEncoding allows a protocol's encoding to be specified
// in terms of those details.
struct PulseModulationEncoding {
    uint16_t protocol;
    uint16_t headerMark;
    uint16_t headerSpace;
    uint16_t numBits;
    uint16_t oneMark;
    uint16_t oneSpace;
    uint16_t zeroMark;
    uint16_t zeroSpace;
    uint16_t stopMark;
    uint16_t khz;
};

bool GetIREncoding(uint8_t i, PulseModulationEncoding *result);

struct IRCode {
    int32_t data;
    int8_t protocol;
};

#endif /* IRENCODING_H_ */