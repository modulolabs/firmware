/*
 * StatusLED.cpp
 *
 * Created: 11/11/2015 3:37:55 PM
 *  Author: ekt
 */ 

#include <asf.h>

#define LED_PIN 15

void SetStatusLED(bool on) {
	port_pin_set_output_level(LED_PIN, on);
}
