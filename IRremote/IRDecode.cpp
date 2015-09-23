/*
 * IRDecode.cpp
 *
 * Created: 3/30/2015 8:57:48 PM
 *  Author: ekt
 */ 

#include "../Config.h"
#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR)

#include "IRremoteInt.h"
#include "IRremote.h"
#include "IREncoding.h"

#include <avr/pgmspace.h>

#if NEW_DECODE

#ifdef RC5
   long decodeRC5(decode_results *results);
#endif
#ifdef RC6
   long decodeRC6(decode_results *results);
#endif

static bool MATCH(int measured, int desired) {return measured >= TICKS_LOW(desired) && measured <= TICKS_HIGH(desired);}
static bool MATCH_MARK(int measured_ticks, int desired_us) {return MATCH(measured_ticks, (desired_us + MARK_EXCESS));}
static bool MATCH_SPACE(int measured_ticks, int desired_us) {return MATCH(measured_ticks, (desired_us - MARK_EXCESS));}

static bool decodePulseModulation(
  decode_results *results,
  const PulseModulationEncoding &encoding)
{
    // Calculate the total number of entries in rawbuf that will be required
    const int requiredLen =
        1 + // first space
        (encoding.headerMark ? 1 : 0) +
        (encoding.headerSpace ? 1 : 0) +
        2*encoding.numBits + 
        (encoding.stopMark ? 1 : 0);

    // Verify the length
    if (results->rawlen < requiredLen) {
        return false;
    }

    int offset = 1; // Skip first space

    if (encoding.headerMark) {
        if (!MATCH_MARK(results->rawbuf[offset], encoding.headerMark)) {
          return false;
        }
        offset++;
    }

    if (encoding.headerSpace) {
        if (!MATCH_MARK(results->rawbuf[offset], encoding.headerSpace)) {
          return false;
        }
        offset++;
    }


    uint32_t data = 0;
    for (int i = 0; i < (int)encoding.numBits; i++) {
        if (MATCH_MARK(results->rawbuf[offset], encoding.oneMark) and
          MATCH_SPACE(results->rawbuf[offset+1], encoding.oneSpace)) {
          data = (data << 1) | 1;
        } else if (MATCH_MARK(results->rawbuf[offset], encoding.zeroMark) and
          MATCH_SPACE(results->rawbuf[offset+1], encoding.zeroSpace)) {
          data <<= 1;
        } else {
          return false;
        }
        offset += 2;
    }

    //Stop bit
    if (encoding.stopMark) {
        if (!MATCH_MARK(results->rawbuf[offset], encoding.stopMark)) {
          return false;
        }
    }

    // Success
    results->bits = encoding.numBits;
    results->value = data;
    results->decode_type = encoding.protocol;
    return true;
}


int IRrecv::decode(decode_results *results) {
    results->rawbuf = irparams.rawbuf;
    results->rawlen = irparams.rawlen;
    results->decode_type = -1;
    if (irparams.rcvstate != STATE_STOP) {
        return ERR;
    }


    PulseModulationEncoding encoding;
    for (volatile int i=0; i < NUM_IR_ENCODINGS ; i++) {
		if (!GetIREncoding(i, &encoding)) {
			break;
		}
		if (decodePulseModulation(results, encoding)) {
			return DECODED;
		}
       // if (GetIREncoding(i, &encoding) and ) {
       //     return DECODED;
        //}
    }

#ifdef RC5
    if (decodeRC5(results)) {
        return DECODED;
    }
#endif

#ifdef RC6
    if (decodeRC6(results)) {
        return DECODED;
    }
#endif

    resume();
    return ERR;
}

#endif


#endif
