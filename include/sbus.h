#ifndef SBUS_h
#define SBUS_h

#include <Arduino.h>
#include <elapsedMillis.h>
#include <EventManager.h>

#define SBUS_PACKAGE_HEADER  0x0F
#define SBUS_PACKAGE_FOOTER  0x00
#define SBUS_PACKAGE_SIZE    24 // 25 - startByte
#define SBUS_PACKAGE_TIMEOUT 100
#define SBUS_CHANNELS_MAX    18
#define SBUS_CHANNELS_OFFSET 992
#define SBUS_CHANNELS_MAX_VALUE 800

#define SBUS_EVENT_PACKAGE_RECEIVED 0x00
#define SBUS_EVENT_LOSTFRAME     0x01
#define SBUS_EVENT_FAILSAFE      0x02
#define SBUS_EVENT_TIMEOUT       0x03
#define SBUS_EVENT_VALUE_CHANGED 0x04
#define SBUS_EVENT_CLICKED       0x05
#define SBUS_EVENT_PRESSED       0x06
#define SBUS_EVENT_LONG_CLICKED  0x07
#define SBUS_EVENT_LONG_PRESSED  0x08
#define SBUS_EVENT_RELEASED      0x09

class SBUSClass
{
protected:
public:
    SBUSClass();
    ~SBUSClass();
    void begin(void);
    void begin(Stream &s);
    void processInput(void);
    int16_t getChannel(uint8_t channel);
    int16_t getLastChannel(uint8_t channel);
    bool getLostFrame(void);
    bool getFailsafe(void);
    bool getTimeout(void);
    void setNeutral(uint8_t channel, int16_t neutral);
    int16_t getNeutral(uint8_t channel);
    void setDeadzone(uint8_t channel, int16_t deadzone);
    int16_t getDeadzone(uint8_t channel);
    void setReverse(uint8_t channel, bool reverse);
    bool getReverse(uint8_t channel);
    void setEnabled(uint8_t channel, bool enabled);
    bool getEnabled(uint8_t channel);
    void setDigital(uint8_t channel, bool digital);
    bool getDigital(uint8_t channel);
    bool attach(EventManager::EventListener listener) {
        return em->addListener(SBUS_EVENT_PACKAGE_RECEIVED, listener);
    };
    bool attachEvent(int eventCode, EventManager::EventListener listener) {
        return em->addListener(eventCode, listener);
    };
    bool detach(EventManager::EventListener listener) {
        return em->removeListener(listener);
    };
private:
    elapsedMillis timeout;
    EventManager *em;
    Stream *SBUSStream;
    uint32_t _lastPackage;
    int16_t _channel[SBUS_CHANNELS_MAX];
    int16_t _oldChannel[SBUS_CHANNELS_MAX];
    int16_t _lastChannel[SBUS_CHANNELS_MAX];
    int16_t _neutral[SBUS_CHANNELS_MAX];
    int16_t _deadzone[SBUS_CHANNELS_MAX];
    elapsedMillis timeElapsed[SBUS_CHANNELS_MAX];
    bool _reverse[SBUS_CHANNELS_MAX];
    bool _enabled[SBUS_CHANNELS_MAX];
    bool _digital[SBUS_CHANNELS_MAX];
    bool _longPressed[SBUS_CHANNELS_MAX];
    bool _lostframe;
    bool _oldLostframe;
    bool _failsafe;
    bool _oldFailsafe;
    bool _timeout;
    bool _oldTimeout;
};

extern SBUSClass SBUS;

#endif