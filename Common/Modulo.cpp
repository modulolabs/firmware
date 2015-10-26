/*
 * Modulo.cpp
 *
 * Created: 7/9/2014 10:03:45 AM
 *  Author: ekt
 */


#include "Modulo.h"
#include "TwoWire.h"
#include "DeviceID.h"
#include <string.h>


static ModuloStatus _status;
static volatile uint8_t *_statusDDR = NULL;
static volatile uint8_t *_statusPort = NULL;
static volatile uint8_t _statusMask = 0;
static uint8_t _statusCounter = 0;
static uint16_t _statusBreathe = 0;
const uint8_t moduloBroadcastAddress = 9;

#define BroadcastCommandGlobalReset 0
#define BroadcastCommandGetNextDeviceID 1
#define BroadcastCommandSetAddress 2
#define BroadcastCommandGetAddress 3
#define BroadcastCommandGetDeviceType 4
#define BroadcastCommandGetDeviceVersion 5
#define BroadcastCommandGetCompanyName 6
#define BroadcastCommandGetProductName 7
#define BroadcastCommandGetDocURL 8
#define BroadcastCommandGetDocURLContinued 9
#define BroadcastCommandGetEvent 10
#define BroadcastCommandSetStatusLED 11
#define BroadcastCommandClearEvent 12

void ModuloInit(
	volatile uint8_t *statusDDR,
	volatile uint8_t *statusPort,
	uint8_t statusMask,
	bool useTwoWireInterrupt)
{
	_statusDDR = statusDDR;
	_statusPort = statusPort;
	_statusMask = statusMask;
	
	if (_statusDDR and _statusMask) {
		*_statusDDR |= _statusMask;
	}
	
	TwoWireInit(useTwoWireInterrupt);
	TwoWireSetDeviceAddress(0);

	// Ensure that we have a valid device id
	GetDeviceID();
    ModuloReset();
	ModuloSetStatus(ModuloStatusBlinking);
}


void ModuloSetStatus(ModuloStatus status) {
	_status = status;
}


void ModuloUpdateStatusLED() {
	if (!_statusMask or !_statusPort or !_statusDDR) {
		return;
	}
	switch(_status) {
		case ModuloStatusOff:
			*_statusPort &= ~_statusMask;
			break;
		case ModuloStatusOn:
			*_statusPort |= _statusMask;
			break;
		case  ModuloStatusBlinking:
			_statusCounter = (_statusCounter+1) % 100;
			if (_statusCounter == 0) {
				*_statusPort ^= _statusMask;
			}
			break;
#if 0
		case ModuloStatusBreathing:
			_statusCounter = (_statusCounter+1) % 10;
			if (_statusCounter == 0) {
				_statusBreathe += .15;
				if (_statusBreathe >= 20) {
					_statusBreathe = 0;
				}
			}
	
			uint8_t threshold = _statusBreathe <= 10 ? _statusBreathe : 20-_statusBreathe;
			if (_statusCounter < threshold) {
				*_statusPort |= _statusMask;
			} else {
				*_statusPort &= ~_statusMask;
			}
			break;
#endif
	}

}

static bool _shouldReplyToBroadcastRead = false;

static uint8_t _eventCode = 0;
static uint16_t _eventData = 0;

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
                    ModuloReset();
                }
                break;


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
            case BroadcastCommandGetCompanyName:
            case BroadcastCommandGetProductName:
            case BroadcastCommandGetDocURL:
            case BroadcastCommandGetDocURLContinued:
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
                readBuffer->AppendString(ModuloDeviceType);
                return true;
            case BroadcastCommandGetDeviceVersion:
                readBuffer->AppendValue<uint16_t>(ModuloDeviceVersion);
                return true;
            case BroadcastCommandGetCompanyName:
                readBuffer->AppendString(ModuloCompanyName);
                return true;
            case BroadcastCommandGetProductName:
                readBuffer->AppendString(ModuloProductName);
                return true;
            case BroadcastCommandGetDocURL:
                readBuffer->AppendString(ModuloDocURL);
                return true;
            case BroadcastCommandGetDocURLContinued:
                readBuffer->AppendString(ModuloDocURL);
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


