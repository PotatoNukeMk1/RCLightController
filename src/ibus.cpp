#include "ibus.h"

IBUSClass IBUS;

const uint32_t LONG_PRESS_TIME  = 500;

IBUSClass::IBUSClass(void)
{

}

IBUSClass::~IBUSClass(void)
{

}

void IBUSClass::begin()
{
    Serial.begin(115200);
    begin(Serial);
}

void IBUSClass::begin(Stream &s)
{
    IBUSStream = &s;
    while(IBUSStream->available())
        IBUSStream->read();

    timeout = 0;
}

void IBUSClass::processInput(void)
{
    if(timeout >= IBUS_PACKAGE_TIMEOUT) {
        _timeout = true;
    } else  {
        _timeout = false;
    }
    if(_timeout != _oldTimeout) {
        _oldTimeout = _timeout;
        em->queueEvent(IBUS_EVENT_TIMEOUT, (int)_timeout);
    }
    // Check for available data
    if(IBUSStream->available() > 0) {
        // Check for ibus header
        uint8_t startByte = IBUSStream->read();
        if(startByte == IBUS_PACKAGE_HEADER) {
            uint8_t buf[IBUS_PACKAGE_SIZE];
            uint16_t len = IBUSStream->readBytes(buf, IBUS_PACKAGE_SIZE);
            if(buf[0] == IBUS_PACKAGE_CMD && len == IBUS_PACKAGE_SIZE) {
                uint16_t checksum = 0xffff - startByte;
                checksum -= buf[0];
                for(int i=1; i<IBUS_CHANNELS_MAX * 2 + 1; i+=2) {
                    _channel[i / 2] = static_cast<int16_t>(buf[i] | ((buf[i+1] << 8) & 0x0FFF)) - IBUS_PACKAGE_OFFSET;
                    checksum -= buf[i];
                    checksum -= buf[i+1];
                }
                if(checksum == (buf[29] | (buf[30] << 8))) {
                    em->queueEvent(IBUS_EVENT_PACKAGE_RECEIVED, 0);
                    for(uint8_t i=0; i<IBUS_CHANNELS_MAX; i++) {
                        // set neutral point
                        if(_neutral[i] != 0) _channel[i] += _neutral[i];
                        // set reverse
                        if(_reverse[i]) _channel[i] *= -1;
                        // set deadzone
                        if(abs(_channel[i]) <= _deadzone[i]) _channel[i] = 0;
                        // only continue if enabled
                        if(!_enabled[i]) continue;
                        if(_digital[i]) {
                            if(_channel[i] != 0) _lastChannel[i] = _channel[i];
                            // digital
                            if(_channel[i] != _oldChannel[i]) {
                                _oldChannel[i] = _channel[i];
                                if(_channel[i] != _neutral[i]) {
                                    em->queueEvent(IBUS_EVENT_PRESSED, i);
                                    timeElapsed[i] = 0;
                                } else {
                                    em->queueEvent(IBUS_EVENT_RELEASED, i);
                                    if(timeElapsed[i] <= LONG_PRESS_TIME) {
                                        em->queueEvent(IBUS_EVENT_CLICKED, i);
                                    }
                                    if(timeElapsed[i] >= LONG_PRESS_TIME) {
                                        _longPressed[i] = false;
                                        em->queueEvent(IBUS_EVENT_LONG_CLICKED, i);
                                    }
                                }
                            }
                            if(!_longPressed[i] && _channel[i] != _neutral[i] && timeElapsed[i] >= LONG_PRESS_TIME) {
                                _longPressed[i] = true;
                                em->queueEvent(IBUS_EVENT_LONG_PRESSED, i);
                            }
                        } else {
                            // analog
                            if(_channel[i] != _oldChannel[i]) {
                                _oldChannel[i] = _channel[i];
                                em->queueEvent(IBUS_EVENT_VALUE_CHANGED, i);
                            }
                        }
                    }
                    timeout = 0;
                }
            }
        }
    }
}

int16_t IBUSClass::getChannel(uint8_t channel)
{
    if(channel >= (IBUS_CHANNELS_MAX)) return 0;
    return _channel[channel];
}

int16_t IBUSClass::getLastChannel(uint8_t channel)
{
    if(channel >= (IBUS_CHANNELS_MAX)) return 0;
    return _lastChannel[channel];
}

bool IBUSClass::getLostFrame()
{
    return _lostframe;
}

bool IBUSClass::getFailsafe()
{
    return _failsafe;
}

bool IBUSClass::getTimeout(void)
{
    return _timeout;
}

void IBUSClass::setNeutral(uint8_t channel, int16_t neutral)
{
    if(channel >= IBUS_CHANNELS_MAX) return;
    if(abs(neutral) <= IBUS_CHANNELS_MAX_VALUE) {
        _neutral[channel] = neutral;
    }
}

int16_t IBUSClass::getNeutral(uint8_t channel)
{
    if(channel >= IBUS_CHANNELS_MAX) return 0;
    return _neutral[channel];
}

void IBUSClass::setDeadzone(uint8_t channel, int16_t deadzone)
{
    if(channel >= IBUS_CHANNELS_MAX) return;
    if(abs(deadzone) < IBUS_CHANNELS_MAX_VALUE) {
        _deadzone[channel] = deadzone;
    }
}

int16_t IBUSClass::getDeadzone(uint8_t channel)
{
    if(channel >= IBUS_CHANNELS_MAX) return 0;
    return _deadzone[channel];
}

void IBUSClass::setReverse(uint8_t channel, bool reverse)
{
    if(channel >= IBUS_CHANNELS_MAX) return;
    _reverse[channel] = reverse;
}

bool IBUSClass::getReverse(uint8_t channel)
{
    if(channel >= IBUS_CHANNELS_MAX) return 0;
    return _reverse[channel];
}

void IBUSClass::setEnabled(uint8_t channel, bool enabled)
{
    if(channel >= IBUS_CHANNELS_MAX) return;
    _enabled[channel] = enabled;
}

bool IBUSClass::getEnabled(uint8_t channel)
{
    if(channel >= IBUS_CHANNELS_MAX) return 0;
    return _enabled[channel];
}

void IBUSClass::setDigital(uint8_t channel, bool digital)
{
    if(channel >= IBUS_CHANNELS_MAX) return;
    _digital[channel] = digital;
}

bool IBUSClass::getDigital(uint8_t channel)
{
    if(channel >= IBUS_CHANNELS_MAX) return 0;
    return _digital[channel];
}
