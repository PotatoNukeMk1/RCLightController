#ifndef MAIN_h
#define MAIN_h

/*********************
 *     INCLUDES
 *********************/

#include <Arduino.h>
#include <Scheduler.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_AW9523.h>
#include <Adafruit_SPIFlash.h>
#include <SimpleCLI.h>
#if defined(USE_SBUS)
#include <sbus.h>
#elif defined(USE_IBUS)
#include <ibus.h>
#endif

/*********************
 *      DEFINES
 *********************/

#if defined(USE_SBUS)
#define xBUS SBUS
#define xBUS_EVENT_FAILSAFE SBUS_EVENT_FAILSAFE
#define xBUS_EVENT_TIMEOUT SBUS_EVENT_TIMEOUT
#define xBUS_EVENT_VALUE_CHANGED SBUS_EVENT_VALUE_CHANGED
#define xBUS_EVENT_CLICKED SBUS_EVENT_CLICKED
#define xBUS_EVENT_PRESSED SBUS_EVENT_PRESSED
#define xBUS_EVENT_LONG_PRESSED SBUS_EVENT_LONG_PRESSED
#define xBUS_EVENT_RELEASED SBUS_EVENT_RELEASED
#define xBUS_CHANNELS_MAX SBUS_CHANNELS_MAX
#define xBUS_CHANNELS_MAX_VALUE SBUS_CHANNELS_MAX_VALUE
#elif defined(USE_IBUS)
#define xBUS IBUS
#define xBUS_EVENT_FAILSAFE IBUS_EVENT_FAILSAFE
#define xBUS_EVENT_TIMEOUT IBUS_EVENT_TIMEOUT
#define xBUS_EVENT_VALUE_CHANGED IBUS_EVENT_VALUE_CHANGED
#define xBUS_EVENT_CLICKED IBUS_EVENT_CLICKED
#define xBUS_EVENT_PRESSED IBUS_EVENT_PRESSED
#define xBUS_EVENT_LONG_PRESSED IBUS_EVENT_LONG_PRESSED
#define xBUS_EVENT_RELEASED IBUS_EVENT_RELEASED
#define xBUS_CHANNELS_MAX IBUS_CHANNELS_MAX
#define xBUS_CHANNELS_MAX_VALUE IBUS_CHANNELS_MAX_VALUE
#endif

#define VERSION_MAJOR    0
#define VERSION_MINOR    1
#define VERSION_REVISION 0

#define THROTTLE_TYPE_FWDBRKREV 0x00
#define THROTTLE_TYPE_FWDBRK    0x01
#define THROTTLE_TYPE_FWDREV    0x02

#define LIGHT_TYPE_NONE           0x00
#define LIGHT_TYPE_PARKING        0x01
#define LIGHT_TYPE_FOG            0x02
#define LIGHT_TYPE_LOWBEAM        0x03
#define LIGHT_TYPE_HIGHBEAM       0x04
#define LIGHT_TYPE_FRONTEXTRA     0x05
#define LIGHT_TYPE_RIGHTINDICATOR 0x06
#define LIGHT_TYPE_LEFTINDICATOR  0x07
#define LIGHT_TYPE_BRAKE          0x08
#define LIGHT_TYPE_REVERSE        0x09
#define LIGHT_TYPE_REAREXTRA      0x0A

#define INDICATOR_STATE_OFF     0x00
#define INDICATOR_STATE_LEFT    0x01
#define INDICATOR_STATE_RIGHT   0x02
#define INDICATOR_STATE_HAZZARD 0x03

#define DIRECTION_STATE_OFF     0x00
#define DIRECTION_STATE_FORWARD 0x01
#define DIRECTION_STATE_BRAKE   0x02
#define DIRECTION_STATE_REVERSE 0x03

/**********************
 *      TYPEDEFS
 **********************/

typedef struct {
  uint8_t channel;
  int16_t neutral;
  uint16_t deadzone;
  bool reverse;
  bool manual;
  bool autoOff;
} lc_steering_conf_t;

typedef struct {
  uint8_t channel;
  int16_t neutral;
  uint16_t deadzone;
  uint8_t type;
  bool reverse;
} lc_throttle_conf_t;

typedef struct {
  bool enabled;
  uint8_t channel;
  int16_t neutral;
  uint16_t deadzone;
  bool reverse;
} lc_switch_conf_t;

typedef struct {
  bool enabled;
  bool combined;
  uint8_t type;
  uint8_t min;
  uint8_t max;
  uint8_t stage[5];
} lc_led_conf_t;

struct lc_config_t {
  uint16_t blinkInterval;
  uint8_t maxStage;
  lc_steering_conf_t steering;
  lc_throttle_conf_t throttle;
  lc_switch_conf_t switchF[3];
  lc_led_conf_t led[16];
};

/**********************
 * GLOBAL PROTOTYPES
 **********************/

void loop2(void);
void errorHandler(void);
void cmd_create(void);
void cmd_errorCallback(cmd_error* e);
void cmd_helpCallback(cmd* c);
void cmd_resetSettingsCallback(cmd* c);
void cmd_saveSettingsCallback(cmd* c);
void cmd_setBlinkIntervalCallback(cmd* c);
void cmd_setMaxStagesCallback(cmd* c);
void cmd_setSteeringCallback(cmd* c);
void cmd_setThrottleCallback(cmd* c);
void cmd_setSwitchFCallback(cmd* c);
void cmd_setLEDCallback(cmd* c);
bool cmd_waitForUserInputYN(void);
int16_t cmd_waitForUserInput(void);
void xBUS_create(void);
void xBUS_eventCallback(int eventCode, int eventParam);

/**********************
 *     VARIABLES
 **********************/

// Config
lc_config_t config;

// Light Controller
bool turnSignalState = false;
uint8_t indicatorState = INDICATOR_STATE_OFF;
uint8_t directionState = DIRECTION_STATE_OFF;
uint8_t directionStage = DIRECTION_STATE_OFF;
bool flashLightState = false;
bool highBeamState = false;
bool fogLightState = false;
bool frontExtraState = false;
bool rearExtraState = false;
uint8_t currentStage = 0;

// Configuration
bool dtrState = false;
bool resetConfigRequest = false;
bool saveConfigRequest = false;

Adafruit_NeoPixel pixel(1, PIN_NEOPIXEL);
Adafruit_FlashTransport_SPI flashTransport(EXTERNAL_FLASH_USE_CS, EXTERNAL_FLASH_USE_SPI);
Adafruit_SPIFlash flash(&flashTransport);
FatVolume fatfs;
File32 file;
Adafruit_AW9523 aw;
elapsedMillis timeElapsed;
SimpleCLI cli;
Command cmd_resetSettings;
Command cmd_saveSettings;
Command cmd_help;
Command cmd_setBlinkInterval;
Command cmd_setMaxStages;
Command cmd_setSteering;
Command cmd_setThrottle;
Command cmd_setSwitchF;
Command cmd_setLED;
Command cmd_setAllLEDs;

#endif