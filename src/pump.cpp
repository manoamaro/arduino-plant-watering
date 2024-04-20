#include "pump.h"

Pump::Pump() {
  this->lastRunMs = 0;
}

void Pump::setLastRunMs(uint32_t lastRunMs) {
  this->lastRunMs = lastRunMs;
}

uint32_t Pump::getLastRunMs() {
  return this->lastRunMs;
}

void Pump::setConfig(PumpConfig config) {
  this->config = config;
}

PumpConfig Pump::getConfig() {
  return this->config;
}

uint16_t Pump::secondsToNextRun(unsigned long currentMillis) {
  if (currentMillis < lastRunMs) {
    lastRunMs = 0ul;
  }
  unsigned long nextRunMs = lastRunMs + (config.frequency * 60ul * 1000ul);
  return uint16_t((nextRunMs - currentMillis) / 1000ul);
}

bool Pump::isTimeToRun(unsigned long currentMillis, SensorData sensorData, uint8_t idx) {
  bool shouldRun = (currentMillis - lastRunMs) >= (config.frequency * 60ul * 1000ul);
  if (shouldRun) {
    lastRunMs = currentMillis;
  }

  shouldRun = shouldRun &&
    (sensorData.soilMoisture[idx] <= config.soilSensor) &&
    (sensorData.light >= config.lightSensor);

  return shouldRun;

}

