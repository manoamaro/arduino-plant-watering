# Arduino Watering Plants System

Simple DIY plant watering system made with Arduino Uno/nano. 


## Components:
  * Arduino Nano (can also use the UNO) for prototyping
  * ATMega328P MCU for final version
  * SSD1306 OLED 128x64 to show information and settings
  * three buttons as user input
  * DHT22 temperature and humidity sensor
  * 5V DC Water pumps
  * TIP120 NPN transistors to control the water pumps
  * Some resistors and diodes
  * LM7805 for powering the final version with +7.5V power supply
    * AMS1117 can also be used if power supply is lower than 7.5v

## Schema:

This is the final schema using ATMega328P, with 3 pumps. The amount of pumps can be programmed to as many IO pins are available.

![Schema](https://github.com/manoamaro/arduino-plant-watering/blob/master/schema.png)

## TODOs:
  * Use the Temperature and Humidity as input to decide if should activate the water plants or not.
  * Replace transistor with relay if want to use a bigger water pump.