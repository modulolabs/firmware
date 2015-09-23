/*
 * IRTransmit.h
 *
 * Created: 3/30/2015 10:19:34 PM
 *  Author: ekt
 */ 


#ifndef IRTRANSMIT_H_
#define IRTRANSMIT_H_


#include "../Config.h"
#if defined(MODULE_TYPE) && (MODULE_TYPE == MODULE_TYPE_IR)

#include "IREncoding.h"

// Transmit the provided data using the specified encoding
static bool transmitPulseModulation(uint32_t data, const PulseModulationEncoding &encoding);

#endif
#endif /* IRTRANSMIT_H_ */