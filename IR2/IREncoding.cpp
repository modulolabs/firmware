/*
 * IREncoding.cpp
 *
 * Created: 3/30/2015 10:10:17 PM
 *  Author: ekt
 */

#include "../Config.h"
#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR2)

#include "IREncoding.h"
#include <avr/eeprom.h>
#include <avr/pgmspace.h>

#define READ_WORD(address) (pgm_read_word(address))

#define NEC_HDR_MARK	9000
#define NEC_HDR_SPACE	4500
#define NEC_BIT_MARK	560
#define NEC_ONE_SPACE	1690
#define NEC_ZERO_SPACE	560
#define NEC_RPT_SPACE	2250

#define SONY_HDR_MARK	2400
#define SONY_HDR_SPACE	600
#define SONY_ONE_MARK	1200
#define SONY_ZERO_MARK	600
#define SONY_RPT_LENGTH 45000
#define SONY_DOUBLE_SPACE_USECS  500  // usually ssee 713 - not using ticks as get number wrapround

#define JVC_HDR_MARK 8000
#define JVC_HDR_SPACE 4000
#define JVC_BIT_MARK 600
#define JVC_ONE_SPACE 1600
#define JVC_ZERO_SPACE 550
#define JVC_RPT_LENGTH 60000

#define PANASONIC_HDR_MARK 3502
#define PANASONIC_HDR_SPACE 1750
#define PANASONIC_BIT_MARK 502
#define PANASONIC_ONE_SPACE 1244
#define PANASONIC_ZERO_SPACE 400

#define LG_HDR_MARK 8000
#define LG_HDR_SPACE 4000
#define LG_BIT_MARK 600
#define LG_ONE_SPACE 1600
#define LG_ZERO_SPACE 550
#define LG_RPT_LENGTH 60000


#define NEC_BITS 32
#define SONY_BITS 12
#define SANYO_BITS 12
#define MITSUBISHI_BITS 16
#define MIN_RC5_SAMPLES 11
#define MIN_RC6_SAMPLES 1
#define PANASONIC_BITS 48
#define JVC_BITS 16
#define LG_BITS 28
#define SAMSUNG_BITS 32
#define WHYNTER_BITS 32


static const PulseModulationEncoding IREncodings[NUM_IR_ENCODINGS] PROGMEM = {

	// NEC
	{
		.protocol = 1,
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
		.protocol = 2,
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
		.protocol = 3,
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
		.protocol = 4,
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
		.protocol = 5,
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
		.protocol = 6,
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
		.protocol = 7,
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

#endif
