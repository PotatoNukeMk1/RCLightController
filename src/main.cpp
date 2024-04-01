#include <main.h>

void setup() {
  Serial.begin(115200);
  Serial.flush();

  // Create cli config menu
  cmd_create();

  Wire.begin();

  // Initialize external flash
  if(!flash.begin()) {
    Serial.println(F("Error: failed to initialize external flash"));
    errorHandler();
  }

  // Initialize file system
  if(!fatfs.begin(&flash)) {
    Serial.println(F("Error: failed to initialize file system"));
    errorHandler();
  }

  // Load config
  if(fatfs.exists("config.bin")) {
    file = fatfs.open("config.bin", O_RDONLY);
    if(!file) {
      Serial.println(F("Error: failed to open config file for read"));
      errorHandler();
    }
    file.read(&config, sizeof(config));
    file.close();
  } else {
    file = fatfs.open("config.bin", O_RDWR | O_CREAT);
    if(!file) {
      Serial.println(F("Error: failed to open config file for write"));
      errorHandler();
    }
    file.write(&config, sizeof(config));
    file.close();
  }

  // Initialize NeoPixel
  pixel.begin();
  pixel.setBrightness(32);
  pixel.clear();
  pixel.show();

#if defined(USE_SBUS)
  Serial1.begin(100000, SERIAL_8E2);
  Serial1.setTimeout(6);
  xBUS.begin(Serial1);
#elif defined(USE_IBUS)
  Serial1.begin(115200);
  Serial1.setTimeout(6);
  IBUS.begin(Serial1);
#endif

  // Configure xBUS
  xBUS_create();

  // Initialize I2C GPIO expander
  if(!aw.begin(0x5B)) {
    Serial.println(F("Error: failed to initialize I2C GPIO expander"));
    errorHandler();
  }
  for(uint8_t j=0; j<16; j++) {
    aw.pinMode(j, AW9523_LED_MODE);
    aw.analogWrite(j, config.led[j].stage[currentStage]);
  }

  // Show green NeoPixel
  pixel.setPixelColor(0, pixel.Color(0, 255, 0));
  pixel.show();

  // Reset lights to default state
  indicatorState = INDICATOR_STATE_OFF;
  if(config.throttle.type == THROTTLE_TYPE_FWDREV) {
    directionState = DIRECTION_STATE_BRAKE;
  }
  directionStage = DIRECTION_STATE_OFF;
  flashLightState = false;
  highBeamState = false;
  fogLightState = false;
  frontExtraState = false;
  rearExtraState = false;
  currentStage = 0;

  // Start loop2
  Scheduler.startLoop(loop2);
}

void loop() {
  if(timeElapsed >= config.blinkInterval) {
    timeElapsed = 0;
    turnSignalState = !turnSignalState;
  }
  if(!Serial.dtr()) {
    if(dtrState) {
      dtrState = false;
      pixel.setPixelColor(0, pixel.Color(0, 255, 0));
      pixel.show();
    }
    for(uint8_t i=0; i<16; i++) {
      if(config.led[i].enabled) {
        // Indicator Left and Right
        if(config.led[i].type == LIGHT_TYPE_LEFTINDICATOR || config.led[i].type == LIGHT_TYPE_RIGHTINDICATOR) {
          if(indicatorState == INDICATOR_STATE_LEFT && config.led[i].type == LIGHT_TYPE_LEFTINDICATOR) {
            // Left indicator active
            aw.analogWrite(i, turnSignalState?config.led[i].max:config.led[i].min);
          } else if(indicatorState == INDICATOR_STATE_RIGHT && config.led[i].type == LIGHT_TYPE_LEFTINDICATOR) {
            // Left indicator inactive
            if(config.led[i].combined && directionState == DIRECTION_STATE_BRAKE) {
              // Left indicator combined with brake
              aw.analogWrite(i, config.led[i].max);
            } else {
              // Left indicator off
              aw.analogWrite(i, config.led[i].stage[currentStage]);
            }
          } else if(indicatorState == INDICATOR_STATE_RIGHT && config.led[i].type == LIGHT_TYPE_RIGHTINDICATOR) {
            // Right indicator active
            aw.analogWrite(i, turnSignalState?config.led[i].max:config.led[i].min);
          } else if(indicatorState == INDICATOR_STATE_LEFT && config.led[i].type == LIGHT_TYPE_RIGHTINDICATOR) {
            // Right indicator inactive
            if(config.led[i].combined && directionState == DIRECTION_STATE_BRAKE) {
              // Right indicator combined with brake
              aw.analogWrite(i, config.led[i].max);
            } else {
              // Right indicator off
              aw.analogWrite(i, config.led[i].stage[currentStage]);
            }
          } else if(indicatorState == INDICATOR_STATE_HAZZARD) {
            // Left and Right indicator active
            aw.analogWrite(i, turnSignalState?config.led[i].max:config.led[i].min);
          } else if(indicatorState == INDICATOR_STATE_OFF) {
            if(config.led[i].combined && directionState == DIRECTION_STATE_BRAKE) {
              // Left and Right indicator combined with brake
              aw.analogWrite(i, config.led[i].max);
            } else {
              // Left and Right indicator off
              aw.analogWrite(i, config.led[i].stage[currentStage]);
            }
          }
        } else {
          // Brake and Reverse
          if(config.led[i].type == LIGHT_TYPE_BRAKE || config.led[i].type == LIGHT_TYPE_REVERSE) {
            if(directionState == DIRECTION_STATE_BRAKE && config.led[i].type == LIGHT_TYPE_BRAKE) {
              // Brake active
              aw.analogWrite(i, config.led[i].max);
            } else if(directionState == DIRECTION_STATE_REVERSE && config.led[i].type == LIGHT_TYPE_REVERSE) {
              // Reverse active
              aw.analogWrite(i, config.led[i].max);
            } else {
              // Brake and Reverse off
              aw.analogWrite(i, config.led[i].stage[currentStage]);
            }
          } else {
            // High beam
            if(highBeamState && config.led[i].type == LIGHT_TYPE_HIGHBEAM) {
              // Highbeam active
              aw.analogWrite(i, config.led[i].max);
            } else if(flashLightState && config.led[i].type == LIGHT_TYPE_HIGHBEAM) {
              // Flashlight (highbeam) active
              aw.analogWrite(i, config.led[i].max);
            } else {
              if(fogLightState && config.led[i].type == LIGHT_TYPE_FOG) {
                // Foglights active
                aw.analogWrite(i, config.led[i].max);
              } else {
                if(frontExtraState && config.led[i].type == LIGHT_TYPE_FRONTEXTRA) {
                  // Front extra active
                  aw.analogWrite(i, config.led[i].max);
                } else if(flashLightState && config.led[i].type == LIGHT_TYPE_FRONTEXTRA && config.led[i].combined) {
                  // Flashlight (frontextra) active
                  aw.analogWrite(i, config.led[i].max);
                } else {
                  if(rearExtraState && config.led[i].type == LIGHT_TYPE_REAREXTRA) {
                    // Rear extra active
                    aw.analogWrite(i, config.led[i].max);
                  } else {
                    // Light off
                    aw.analogWrite(i, config.led[i].stage[currentStage]);
                  }
                }
              }
            }
          }
        }
      }
    }
  } else {
    if(!dtrState) {
      dtrState = true;
      delay(100);
      for(uint8_t i=0; i<16; i++) {
        aw.analogWrite(i, 0);
      }
      pixel.setPixelColor(0, pixel.Color(0, 0, 255));
      pixel.show();
      Serial.print(F("RC Light Controller v"));
      Serial.print(VERSION_MAJOR, DEC);
      Serial.print(F("."));
      Serial.print(VERSION_MINOR, DEC);
      Serial.print(F("."));
      Serial.println(VERSION_REVISION, DEC);
    }
    if(Serial.available()) {
      String input = Serial.readStringUntil('\n');
      cli.parse(input);
    }
  }
  delay(33);
}

void loop2() {
  // SBUS processing incoming data
  xBUS.processInput();
  yield();
}

void errorHandler() {
  // Halt on error
  pixel.setPixelColor(0, pixel.Color(255, 0, 0));
  pixel.show();
  while(true) {
    delay(10);
    yield();
  }
}

void cmd_create(void)
{
  cli.setOnError(cmd_errorCallback);
  // help command
  cmd_help = cli.addCommand("help", cmd_helpCallback);
  // reset command
  cmd_resetSettings = cli.addCommand("reset", cmd_resetSettingsCallback);
  // save command
  cmd_saveSettings = cli.addCommand("save", cmd_saveSettingsCallback);
  // blinkInterval command
  cmd_setBlinkInterval = cli.addCommand("blinkInterval", cmd_setBlinkIntervalCallback);
  cmd_setBlinkInterval.addPositionalArgument("interval");
  cmd_setBlinkInterval.addFlagArgument("l");
  // maxStage command
  cmd_setMaxStages = cli.addCommand("maxStage", cmd_setMaxStagesCallback);
  cmd_setMaxStages.addPositionalArgument("stage");
  cmd_setMaxStages.addFlagArgument("l");
  // steering command
  cmd_setSteering = cli.addCommand("steering", cmd_setSteeringCallback);
  cmd_setSteering.addArgument("c");
  cmd_setSteering.addArgument("n", "0");
  cmd_setSteering.addArgument("d", "100");
  cmd_setSteering.addFlagArgument("r");
  cmd_setSteering.addFlagArgument("m");
  cmd_setSteering.addFlagArgument("a");
  cmd_setSteering.addFlagArgument("l");
  // throttle command
  cmd_setThrottle = cli.addCommand("throttle", cmd_setThrottleCallback);
  cmd_setThrottle.addArgument("c");
  cmd_setThrottle.addArgument("n", "0");
  cmd_setThrottle.addArgument("d", "100");
  cmd_setThrottle.addArgument("t", "0");
  cmd_setThrottle.addFlagArgument("r");
  cmd_setThrottle.addFlagArgument("l");
  // switch command
  cmd_setSwitchF = cli.addCommand("switch", cmd_setSwitchFCallback);
  cmd_setSwitchF.addPositionalArgument("function");
  cmd_setSwitchF.addArgument("c", "0");
  cmd_setSwitchF.addArgument("n", "0");
  cmd_setSwitchF.addArgument("d", "100");
  cmd_setSwitchF.addFlagArgument("r");
  cmd_setSwitchF.addFlagArgument("e");
  cmd_setSwitchF.addFlagArgument("l");
  // led command
  cmd_setLED = cli.addCommand("led", cmd_setLEDCallback);
  cmd_setLED.addPositionalArgument("pin");
  cmd_setLED.addFlagArgument("l");
}

void cmd_errorCallback(cmd_error* e)
{
  CommandError cmdError(e);
  Serial.print(F("Error: "));
  Serial.println(cmdError.toString());
  if(cmdError.hasCommand()) {
    Serial.print(F("Did you mean \""));
    Serial.print(cmdError.getCommand().toString());
    Serial.println(F("\"?"));
  }
}
void cmd_helpCallback(cmd* c)
{
  Serial.print(cli.toString());
  Serial.println(F("Options:"));
  Serial.println(F("\t-interval\tBlink interval for indicator lights. Possible value: 250-2500"));
  Serial.println(F("\t-l\t\tlist settings"));
  Serial.println(F("\t-stage\t\tMax stage for LEDs. Possible value: 1-5"));
  Serial.print(F("\t-c\t\tchannel number (0-"));
  Serial.print(xBUS_CHANNELS_MAX, DEC);
  Serial.println(F(")"));
  Serial.print(F("\t-n\t\tneutral value (0-"));
  Serial.print(xBUS_CHANNELS_MAX_VALUE, DEC);
  Serial.println(F(")"));
  Serial.print(F("\t-d\t\tdeadzone value (0-"));
  Serial.print(xBUS_CHANNELS_MAX_VALUE-1, DEC);
  Serial.println(F(")"));
  Serial.println(F("\t-r\t\tEnable reverse"));
  Serial.println(F("\t-m\t\tEnable manual indicator"));
  Serial.println(F("\t-a\t\tEnable automatic indicator stopping"));
  Serial.println(F("\t-t\t\tThrottle type. Possible values:"));
  Serial.println(F("\t\t\t\t0: Forward/Brake/Reverse (Normal mode)"));
  Serial.println(F("\t\t\t\t1: Forward/Brake (Race mode)"));
  Serial.println(F("\t\t\t\t2: Forward/Reverse (Crawler mode)"));
  Serial.println(F("\t-e\t\tEnable switch/led"));
}

void cmd_resetSettingsCallback(cmd* c)
{
  Command cmd(c);
  Serial.println(F("Reset changed config to previous settings (y/n)? "));
  if(!cmd_waitForUserInputYN()) {
    Serial.println(F("Aborted"));
    return;
  }
  if(!fatfs.exists("config.bin")) {
    Serial.println(F("Error: config file doesn't exist"));
    return;
  }
  file = fatfs.open("config.bin", O_RDONLY);
  if(!file) {
    Serial.println(F("Error: failed to open config file for read"));
    return;
  }
  file.read(&config, sizeof(config));
  file.close();
  Serial.println(F("Ok"));
}

void cmd_saveSettingsCallback(cmd* c)
{
  Command cmd(c);
  Serial.println(F("Save changed config (y/n)? "));
  if(!cmd_waitForUserInputYN()) {
    Serial.println(F("Aborted"));
    return;
  }
  file = fatfs.open("config.bin", O_RDWR | O_CREAT);
  if(!file) {
    Serial.println(F("Error: failed to open config file for write"));
    return;
  }
  file.write(&config, sizeof(config));
  file.close();
  Serial.println(F("Ok"));
}

void cmd_setBlinkIntervalCallback(cmd* c)
{
  Command cmd(c);
  Argument blinkIntervalArg = cmd.getArgument(F("interval"));
  Argument listArg = cmd.getArgument(F("l"));
  int16_t blinkInterval = blinkIntervalArg.getValue().toInt();
  bool list = listArg.isSet();
  if(!list) {
    if(blinkInterval >= 250 && blinkInterval <=2500) {
      config.blinkInterval = blinkInterval;
    } else {
      Serial.println(F("Error: blinkInterval value invalid. Valid value: 250 to 2500"));
      return;
    }
  }
  Serial.print(F("interval: "));
  Serial.println(config.blinkInterval, DEC);
}

void cmd_setMaxStagesCallback(cmd* c)
{
  Command cmd(c);
  Argument maxStageArg = cmd.getArgument(F("stage"));
  Argument listArg = cmd.getArgument(F("l"));
  int16_t maxStage = maxStageArg.getValue().toInt();
  bool list = listArg.isSet();
  if(!list) {
    if(maxStage >= 1 && maxStage <=5) {
      config.maxStage = maxStage;
    } else {
      Serial.println(F("Error: maxStage value invalid. Valid value: 1 to 5"));
      return;
    }
  }
  Serial.print(F("stage: "));
  Serial.println(config.maxStage, DEC);
}

void cmd_setSteeringCallback(cmd* c)
{
  Command cmd(c);
  Argument channelArg = cmd.getArgument(F("c"));
  Argument neutralArg = cmd.getArgument(F("n"));
  Argument deadzoneArg = cmd.getArgument(F("d"));
  Argument reverseArg = cmd.getArgument(F("r"));
  Argument manualArg = cmd.getArgument(F("m"));
  Argument autoOffArg = cmd.getArgument(F("a"));
  Argument listArg = cmd.getArgument(F("l"));
  int16_t channel = channelArg.getValue().toInt();
  int16_t neutral = neutralArg.getValue().toInt();
  int16_t deadzone = deadzoneArg.getValue().toInt();
  bool reverse  = reverseArg.isSet();
  bool manual = manualArg.isSet();
  bool autoOff = autoOffArg.isSet();
  bool list = listArg.isSet();
  if(!list) {
    if(channel >= 0 && channel < xBUS_CHANNELS_MAX) {
      if(abs(neutral) <= xBUS_CHANNELS_MAX_VALUE) {
        if(abs(deadzone) < xBUS_CHANNELS_MAX_VALUE) {
          config.steering.channel = channel;
          config.steering.neutral = neutral;
          config.steering.deadzone = deadzone;
          config.steering.reverse = reverse;
          config.steering.manual = manual;
          config.steering.autoOff = autoOff;
        } else {
          Serial.print(F("Error: deadzone value invalid. Valid value: -"));
          Serial.print(xBUS_CHANNELS_MAX_VALUE - 1, DEC);
          Serial.print(F(" to "));
          Serial.println(xBUS_CHANNELS_MAX_VALUE - 1, DEC);
          return;
        }
      } else {
        Serial.print(F("Error: neutral value invalid. Valid value: -"));
        Serial.print(xBUS_CHANNELS_MAX_VALUE, DEC);
        Serial.print(F(" to "));
        Serial.println(xBUS_CHANNELS_MAX_VALUE, DEC);
        return;
      }
    } else {
      Serial.print(F("Error: channel value invalid. Valid value: 0 to "));
      Serial.println(xBUS_CHANNELS_MAX, DEC);
      return;
    }
  }
  Serial.print(F("Channel: "));
  Serial.println(config.steering.channel, DEC);
  Serial.print(F("Neutral: "));
  Serial.println(config.steering.neutral, DEC);
  Serial.print(F("Deadzone: "));
  Serial.println(config.steering.deadzone, DEC);
  Serial.print(F("Reverse: "));
  Serial.println(config.steering.reverse?"enabled":"disabled");
  Serial.print(F("Manual: "));
  Serial.println(config.steering.manual?"enabled":"disabled");
  Serial.print(F("Auto Off: "));
  Serial.println(config.steering.autoOff?"enabled":"disabled");
}

void cmd_setThrottleCallback(cmd* c)
{
  Command cmd(c);
  Argument channelArg = cmd.getArgument(F("c"));
  Argument neutralArg = cmd.getArgument(F("n"));
  Argument deadzoneArg = cmd.getArgument(F("d"));
  Argument typeArg = cmd.getArgument(F("t"));
  Argument reverseArg = cmd.getArgument(F("r"));
  Argument listArg = cmd.getArgument(F("l"));
  int16_t channel = channelArg.getValue().toInt();
  int16_t neutral = neutralArg.getValue().toInt();
  int16_t deadzone = deadzoneArg.getValue().toInt();
  int16_t type = typeArg.getValue().toInt();
  bool reverse  = reverseArg.isSet();
  bool list = listArg.isSet();
  if(!list) {
    if(channel >= 0 && channel < xBUS_CHANNELS_MAX) {
      if(abs(neutral) <= xBUS_CHANNELS_MAX_VALUE) {
        if(abs(deadzone) < xBUS_CHANNELS_MAX_VALUE) {
          if(type >= 0 && type <= 2) {
            config.throttle.channel = channel;
            config.throttle.neutral = neutral;
            config.throttle.deadzone = deadzone;
            config.throttle.type = type;
            config.throttle.reverse = reverse;
          } else {
            Serial.println(F("Error: type value invalid. Valid values:"));
            Serial.println(F("0: Forward/Brake/Reverse (Normal mode)"));
            Serial.println(F("1: Forward/Brake (Race mode)"));
            Serial.println(F("2: Forward/Reverse (Crawler mode)"));
            return;
          }
        } else {
          Serial.print(F("Error: deadzone value invalid. Valid value: -"));
          Serial.print(xBUS_CHANNELS_MAX_VALUE - 1, DEC);
          Serial.print(F(" to "));
          Serial.println(xBUS_CHANNELS_MAX_VALUE - 1, DEC);
          return;
        }
      } else {
        Serial.print(F("Error: neutral value invalid. Valid value: -"));
        Serial.print(xBUS_CHANNELS_MAX_VALUE, DEC);
        Serial.print(F(" to "));
        Serial.println(xBUS_CHANNELS_MAX_VALUE, DEC);
        return;
      }
    } else {
      Serial.print(F("Error: channel value invalid. Valid value: 0 to "));
      Serial.println(xBUS_CHANNELS_MAX, DEC);
      return;
    }
  }
  Serial.print(F("Channel: "));
  Serial.println(config.throttle.channel, DEC);
  Serial.print(F("Neutral: "));
  Serial.println(config.throttle.neutral, DEC);
  Serial.print(F("Deadzone: "));
  Serial.println(config.throttle.deadzone, DEC);
  Serial.print(F("Type "));
  switch(config.throttle.type) {
    case THROTTLE_TYPE_FWDBRKREV:
      Serial.println(F("0: Forward/Brake/Reverse (Normal mode)"));
      break;
    case THROTTLE_TYPE_FWDBRK:
      Serial.println(F("1: Forward/Brake (Race mode)"));
      break;
    case THROTTLE_TYPE_FWDREV:
      Serial.println(F("2: Forward/Reverse (Crawler mode)"));
      break;
  }
  Serial.print(F("Reverse: "));
  Serial.println(config.throttle.reverse?"enabled":"disabled");
}

void cmd_setSwitchFCallback(cmd* c)
{
  Command cmd(c);
  Argument switchArg = cmd.getArgument(F("function"));
  Argument channelArg = cmd.getArgument(F("c"));
  Argument neutralArg = cmd.getArgument(F("n"));
  Argument deadzoneArg = cmd.getArgument(F("d"));
  Argument reverseArg = cmd.getArgument(F("r"));
  Argument enabledArg = cmd.getArgument(F("e"));
  Argument listArg = cmd.getArgument(F("l"));
  int16_t switchF = switchArg.getValue().toInt();
  int16_t channel = channelArg.getValue().toInt();
  int16_t neutral = neutralArg.getValue().toInt();
  int16_t deadzone = deadzoneArg.getValue().toInt();
  bool reverse  = reverseArg.isSet();
  bool enabled  = enabledArg.isSet();
  bool list = listArg.isSet();
  if(switchF >= 0 && switchF <= 2) {
    if(!list) {
      if(channel >= 0 && channel < xBUS_CHANNELS_MAX) {
        if(abs(neutral) <= xBUS_CHANNELS_MAX_VALUE) {
          if(abs(deadzone) < xBUS_CHANNELS_MAX_VALUE) {
            config.switchF[switchF].enabled = enabled;
            config.switchF[switchF].channel = channel;
            config.switchF[switchF].neutral = neutral;
            config.switchF[switchF].deadzone = deadzone;
            config.switchF[switchF].reverse = reverse;
          } else {
            Serial.print(F("Error: deadzone value invalid. Valid value: -"));
            Serial.print(xBUS_CHANNELS_MAX_VALUE - 1, DEC);
            Serial.print(F(" to "));
            Serial.println(xBUS_CHANNELS_MAX_VALUE - 1, DEC);
            return;
          }
        } else {
          Serial.print(F("Error: neutral value invalid. Valid value: -"));
          Serial.print(xBUS_CHANNELS_MAX_VALUE, DEC);
          Serial.print(F(" to "));
          Serial.println(xBUS_CHANNELS_MAX_VALUE, DEC);
          return;
        }
      } else {
        Serial.print(F("Error: channel value invalid. Valid value: 0 to "));
        Serial.println(xBUS_CHANNELS_MAX, DEC);
        return;
      }
    }
  } else {
    Serial.println(F("Error: switch value invalid. Valid value: 0 to 2"));
    return;
  }
  Serial.print(F("Enabled: "));
  Serial.println(config.switchF[switchF].enabled?"true":"false");
  Serial.print(F("Channel: "));
  Serial.println(config.switchF[switchF].channel, DEC);
  Serial.print(F("Neutral: "));
  Serial.println(config.switchF[switchF].neutral, DEC);
  Serial.print(F("Deadzone: "));
  Serial.println(config.switchF[switchF].deadzone, DEC);
  Serial.print(F("Reverse: "));
  Serial.println(config.switchF[switchF].reverse?"enabled":"disabled");
}

void cmd_setLEDCallback(cmd* c)
{
  Command cmd(c);
  Argument ledArg = cmd.getArgument(F("pin"));
  Argument listArg = cmd.getArgument(F("l"));
  int16_t led = ledArg.getValue().toInt();
  bool list = listArg.isSet();
  if(led >= 0 && led <= 15) {
    if(!list) {
      aw.analogWrite(led, 255);
      Serial.print(F("Change settings for LED #"));
      Serial.print(led, DEC);
      Serial.println(F(" (y/n)? "));
      if(!cmd_waitForUserInputYN()) {
        aw.analogWrite(led, 0);
        Serial.println(F("Aborted"));
        return;
      }
      bool enabled = false;
      bool combined = false;
      uint8_t type = LIGHT_TYPE_NONE;
      uint8_t min = 0;
      uint8_t max = 0;
      uint8_t stage[5] = {0};
      int16_t value = 0;
      Serial.println(F("Enable this LED (y/n)? "));
      enabled = cmd_waitForUserInputYN();
      Serial.println(F("Choose type of light"));
      Serial.println(F("1 Parking light"));
      Serial.println(F("2 Fog light"));
      Serial.println(F("3 Low beam"));
      Serial.println(F("4 High beam"));
      Serial.println(F("5 Front extra"));
      Serial.println(F("6 Right indicator"));
      Serial.println(F("7 Left indicator"));
      Serial.println(F("8 Brake light"));
      Serial.println(F("9 Reverse light"));
      Serial.println(F("10 Rear extra"));
      value = cmd_waitForUserInput();
      if(value >= 1 && value <= 10) {
        type = value;
        if(type == 6 || type == 7) {
          Serial.println(F("Combine indicator with brake (US-Style) (y/n)? "));
          combined = cmd_waitForUserInputYN();
        }
        if(type == 5) {
          Serial.println(F("Combine front extra with flashlight/high beam (y/n)? "));
          combined = cmd_waitForUserInputYN();
        }
        Serial.println(F("Min pwm value for this LED (0-255): "));
        value = cmd_waitForUserInput();
        if(value >= 0 && value <= 255) {
          min = value;
          Serial.println(F("Max pwm value for this LED (0-255): "));
          value = cmd_waitForUserInput();
          if(value >= 0 && value <= 255) {
            max = value;
            for(uint8_t i=0; i<=config.maxStage; i++) {
              Serial.print(F("Value for stage #"));
              Serial.print(i);
              Serial.println(F(" (0-255): "));
              value = cmd_waitForUserInput();
              if(value < 0 || value > 255) {
                aw.analogWrite(led, 0);
                Serial.println(F("Error: stage value invalid. Valid value: 0 to 255"));
                return;
              }
              stage[i] = value;
            }
            Serial.println(F("Do you want to keep this settings (y/n)? "));
            if(!cmd_waitForUserInputYN()) {
              aw.analogWrite(led, 0);
              Serial.println(F("Aborted"));
              return;
            }
            config.led[led].enabled = enabled;
            config.led[led].combined = combined;
            config.led[led].type = type;
            config.led[led].min = min;
            config.led[led].max = max;
            for(uint8_t j=0; j<=config.maxStage; j++) {
              config.led[led].stage[j] = stage[j];
            }
            aw.analogWrite(led, 0);
          } else {
            aw.analogWrite(led, 0);
            Serial.println(F("Error: max value invalid. Valid value: 0 to 255"));
            return;
          }
        } else {
          aw.analogWrite(led, 0);
          Serial.println(F("Error: min value invalid. Valid value: 0 to 255"));
          return;
        }
      } else {
        aw.analogWrite(led, 0);
        Serial.println(F("Error: type value invalid. Valid value: 0 to 10"));
        return;
      }
    }
  } else {
    aw.analogWrite(led, 0);
    Serial.println(F("Error: led value invalid. Valid value: 0 to 15"));
    return;
  }
  Serial.print(F("Enabled: "));
  Serial.println(config.led[led].enabled?"true":"false");
  Serial.print(F("Type: "));
  switch(config.led[led].type) {
    case LIGHT_TYPE_NONE:
      Serial.println(F("N/A"));
      break;
    case LIGHT_TYPE_PARKING:
      Serial.println(F("Parking light"));
      break;
    case LIGHT_TYPE_FOG:
      Serial.println(F("Fog light"));
      break;
    case LIGHT_TYPE_LOWBEAM:
      Serial.println(F("Low beam"));
      break;
    case LIGHT_TYPE_HIGHBEAM:
      Serial.println(F("High beam"));
      break;
    case LIGHT_TYPE_FRONTEXTRA:
      Serial.println(F("Front extra"));
      break;
    case LIGHT_TYPE_RIGHTINDICATOR:
      Serial.println(F("Right indicator"));
      Serial.print(F("Combined (US-Style): "));
      Serial.println(config.led[led].combined?"enabled":"disabled");
      break;
    case LIGHT_TYPE_LEFTINDICATOR:
      Serial.println(F("Left indicator"));
      Serial.print(F("Combined (US-Style): "));
      Serial.println(config.led[led].combined?"enabled":"disabled");
      break;
    case LIGHT_TYPE_BRAKE:
      Serial.println(F("Brake light"));
      break;
    case LIGHT_TYPE_REVERSE:
      Serial.println(F("Reverse light"));
      break;
    case LIGHT_TYPE_REAREXTRA:
      Serial.println(F("Rear extra"));
      break;
  }
  Serial.print(F("Min value: "));
  Serial.println(config.led[led].min, DEC);
  Serial.print(F("Max value: "));
  Serial.println(config.led[led].max, DEC);
  Serial.print(F("Stages: "));
  Serial.print(F("{"));
  for(uint8_t k=0; k<=config.maxStage; k++) {
    Serial.print(config.led[led].stage[k]);
    if(k < config.maxStage) Serial.print(F(", "));
  }
  Serial.println(F("}"));
}

bool cmd_waitForUserInputYN(void)
{
  while(true) {
    if(Serial.available()) {
      char c = Serial.read();
      if(c == 'y' || c == 'Y') {
        while(Serial.available()) {
          Serial.read();
        }
        return true;
      } else {
        while(Serial.available()) {
          Serial.read();
        }
        return false;
      }
    }
    yield();
  }
}

int16_t cmd_waitForUserInput(void)
{
  while(true) {
    if(Serial.available()) {
      int16_t value = Serial.parseInt();
      while(Serial.available()) {
        Serial.read();
      }
      return value;
    }
    yield();
  }
}

void xBUS_create()
{
  // steering config
  xBUS.setEnabled(config.steering.channel, true);
  if(config.steering.reverse) xBUS.setReverse(config.steering.channel, true);
  if(config.steering.neutral != 0) xBUS.setNeutral(config.steering.channel, config.steering.neutral);
  if(config.steering.deadzone != 0) xBUS.setDeadzone(config.steering.channel, config.steering.deadzone);

  // throttle config
  xBUS.setEnabled(config.throttle.channel, true);
  if(config.throttle.reverse) xBUS.setReverse(config.throttle.channel, true);
  if(config.throttle.neutral != 0) xBUS.setNeutral(config.throttle.channel, config.throttle.neutral);
  if(config.throttle.deadzone != 0) xBUS.setDeadzone(config.throttle.channel, config.throttle.deadzone);

  // switch F1 config
  xBUS.setEnabled(config.switchF[0].channel, true);
  xBUS.setDigital(config.switchF[0].channel, true);
  if(config.switchF[0].reverse) xBUS.setReverse(config.switchF[0].channel, true);
  if(config.switchF[0].neutral != 0) xBUS.setNeutral(config.switchF[0].channel, config.switchF[0].neutral);
  if(config.switchF[0].deadzone != 0) xBUS.setDeadzone(config.switchF[0].channel, config.switchF[0].deadzone);

  // switch F2 config
  if(config.switchF[1].enabled) {
    xBUS.setEnabled(config.switchF[1].channel, true);
    xBUS.setDigital(config.switchF[1].channel, true);
    if(config.switchF[1].reverse) xBUS.setReverse(config.switchF[1].channel, true);
    if(config.switchF[1].neutral != 0) xBUS.setNeutral(config.switchF[1].channel, config.switchF[1].neutral);
    if(config.switchF[1].deadzone != 0) xBUS.setDeadzone(config.switchF[1].channel, config.switchF[1].deadzone);
  }

  // switch F3 config
  if(config.switchF[2].enabled) {
    xBUS.setEnabled(config.switchF[2].channel, true);
    xBUS.setDigital(config.switchF[2].channel, true);
    if(config.switchF[2].reverse) xBUS.setReverse(config.switchF[2].channel, true);
    if(config.switchF[2].neutral != 0) xBUS.setNeutral(config.switchF[2].channel, config.switchF[2].neutral);
    if(config.switchF[2].deadzone != 0) xBUS.setDeadzone(config.switchF[2].channel, config.switchF[2].deadzone);
  }

  xBUS.attachEvent(xBUS_EVENT_FAILSAFE, xBUS_eventCallback);
  xBUS.attachEvent(xBUS_EVENT_TIMEOUT, xBUS_eventCallback);
  xBUS.attachEvent(xBUS_EVENT_VALUE_CHANGED, xBUS_eventCallback);
  xBUS.attachEvent(xBUS_EVENT_CLICKED, xBUS_eventCallback);
  xBUS.attachEvent(xBUS_EVENT_PRESSED, xBUS_eventCallback);
  xBUS.attachEvent(xBUS_EVENT_LONG_PRESSED, xBUS_eventCallback);
  xBUS.attachEvent(xBUS_EVENT_RELEASED, xBUS_eventCallback);
}

void xBUS_eventCallback(int eventCode, int eventParam) {
  if(eventCode == xBUS_EVENT_FAILSAFE || eventCode == xBUS_EVENT_TIMEOUT) {
    // Failsafe and timeout event
    indicatorState = INDICATOR_STATE_OFF;
    if(config.throttle.type == THROTTLE_TYPE_FWDREV) {
      directionState = DIRECTION_STATE_BRAKE;
    }
    directionStage = DIRECTION_STATE_OFF;
    flashLightState = false;
    highBeamState = false;
    fogLightState = false;
    frontExtraState = false;
    rearExtraState = false;
    currentStage = 0;
  }
  if(eventParam == config.steering.channel && eventCode == xBUS_EVENT_VALUE_CHANGED) {
    // Steering axis event
    if(!config.steering.manual) {
      if(xBUS.getChannel(eventParam) > 0) {
        // right
        indicatorState = INDICATOR_STATE_RIGHT;
      } else if(xBUS.getChannel(eventParam) < 0) {
        // left
        indicatorState = INDICATOR_STATE_LEFT;
      } else {
        // neutral
        indicatorState = INDICATOR_STATE_OFF;
      }
    } else if(config.steering.autoOff) {
      if(xBUS.getChannel(eventParam) > 0 && indicatorState == INDICATOR_STATE_LEFT) {
        // steering to right
        indicatorState = INDICATOR_STATE_OFF;
      } else if(xBUS.getChannel(eventParam) < 0 && indicatorState == INDICATOR_STATE_RIGHT) {
        // steering to left
        indicatorState = INDICATOR_STATE_OFF;
      }
    }
  }
  if(eventParam == config.throttle.channel && eventCode == xBUS_EVENT_VALUE_CHANGED) {
    // Throttle axis event
    if(config.throttle.type == THROTTLE_TYPE_FWDBRK) {
      // Race mode
      if(xBUS.getChannel(eventParam) < 0) {
        // brake
        directionState = DIRECTION_STATE_BRAKE;
      } else {
        // forward/neutral
        directionState = DIRECTION_STATE_OFF;
      }
    } else if(config.throttle.type == THROTTLE_TYPE_FWDBRKREV) {
      // Normal mode
      if(xBUS.getChannel(eventParam) > 0) {
        // forward
        directionState = DIRECTION_STATE_FORWARD;
        directionStage = DIRECTION_STATE_OFF;
      } else if(xBUS.getChannel(eventParam) < 0 && directionStage == DIRECTION_STATE_OFF) {
        // brake
        directionState = DIRECTION_STATE_BRAKE;
      } else if(xBUS.getChannel(eventParam) < 0 && directionStage == DIRECTION_STATE_REVERSE) {
        // reverse
        directionState = DIRECTION_STATE_REVERSE;
      } else if(directionState == DIRECTION_STATE_BRAKE) {
        // neutral after brake
        directionState = DIRECTION_STATE_OFF;
        directionStage = DIRECTION_STATE_REVERSE;
      } else if(directionState == DIRECTION_STATE_REVERSE) {
        // neutral after reverse
        directionState = DIRECTION_STATE_OFF;
      } else {
        // neutral
        directionState = DIRECTION_STATE_OFF;
      }
    } else if(config.throttle.type == THROTTLE_TYPE_FWDREV) {
      // Crawler mode
      if(xBUS.getChannel(eventParam) > 0) {
        // forward
        directionState = DIRECTION_STATE_OFF;
      } else if(xBUS.getChannel(eventParam) < 0) {
        // reverse
        directionState = DIRECTION_STATE_REVERSE;
      } else {
        // neutral
        directionState = DIRECTION_STATE_BRAKE;
      }
    }
  }
  if(eventParam == config.switchF[0].channel) {
    // Switch F1 event
    if(eventCode == xBUS_EVENT_CLICKED) {
      if(xBUS.getLastChannel(eventParam) < 0) {
        if(indicatorState != INDICATOR_STATE_LEFT) {
          indicatorState = INDICATOR_STATE_LEFT;
        } else {
          indicatorState = INDICATOR_STATE_OFF;
        }
      } else {
        if(indicatorState != INDICATOR_STATE_RIGHT) {
          indicatorState = INDICATOR_STATE_RIGHT;
        } else {
          indicatorState = INDICATOR_STATE_OFF;
        }
      }
    }
    if(eventCode == xBUS_EVENT_LONG_PRESSED) {
      if(xBUS.getLastChannel(eventParam) < 0) {
        if(indicatorState != INDICATOR_STATE_HAZZARD) {
          indicatorState = INDICATOR_STATE_HAZZARD;
        } else {
          indicatorState = INDICATOR_STATE_OFF;
        }
      } else {
        if(currentStage < config.maxStage) {
          currentStage++;
        } else {
          currentStage = 0;
        }
      }
    }
  }
  if(eventParam == config.switchF[1].channel && config.switchF[1].enabled) {
    // Switch F2 event
    if(eventCode == xBUS_EVENT_PRESSED) {
      if(xBUS.getLastChannel(eventParam) > 0) flashLightState = true;
    } else if(eventCode == xBUS_EVENT_RELEASED) {
      flashLightState = false;
    }
    if(eventCode == xBUS_EVENT_LONG_PRESSED) {
      if(xBUS.getLastChannel(eventParam) > 0) {
        highBeamState = !highBeamState;
      } else {
        fogLightState = !fogLightState;
      }
    }
  }
  if(eventParam == config.switchF[2].channel && config.switchF[2].enabled && eventCode == xBUS_EVENT_CLICKED) {
    // Switch F3 event
    if(xBUS.getLastChannel(eventParam) > 0) {
      frontExtraState = !frontExtraState;
    } else {
      rearExtraState = !rearExtraState;
    }
  }
}