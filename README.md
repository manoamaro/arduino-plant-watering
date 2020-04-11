# Arduino Watering Plants System

Simple DIY plant watering system made with Arduino.

## Components:
  * Arduino Nano (can also use the UNO)
  * PCD8544 Display (Nokia 5110) to show information and settings
  * two buttons as user input
  * DHT02 temperature and humidity sensor
  * 5V DC Water pump
  * BC548 NPN transistor to controll the water pump
  * Some resistors and LED

## Schema:

![Schema](https://github.com/manoamaro/arduino-plant-watering/blob/master/schema.png)

## Protoboard:

![Schema](https://github.com/manoamaro/arduino-plant-watering/blob/master/protoboard.jpg)

## Final Version:

![Final Version](https://github.com/manoamaro/arduino-plant-watering/blob/master/final.jpg)

## TODOs:
  * Use the Temperature and Humidity as input to decide if should activate the water plants or not.
  * Replace transistor with relay if want to use a bigger water pump.