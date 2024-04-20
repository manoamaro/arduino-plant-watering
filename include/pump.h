#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>
#include "pump_config.h"
#include "sensor_data.h"

class Pump {
  private:
    uint32_t lastRunMs;
    uint32_t startedAtMs;
    PumpConfig config;
    uint8_t pin;
    bool running = false;
  public:
    Pump(uint8_t pin);
    void setLastRunMs(uint32_t lastRunMs);
    uint32_t getLastRunMs();
    void setConfig(PumpConfig config);
    PumpConfig getConfig();
    uint8_t getPin();
    bool isRunning();
    void setRunning(bool running);
    uint32_t getStartedAtMs();
    void setStartedAtMs(uint32_t startedAtMs);
    void incFrequency(bool up);
    void incSecondsPump(bool up);
    void incPumpPower(bool up);
    void incSoilSensor(bool up);
    void incLightSensor(bool up);
    uint16_t secondsToNextRun(unsigned long currentMillis);
    bool isTimeToRun(unsigned long currentMillis, SensorData sensorData, uint8_t idx);
};

#endif /* PUMP_H */