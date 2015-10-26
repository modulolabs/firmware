/*
 * Fuses.c
 *
 * Created: 2/6/2015 3:08:19 PM
 *  Author: ekt
 */ 
 
 #include <avr/io.h>

 FUSES =
 {
     0xE2, // .low
     0xDF, // .high
     0xFF, // .extended
 };