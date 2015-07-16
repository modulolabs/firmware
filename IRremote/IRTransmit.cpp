/*
 * IRTransmit.cpp
 *
 * Created: 3/30/2015 10:19:46 PM
 *  Author: ekt
 */ 
#include "IRTransmit.h"
#include "Clock.h"
#include <avr/io.h>


static void mark(int time) {
  // Sends an IR mark for the specified number of microseconds.
  // The mark output is modulated at the PWM frequency.
  TIMER_ENABLE_PWM; // Enable pin 3 PWM output
  if (time > 0) delayMicroseconds(time);
}

/* Leave pin off for time (given in microseconds) */
static void space(int time) {
  // Sends an IR space for the specified number of microseconds.
  // A space is no output, so the PWM output is disabled.
  TIMER_DISABLE_PWM; // Disable pin 3 PWM output
  if (time > 0) delayMicroseconds(time);
}

static void enableIROut(int khz) {
  // Enables IR output.  The khz value controls the modulation frequency in kilohertz.
  // The IR output will be on pin 3 (OC2B).
  // This routine is designed for 36-40KHz; if you use it for other values, it's up to you
  // to make sure it gives reasonable results.  (Watch out for overflow / underflow / rounding.)
  // TIMER2 is used in phase-correct PWM mode, with OCR2A controlling the frequency and OCR2B
  // controlling the duty cycle.
  // There is no prescaling, so the output frequency is 16MHz / (2 * OCR2A)
  // To turn the output on and off, we leave the PWM running, but connect and disconnect the output pin.
  // A few hours staring at the ATmega documentation and this will all make sense.
  // See my Secrets of Arduino PWM at http://arcfn.com/2009/07/secrets-of-arduino-pwm.html for details.

 
  // Disable the Timer2 Interrupt (which is used for receiving IR)
  TIMER_DISABLE_INTR; //Timer2 Overflow Interrupt
  
  //pinMode(TIMER_PWM_PIN, OUTPUT);
  //digitalWrite(TIMER_PWM_PIN, LOW); // When not sending PWM, we want it low
  DDRA |= _BV(0); // A0
  PORTA &= ~_BV(0);

  // COM2A = 00: disconnect OC2A
  // COM2B = 00: disconnect OC2B; to send signal set to 10: OC2B non-inverted
  // WGM2 = 101: phase-correct PWM with OCRA as top
  // CS2 = 000: no prescaling
  // The top value for the timer.  The modulation frequency will be SYSCLOCK / 2 / OCR2A.
  //TIMER_CONFIG_KHZ(khz);

  uint8_t modeA = 2;
  uint8_t modeB = 2;
  uint8_t wgm = 14;

  TOCPMSA0 &= ~(0b11 << TOCC0S0);
  TOCPMSA0 |= (1 << TOCC0S0);
  TCCR1A =  (modeA << COM1A0) | (modeB << COM1B0) | (wgm & 0b11);
  TCCR1B = (TCCR1B & 0b11100111) | ((wgm >> 2) << WGM12);

  uint16_t topValue = 1024;
  ICR1H = topValue >> 8;
  ICR1L = topValue & 0xFF;

  TOCPMCOE |= _BV(0);
  TCCR1B  |= 1; // Clock select

}

static bool transmitPulseModulation(uint32_t data, const PulseModulationEncoding &encoding)
{
	/*
    enableIROut(pgm_read_word_near(&encoding.khz));
    for (int i=0; i < PULSE_MODULATION_MAX_HEADER_LENGTH; i++) {
        int16_t h = 0; //pgm_read_word_near(&encoding.header[i]);
        if (h) {
            if (i % 2 ) {
                space(h);
            } else {
                mark(h);
            }
        }
    }

    int16_t oneMark = pgm_read_word_near(&encoding.oneMark);
    int16_t oneSpace = pgm_read_word_near(&encoding.oneSpace);
    int16_t zeroMark = pgm_read_word_near(&encoding.zeroMark);
    int16_t zeroSpace = pgm_read_word_near(&encoding.zeroSpace);

    for (int i = 0; i < pgm_read_word_near(&encoding.numBits); i++) {
        if (data & TOPBIT) {
            mark(oneMark);
            space(oneSpace);
        } else {
            mark(zeroMark);
            space(zeroSpace);
        }
        data <<= 1;
    }

    int16_t stopMark = pgm_read_word_near(&encoding.stopMark);
    if (stopMark > 0) {
        mark(stopMark);
        space(0);
    }
	*/
	return false;
}

