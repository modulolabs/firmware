/*
 * IREncoding.cpp
 *
 * Created: 3/30/2015 10:10:17 PM
 *  Author: ekt
 */ 
#include "IREncoding.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#define READ_WORD(address) (pgm_read_word(address))

static const PulseModulationEncoding IREncodings[NUM_IR_ENCODINGS] PROGMEM = {

// NEC
{
    .protocol = NEC,
    .headerMark = NEC_HDR_MARK,
    .headerSpace = NEC_HDR_SPACE,
    .numBits = 32,
    .oneMark = NEC_BIT_MARK,
    .oneSpace = NEC_ONE_SPACE,
    .zeroMark = NEC_BIT_MARK,
    .zeroSpace = NEC_ZERO_SPACE,
    .stopMark = NEC_BIT_MARK,
    .khz = 38
},

// Special NEC Repeat Codes
{
    .protocol = NEC,
    .headerMark = NEC_HDR_MARK,
    .headerSpace = NEC_RPT_SPACE,
    .numBits = 0,
    .oneMark = NEC_BIT_MARK,
    .oneSpace = NEC_ONE_SPACE,
    .zeroMark = NEC_BIT_MARK,
    .zeroSpace = NEC_ZERO_SPACE,
    .stopMark = NEC_BIT_MARK,
    .khz = 38
},

{
    .protocol = SONY,
    .headerMark = SONY_HDR_MARK,
    .headerSpace = SONY_HDR_SPACE,
    .numBits = SONY_BITS,
    .oneMark = SONY_ONE_MARK,
    .oneSpace = SONY_HDR_SPACE,
    .zeroMark = SONY_ZERO_MARK,
    .zeroSpace = SONY_HDR_SPACE,
    .stopMark = 0,
    .khz = 40
},

{
    .protocol = JVC,
    .headerMark = JVC_HDR_MARK,
    .headerSpace = JVC_HDR_SPACE,
    .numBits = JVC_BITS,
    .oneMark = JVC_BIT_MARK,
    .oneSpace = JVC_ONE_SPACE,
    .zeroMark = JVC_BIT_MARK,
    .zeroSpace = JVC_ZERO_SPACE,
    .stopMark = JVC_BIT_MARK,
    .khz = 38
},

{
    .protocol = JVC,
    .headerMark = 0, // No Header for repeat codes
    .headerSpace = 0, 
    .numBits = JVC_BITS,
    .oneMark = JVC_BIT_MARK,
    .oneSpace = JVC_ONE_SPACE,
    .zeroMark = JVC_BIT_MARK,
    .zeroSpace = JVC_ZERO_SPACE,
    .stopMark = JVC_BIT_MARK,
    .khz = 38
},

{
    .protocol = PANASONIC,
    .headerMark = PANASONIC_HDR_MARK,
    .headerSpace = PANASONIC_HDR_SPACE,
    .numBits = PANASONIC_BITS,
    .oneMark = PANASONIC_BIT_MARK,
    .oneSpace = PANASONIC_ONE_SPACE,
    .zeroMark = PANASONIC_BIT_MARK,
    .zeroSpace = PANASONIC_ZERO_SPACE,
    .stopMark = PANASONIC_BIT_MARK,
    .khz = 35
},

 {
    .protocol = LG,
    .headerMark = LG_HDR_MARK, 
    .headerSpace = LG_HDR_SPACE,
    .numBits = LG_BITS,
    .oneMark = LG_BIT_MARK,
    .oneSpace = LG_ONE_SPACE,
    .zeroMark = LG_BIT_MARK,
    .zeroSpace = LG_ZERO_SPACE,
    .stopMark = LG_BIT_MARK,
    .khz = 38
},


};


bool GetIREncoding(uint8_t i, PulseModulationEncoding *result) {
    if (i >= NUM_IR_ENCODINGS) {
        return false;
    }

	const PulseModulationEncoding *src = &IREncodings[i];
    result->protocol = READ_WORD(&src->protocol);
    if (result->protocol == 0) {
        return false;
    }

    result->headerMark = READ_WORD(&src->headerMark);
    result->headerSpace = READ_WORD(&src->headerSpace);
    result->numBits = READ_WORD(&src->numBits);
    result->oneMark = READ_WORD(&src->oneMark);
    result->oneSpace = READ_WORD(&src->oneSpace);
    result->zeroMark = READ_WORD(&src->zeroMark);
    result->zeroSpace = READ_WORD(&src->zeroSpace);
    result->stopMark = READ_WORD(&src->stopMark);
    result->khz = READ_WORD(&src->khz);
    return true;
}

