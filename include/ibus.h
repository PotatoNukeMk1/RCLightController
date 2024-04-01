#ifndef IBUS_h
#define IBUS_h

#include <Arduino.h>
#include <elapsedMillis.h>
#include <EventManager.h>

#define IBUS_PACKAGE_HEADER 0x20
#define IBUS_PACKAGE_CMD    0x40
#define IBUS_PACKAGE_SIZE   IBUS_PACKAGE_HEADER - 1 // 32 - startByte
#define IBUS_PACKAGE_TIMEOUT 100
#define IBUS_CHANNELS_MAX   14
#define IBUS_PACKAGE_OFFSET 1500
#define IBUS_CHANNELS_MAX_VALUE 1000

#define IBUS_EVENT_PACKAGE_RECEIVED 0x00
#define IBUS_EVENT_LOSTFRAME     0x01
#define IBUS_EVENT_FAILSAFE      0x02
#define IBUS_EVENT_TIMEOUT       0x03
#define IBUS_EVENT_VALUE_CHANGED 0x04
#define IBUS_EVENT_CLICKED       0x05
#define IBUS_EVENT_PRESSED       0x06
#define IBUS_EVENT_LONG_CLICKED  0x07
#define IBUS_EVENT_LONG_PRESSED  0x08
#define IBUS_EVENT_RELEASED      0x09

class IBUSClass
{
public:
    IBUSClass(void);
    ~IBUSClass(void);
    void begin();
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
        return em->addListener(IBUS_EVENT_PACKAGE_RECEIVED, listener);
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
    Stream *IBUSStream;
    uint32_t _lastPackage;
    int16_t _channel[IBUS_CHANNELS_MAX];
    int16_t _oldChannel[IBUS_CHANNELS_MAX];
    int16_t _lastChannel[IBUS_CHANNELS_MAX];
    int16_t _neutral[IBUS_CHANNELS_MAX];
    int16_t _deadzone[IBUS_CHANNELS_MAX];
    elapsedMillis timeElapsed[IBUS_CHANNELS_MAX];
    bool _reverse[IBUS_CHANNELS_MAX];
    bool _enabled[IBUS_CHANNELS_MAX];
    bool _digital[IBUS_CHANNELS_MAX];
    bool _longPressed[IBUS_CHANNELS_MAX];
    bool _lostframe;
    bool _oldLostframe;
    bool _failsafe;
    bool _oldFailsafe;
    bool _timeout;
    bool _oldTimeout;
};

extern IBUSClass IBUS;

#endif
