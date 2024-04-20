#ifndef PUMP_CONFIG_H
#define PUMP_CONFIG_H

#include <Arduino.h>

struct PumpConfig {
  int frequency; // in minutes, for e.g. every 30min or 720min(12h)
  int secondsPump; // Time in sec to keep the pump running
  int power; // 1-100% power
  uint8_t soilSensor; // Run pump if it's below this level (0-100)
  uint8_t lightSensor; // Run pump if it's above this level (0-100)
};

#endif /* PUMP_CONFIG_H */