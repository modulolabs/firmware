/*
 * Fuses.c
 *
 * Created: 2/6/2015 3:08:19 PM
 *  Author: ekt
 */ 
 
 #include <avr/io.h>

// Store the fuse bits in a separate section of the elf file.
// Note that fuse bits are inverted (0 enables the feature) so we must bitwise
// and the masks together.
FUSES =
{
	.low = FUSE_SUT_CKSEL4 & FUSE_SUT_CKSEL3 & FUSE_SUT_CKSEL2 & FUSE_SUT_CKSEL0,
	.high = FUSE_SPIEN & FUSE_EESAVE,
	.extended = FUSE_SELFPRGEN & FUSE_BODACT0 & FUSE_BODPD0
};

