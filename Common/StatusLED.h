#ifndef STATUS_LED_H
#define STATUS_LED_H

#include <inttypes.h>

void InitStatusLED();
void InitStatusLED(uint8_t pin);
void SetStatusLED(bool on);

#endif
