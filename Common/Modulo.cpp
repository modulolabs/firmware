/*
 * Modulo.cpp
 *
 * Created: 7/9/2014 10:03:45 AM
 *  Author: ekt
 */


#include "Modulo.h"
#include "TwoWire.h"
#include "DeviceID.h"
#include "Clock.h"
#include "StatusLED.h"
#include <string.h>

#define BroadcastCommandGlobalReset 0
#define BroadcastCommandGetNextDeviceID 1
#define BroadcastCommandGetNextUnassignedDeviceID 2
#define BroadcastCommandSetAddress 3
#define BroadcastCommandGetAddress 4
#define BroadcastCommandGetDeviceType 5
#define BroadcastCommandGetDeviceVersion 6
#define BroadcastCommandGetEvent 7
#define BroadcastCommandClearEvent 8
#define BroadcastCommandSetStatusLED 9
#define BroadcastCommandSetDeviceID 10

const uint8_t moduloBroadcastAddress = 9;

static bool _shouldReplyToBroadcastRead = false;

static uint8_t _eventCode = 0;
static uint16_t _eventData = 0;
static ModuloStatus _status = ModuloStatusOff;

void ModuloInit(bool useTwoWireInterrupt)
{
	TwoWireInit(moduloBroadcastAddress, useTwoWireInterrupt);
	TwoWireSetDeviceAddress(0);

	// Ensure that we have a valid device id
	GetDeviceID();
    ModuloReset();
	ModuloSetStatus(ModuloStatusOff);
}


void ModuloSetStatus(ModuloStatus status) {
	_status = status;
}

uint32_t millis();

void ModuloUpdateStatusLED() {
	switch(_status) {
		case ModuloStatusOff:
			SetStatusLED(false);
			break;
		case ModuloStatusOn:
			SetStatusLED(true);
			break;
		case  ModuloStatusBlinking:
			SetStatusLED((millis()/500) % 2);
			break;
	}
}

static bool _ModuloWrite(uint8_t address, uint8_t *data, uint8_t len) {
	ModuloWriteBuffer buffer(address, data, len);

	if (!buffer.IsValid()) {
		return 0;
	}

    _shouldReplyToBroadcastRead = false;

    if (buffer.GetAddress() == moduloBroadcastAddress) {

        switch(buffer.GetCommand()) {
            case BroadcastCommandGlobalReset:
                if (buffer.GetSize() == 0) {
                    TwoWireSetDeviceAddress(0);
					ModuloSetStatus(ModuloStatusOff);	
                    ModuloReset();
                }
                break;

            case BroadcastCommandGetNextUnassignedDeviceID:
				if (TwoWireGetDeviceAddress() != 0) {
					return false;
				}
				// Fall through to GetNextDeviceID
            case BroadcastCommandGetNextDeviceID:
                if (buffer.GetSize() == 2 and
                    (buffer.Get<uint16_t>(0) <= GetDeviceID())) {
                    _shouldReplyToBroadcastRead = true;
                    return true;
                }
                break;

            case BroadcastCommandSetAddress:
                if (buffer.GetSize() == 3 and
                    buffer.Get<uint16_t>(0) == GetDeviceID()) {
                    TwoWireSetDeviceAddress(buffer.Get<uint8_t>(2));
                    return true;
                }
                break;

            // These commands all expect the device ID
            case BroadcastCommandGetAddress:
            case BroadcastCommandGetDeviceType:
            case BroadcastCommandGetDeviceVersion:
                if (buffer.GetSize() == 2 and buffer.Get<uint16_t>(0) == GetDeviceID()) {
					_shouldReplyToBroadcastRead = true;
                    return true;
                }
                break;
            case BroadcastCommandSetStatusLED:
                if (buffer.GetSize() == 3 and buffer.Get<uint16_t>(0) == GetDeviceID()) {
                    ModuloSetStatus((ModuloStatus)buffer.Get<uint8_t>(2));
                    return true;
                }
                break;
            case BroadcastCommandGetEvent:
				if (ModuloGetEvent(&_eventCode, &_eventData)) {
					_shouldReplyToBroadcastRead = true;
					return true;
				}
                break;
			case BroadcastCommandClearEvent:
                if (buffer.GetSize() == 5 and buffer.Get<uint16_t>(1) == GetDeviceID()) {
					ModuloClearEvent(buffer.Get<uint8_t>(0), buffer.Get<uint16_t>(3));
                    return true;
                }
                break;
			case BroadcastCommandSetDeviceID:
				if (buffer.GetSize() == 4 and buffer.Get<uint16_t>(0) == GetDeviceID()) {
					SetDeviceID(buffer.Get<uint16_t>(2));
					return true;
				}
				break;
        }
        return false;
    }

    return ModuloWrite(buffer);
}

static bool _ModuloRead(uint8_t address, uint8_t command, ModuloReadBuffer *readBuffer) {
    if (address== moduloBroadcastAddress) {
        if (!_shouldReplyToBroadcastRead) {
            return false;
        }
        switch(command) {
            case BroadcastCommandGetNextDeviceID:
			case BroadcastCommandGetNextUnassignedDeviceID:			
                {
                    // We have to return the deviceID in big endian order, so that if a collision occurs
                    // the smallest value will win.
                    uint16_t deviceID = GetDeviceID();
                    deviceID = (deviceID << 8) | (deviceID >> 8);
                    readBuffer->AppendValue<uint16_t>(deviceID);
                    return true;
                }
            case BroadcastCommandGetAddress:
                {
                    uint8_t addr = TwoWireGetDeviceAddress();
                    readBuffer->AppendValue<uint8_t>(addr);
                    return true;
                }
            case BroadcastCommandGetDeviceType:
				for (int i=0; i < MODULO_TYPE_SIZE; i++) {
					char c = GetModuloType(i);
					readBuffer->AppendValue<uint8_t>(c);
					if (c == 0) {
						break;
					}
				}
                return true;
            case BroadcastCommandGetDeviceVersion:
                readBuffer->AppendValue<uint16_t>(GetModuloVersion());
                return true;
            case BroadcastCommandGetEvent:
				readBuffer->AppendValue<uint8_t>(_eventCode);
				readBuffer->AppendValue<uint16_t>(GetDeviceID());
				readBuffer->AppendValue<uint16_t>(_eventData);
                return true;
        }
        return false;
    }


    return ModuloRead(command, readBuffer);
}

int TwoWireCallback(uint8_t address, uint8_t *buffer, uint8_t len, uint8_t maxLen) {
	uint8_t command = buffer[0];
	_ModuloWrite(address, buffer, len);

	// Now generate data to send. Note that this overwrites the received data, so
	// don't try to access moduloWriteBuffer past this point.
	ModuloReadBuffer moduloReadBuffer(address, buffer, maxLen);
	if (!_ModuloRead(address, command, &moduloReadBuffer)) {
		// Nothing to send back.
		return 0;
	}

	moduloReadBuffer.AppendValue(moduloReadBuffer.ComputeCRC(address));
	return moduloReadBuffer.GetLength();
}


