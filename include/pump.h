#ifndef PUMP_H
#define PUMP_H

#include <Arduino.h>
#include "pump_config.h"
#include "sensor_data.h"

class Pump {
  private:
    uint32_t lastRunMs;
    PumpConfig config;
  public:
    Pump();
    void setSoilMoisture(uint8_t soilMoisture);
    uint8_t getSoilMoisture();
    void setLastRunMs(uint32_t lastRunMs);
    uint32_t getLastRunMs();
    void setConfig(PumpConfig config);
    PumpConfig getConfig();
    uint16_t secondsToNextRun(unsigned long currentMillis);
    bool isTimeToRun(unsigned long currentMillis, SensorData sensorData, uint8_t idx);
};

#endif /* PUMP_H */