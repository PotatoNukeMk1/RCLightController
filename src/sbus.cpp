#include "sbus.h"

SBUSClass SBUS;

const uint32_t LONG_PRESS_TIME  = 500;

SBUSClass::SBUSClass()
{
    em = new EventManager();
}

SBUSClass::~SBUSClass()
{
    delete(em);
}

void SBUSClass::begin(void)
{
    Serial.begin(100000, SERIAL_8E2);
    begin(Serial);
}

void SBUSClass::begin(Stream &s)
{
    SBUSStream = &s;
    while(SBUSStream->available())
        SBUSStream->read();

    _oldChannel[16] = -800;
    _oldChannel[17] = -800;
    timeout = 0;
}

void SBUSClass::processInput(void)
{
    if(timeout >= SBUS_PACKAGE_TIMEOUT) {
        _timeout = true;
    } else  {
        _timeout = false;
    }
    if(_timeout != _oldTimeout) {
        _oldTimeout = _timeout;
        em->queueEvent(SBUS_EVENT_TIMEOUT, (int)_timeout);
    }
    // Check for available data
    uint16_t bytesAvailable = SBUSStream->available();
    if(bytesAvailable > 0) {
        // Check for sbus header
        uint8_t startByte = SBUSStream->read();
        if(startByte == SBUS_PACKAGE_HEADER) {
            uint8_t buf[SBUS_PACKAGE_SIZE];
            uint16_t len = SBUSStream->readBytes(buf, SBUS_PACKAGE_SIZE);
            // Check sbus footer and length of received data
            if(buf[SBUS_PACKAGE_SIZE - 1] == SBUS_PACKAGE_FOOTER && len == SBUS_PACKAGE_SIZE) {
                // processing channel data and set offset
                _channel[0] = static_cast<int16_t>(buf[0] | ((buf[1] << 8) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[1] = static_cast<int16_t>((buf[1] >> 3) | ((buf[2] << 5) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[2] = static_cast<int16_t>((buf[2] >> 6) | (buf[3] << 2) | ((buf[4] << 10) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[3] = static_cast<int16_t>((buf[4] >> 1) | ((buf[5] << 7) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[4] = static_cast<int16_t>((buf[5] >> 4) | ((buf[6] << 4) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[5] = static_cast<int16_t>((buf[6] >> 7) | (buf[7] << 1) | ((buf[8] << 9) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[6] = static_cast<int16_t>((buf[8] >> 2) | ((buf[9] << 6) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[7] = static_cast<int16_t>((buf[9] >> 5) | ((buf[10] << 3) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[8] = static_cast<int16_t>(buf[11] | ((buf[12] << 8) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[9] = static_cast<int16_t>((buf[12] >> 3) | ((buf[13] << 5) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[10] = static_cast<int16_t>((buf[13] >> 6) | (buf[14] << 2) | ((buf[15] << 10) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[11] = static_cast<int16_t>((buf[15] >> 1) | ((buf[16] << 7) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[12] = static_cast<int16_t>((buf[16] >> 4) | ((buf[17] << 4) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[13] = static_cast<int16_t>((buf[17] >> 7) | (buf[18] << 1) | ((buf[19] << 9) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[14] = static_cast<int16_t>((buf[19] >> 2) | ((buf[20] << 6) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                _channel[15] = static_cast<int16_t>((buf[20] >> 5) | ((buf[21] << 3) & 0x07FF)) - SBUS_CHANNELS_OFFSET;
                // last two channels are one bit
                _channel[16] = (buf[22] & 0x01)?-800:800;
                _channel[17] = ((buf[22] & 0x02) >> 1)?-800:800;
                // lostframe and failsafe is same for my receiver
                _lostframe = (buf[22] & 0x04) >> 2;
                _failsafe = (buf[22] & 0x08) >> 3;
                em->queueEvent(SBUS_EVENT_PACKAGE_RECEIVED, 0);
                for(uint8_t i=0; i<SBUS_CHANNELS_MAX; i++) {
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
                            if(_channel[i] != _neutral[i]) {
                                em->queueEvent(SBUS_EVENT_PRESSED, i);
                                timeElapsed[i] = 0;
                            } else {
                                em->queueEvent(SBUS_EVENT_RELEASED, i);
                                if(timeElapsed[i] <= LONG_PRESS_TIME) {
                                    em->queueEvent(SBUS_EVENT_CLICKED, i);
                                }
                                if(timeElapsed[i] >= LONG_PRESS_TIME) {
                                    _longPressed[i] = false;
                                    em->queueEvent(SBUS_EVENT_LONG_CLICKED, i);
                                }
                            }
                            _oldChannel[i] = _channel[i];
                        }
                        if(!_longPressed[i] && _channel[i] != _neutral[i] && timeElapsed[i] >= LONG_PRESS_TIME) {
                            _longPressed[i] = true;
                            em->queueEvent(SBUS_EVENT_LONG_PRESSED, i);
                        }
                    } else {
                        // analog
                        if(_channel[i] != _oldChannel[i]) {
                            _oldChannel[i] = _channel[i];
                            em->queueEvent(SBUS_EVENT_VALUE_CHANGED, i);
                        }
                    }
                }
                if(_lostframe != _oldLostframe) {
                    _oldLostframe = _lostframe;
                    em->queueEvent(SBUS_EVENT_LOSTFRAME, (int)_lostframe);
                }
                if(_failsafe != _oldFailsafe) {
                    _oldFailsafe = _failsafe;
                    em->queueEvent(SBUS_EVENT_FAILSAFE, (int)_failsafe);
                }
                timeout = 0;
            }
        }
    }
    em->processEvent();
}

int16_t SBUSClass::getChannel(uint8_t channel)
{
    if(channel >= (SBUS_CHANNELS_MAX)) return 0;
    return _channel[channel];
}

int16_t SBUSClass::getLastChannel(uint8_t channel)
{
    if(channel >= (SBUS_CHANNELS_MAX)) return 0;
    return _lastChannel[channel];
}

bool SBUSClass::getLostFrame()
{
    return _lostframe;
}

bool SBUSClass::getFailsafe()
{
    return _failsafe;
}

bool SBUSClass::getTimeout(void)
{
    return _timeout;
}

void SBUSClass::setNeutral(uint8_t channel, int16_t neutral)
{
    if(channel >= SBUS_CHANNELS_MAX) return;
    if(abs(neutral) <= SBUS_CHANNELS_MAX_VALUE) {
        _neutral[channel] = neutral;
    }
}

int16_t SBUSClass::getNeutral(uint8_t channel)
{
    if(channel >= SBUS_CHANNELS_MAX) return 0;
    return _neutral[channel];
}

void SBUSClass::setDeadzone(uint8_t channel, int16_t deadzone)
{
    if(channel >= SBUS_CHANNELS_MAX) return;
    if(abs(deadzone) < SBUS_CHANNELS_MAX_VALUE) {
        _deadzone[channel] = deadzone;
    }
}

int16_t SBUSClass::getDeadzone(uint8_t channel)
{
    if(channel >= SBUS_CHANNELS_MAX) return 0;
    return _deadzone[channel];
}

void SBUSClass::setReverse(uint8_t channel, bool reverse)
{
    if(channel >= SBUS_CHANNELS_MAX) return;
    _reverse[channel] = reverse;
}

bool SBUSClass::getReverse(uint8_t channel)
{
    if(channel >= SBUS_CHANNELS_MAX) return 0;
    return _reverse[channel];
}

void SBUSClass::setEnabled(uint8_t channel, bool enabled)
{
    if(channel >= SBUS_CHANNELS_MAX) return;
    _enabled[channel] = enabled;
}

bool SBUSClass::getEnabled(uint8_t channel)
{
    if(channel >= SBUS_CHANNELS_MAX) return 0;
    return _enabled[channel];
}

void SBUSClass::setDigital(uint8_t channel, bool digital)
{
    if(channel >= SBUS_CHANNELS_MAX) return;
    _digital[channel] = digital;
}

bool SBUSClass::getDigital(uint8_t channel)
{
    if(channel >= SBUS_CHANNELS_MAX) return 0;
    return _digital[channel];
}