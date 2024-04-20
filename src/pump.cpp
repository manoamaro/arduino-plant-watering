#include "pump.h"

Pump::Pump(uint8_t pin) : pin(pin), lastRunMs(0), startedAtMs(0), running(false), config({DEFAULT_FREQUENCY, DEFAULT_SECONDS_PUMP, DEFAULT_PUMP_POWER, DEFAULT_SOIL_SENSOR, DEFAULT_LIGHT_SENSOR}) {
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

uint8_t Pump::getPin() {
  return this->pin;
}

bool Pump::isRunning() {
  return this->running;
}

void Pump::setRunning(bool running) {
  this->running = running;
}

uint32_t Pump::getStartedAtMs() {
  return this->startedAtMs;
}

void Pump::setStartedAtMs(uint32_t startedAtMs) {
  this->startedAtMs = startedAtMs;
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

void Pump::incFrequency(bool up) {
  uint8_t value = config.frequency + (up ? STEPS_FREQUENCY : -STEPS_FREQUENCY);
  config.frequency = constrain(value, MIN_FREQUENCY, MAX_FREQUENCY);
}

void Pump::incSecondsPump(bool up) {
  uint8_t value = config.secondsPump + (up ? STEPS_SECONDS_PUMP : -STEPS_SECONDS_PUMP);
  config.secondsPump = constrain(value, MIN_SECONDS_PUMP, MAX_SECONDS_PUMP);
}

void Pump::incPumpPower(bool up) {
  uint8_t value = config.power + (up ? STEPS_PUMP_POWER : -STEPS_PUMP_POWER);
  config.power = constrain(value, MIN_PUMP_POWER, MAX_PUMP_POWER);
}

void Pump::incSoilSensor(bool up) {
  uint8_t value = config.soilSensor + (up ? STEPS_SOIL_SENSOR : -STEPS_SOIL_SENSOR);
  config.soilSensor = constrain(value, MIN_SOIL_SENSOR, MAX_SOIL_SENSOR);
}

void Pump::incLightSensor(bool up) {
  uint8_t value = config.lightSensor + (up ? STEPS_LIGHT_SENSOR : -STEPS_LIGHT_SENSOR);
  config.lightSensor = constrain(value, MIN_LIGHT_SENSOR, MAX_LIGHT_SENSOR);
}

