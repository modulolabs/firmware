/*
 * MotorCurrentLimit.cpp
 *
 * Created: 11/10/2015 10:09:09 AM
 *  Author: ekt
 */ 

#include "Config.h"
#include "MotorCurrentLimit.h"

#include <avr/io.h>
#include <inttypes.h>
#include <util/delay.h>

static void _incr(int count);
static void _decr(int count);

#define UD_PORT PORTA
#define UD_PIN 0
#define UD_DDR DDRA
#define CS_PORT PORTA
#define CS_PIN 3
#define CS_DDR DDRA

static uint8_t _currentLimit = 0;

void MotorCurrentLimitInit() {
	CS_PORT |= _BV(CS_PIN);
	UD_PORT |= _BV(CS_PIN);
	CS_DDR |= _BV(CS_PIN);
	UD_DDR |= _BV(UD_PIN);
	
	_currentLimit = 0;
	MotorCurrentLimitSet(63);
}

static void _incr(int count) {
	UD_PORT |= _BV(UD_PIN);
	_delay_us(1);
		
	CS_PORT &= ~_BV(CS_PIN);
	_delay_us(1);
		
	for (int i=0; i < count and _currentLimit < 63; i++) {
		UD_PORT &= ~_BV(UD_PIN);
		_delay_us(1);
			
		UD_PORT |= _BV(UD_PIN);
		_delay_us(1);
		
		_currentLimit++;
	}
	
	CS_PORT |= _BV(CS_PIN);
	_delay_us(1);	
}

static void _decr(int count) {
	UD_PORT &= ~_BV(UD_PIN);
	_delay_us(1);
	CS_PORT &= ~_BV(CS_PIN);
	_delay_us(1);
		
	for (int i=0; i < count and _currentLimit > 0; i++) {
		UD_PORT |= _BV(UD_PIN);
		_delay_us(1);
		UD_PORT &= ~_BV(UD_PIN);
		_delay_us(1);
		_currentLimit--;
	}
	
	CS_PORT |= _BV(CS_PIN);
	_delay_us(1);	
}

void MotorCurrentLimitSet(int newCurrentLimit) {
	int delta = newCurrentLimit-_currentLimit;
	if (delta > 0) {
		_incr(delta);
	} else if (delta < 0) {
		_decr(-delta);
	}
}