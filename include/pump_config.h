#ifndef PUMP_CONFIG_H
#define PUMP_CONFIG_H

#include <Arduino.h>

#define STEPS_FREQUENCY 5
#define DEFAULT_FREQUENCY 30
#define MAX_FREQUENCY 1440
#define MIN_FREQUENCY 5

#define STEPS_SECONDS_PUMP 5
#define DEFAULT_SECONDS_PUMP 30
#define MAX_SECONDS_PUMP 300
#define MIN_SECONDS_PUMP 5

#define STEPS_SOIL_SENSOR 5
#define DEFAULT_SOIL_SENSOR 60
#define MAX_SOIL_SENSOR 100
#define MIN_SOIL_SENSOR 0

#define STEPS_LIGHT_SENSOR 5
#define DEFAULT_LIGHT_SENSOR 60
#define MAX_LIGHT_SENSOR 100
#define MIN_LIGHT_SENSOR 0

#define MAX_PUMP_POWER 100
#define MIN_PUMP_POWER 1
#define STEPS_PUMP_POWER 5
#define DEFAULT_PUMP_POWER 80

struct PumpConfig {
  int frequency; // in minutes, for e.g. every 30min or 720min(12h)
  int secondsPump; // Time in sec to keep the pump running
  int power; // 1-100% power
  uint8_t soilSensor; // Run pump if it's below this level (0-100)
  uint8_t lightSensor; // Run pump if it's above this level (0-100)
};

#endif /* PUMP_CONFIG_H */