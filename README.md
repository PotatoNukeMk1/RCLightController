 # RC Light Controller
 
 ### What you need:

* [Adafruit Qt Py M0](https://www.adafruit.com/product/4600) (Seeed Studio XIAO SAMD21 maybe also work but don't fits into the case)
* [SBUS inverter](https://oshwlab.com/schmron/sbusinverter)
* [Adafruit AW9523](https://www.adafruit.com/product/4886)
* [50mm STEMMA QT cable](https://www.adafruit.com/product/4399)
* [Flash chip](https://www.adafruit.com/product/4763)
* [3D printed case](https://www.printables.com/model/806489-rc-light-controller-case) (i tried W25Q128JV but because of the wrong package its hard to solder it)
* [Firmware](https://github.com/PotatoNukeMk1/RCLightController/releases)

### Features:
* Only one cable from receiver to light controller (Be sure your receiver can handle another 500mA current)
* [Futaba S.BUS](https://github.com/PotatoNukeMk1/SBUS) or [Flysky i-bus](https://github.com/PotatoNukeMk1/IBUS) protocol. PPM sum signal decoder follows later
* 3-5 channel input (steering, throttle and one to three 3-position switches)
* Neutral point and deadzone configurable
* Indicator controlled by steering, manual with or without automatic stopping
* 3 Throttle modes (Fwd/Rev, Fwd/Brk/Rev, Fwd/Rev)
* Driver with constant current for 16 configurable LEDs (no resistor needed)
* 5 stages for fixed lights (e.g. parking light, low beam, ...)
* Special lights directly connected to a switch (e.g. flash light/high beam, brake, indicators, ...)
* Combined (US-style) brake/indicator
* Configuration via serial cli. You just need a serial terminal (PuTTY, CuteCom, ...) and a USB-C cable

### Light types:
* Parking light (Front/Rear)
* Fog light (Front/Rear; Switch F2)
* Low beam (Front)
* High beam (Front; Flashlight; Switch F2)
* Front extra (Front; Flashlight; Switch F3)
* Right indicator (Front/Rear; Switch F1; US-style)
* Left indicator (Front/Rear; Switch F1; US-style)
* Brake light (Rear; Throttle)
* Reverse light (Rear; Throttle)
* Rear extra (Rear)

Bootloader update and firmware: You may need to update the bootloader of your Qt Py M0. Please do this before flashing the firmware. To get the UF2 boot drive, just double click the reset button. To flash the firmware just copy the firmware file into the drive mounted on your computer.

After bootloader update you need to run this SPI Flash example first: https://github.com/adafruit/Adafruit_SPIFlash/tree/master/examples/flash_erase

You need to configure channels and LEDs at first run. All inputs and LEDs are unconfigured so don't worry if it stays dark.

Overall price is maybe $20 (EasyEDA offers some coupons for new users). Most products i found with similar features (for 1:14 trucks) costs $100 or more

## Credits

Thanks to [Igor Mikolic-Torreira](https://github.com/igormiktor/) (EventManager)

## Copyright

Copyright (c) 2024 Ronald Schmid