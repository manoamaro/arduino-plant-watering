APP_NAME := arduino-plant-watering
ARDUINO_LIBS := Adafruit_BusIO Adafruit_PCD8544_Nokia_5110_LCD_library Adafruit_GFX_Library DHT_sensor_library Adafruit_Unified_Sensor Adafruit_Sensor DHT DHT_U EpoxyEepromAvr
ARDUINO_LIB_DIRS := /Users/manoel/Library/Arduino15/packages/arduino/hardware/avr/1.8.3/libraries /Users/manoel/Documents/Arduino/libraries
include ./libraries/EpoxyDuino/EpoxyDuino.mk
