#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#define EEPROM_EX

#ifdef EEPROM_EX
#include <EEPROMex.h>
#endif

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

#define MIN_SOIL_SENSOR_CALIBRATION 0
#define MAX_SOIL_SENSOR_CALIBRATION 1023

#define MIN_LIGHT_SENSOR_CALIBRATION 0
#define MAX_LIGHT_SENSOR_CALIBRATION 1023

#define NUM_PUMPS 1

struct PumpConfig {
  int frequency; // in minutes, for e.g. every 30min or 720min(12h)
  int secondsPump; // Time in sec to keep the pump running
  uint8_t soilSensor; // Run pump if it's below this level (0-100)
  uint8_t lightSensor; // Run pump if it's above this level (0-100)
  int soilSensorAirValue; // Calibration value
  int soilSensorWaterValue; // Calibration value
};

struct Pump {
  /*
   * Configuration
   */
  PumpConfig config;

  /**
   * Sensor data
   */
  uint8_t soilMoisture;

  /**
   * Last time the pump was activated
   */
  uint32_t lastRunMs;
};

struct EEPROM_MEM {
  PumpConfig pumpConfigs[NUM_PUMPS];
  int lightDayValue;
  int lightNightValue;
};

struct {
  int8_t temperature;
  uint8_t humidity;
  uint8_t light;
  int lightDayValue;
  int lightNightValue;
} sensorData;

Pump pumps[NUM_PUMPS];

/**
 * Load all Pumps Config from the EEPROM to the memory.
 * If the Pump was already initialized, just loads the config;
 */
void loadEEPROM() {
  EEPROM_MEM mem;
#ifdef EEPROM_EX
  EEPROM.readBlock(0, mem);
#endif
  for (uint8_t i = 0; i < NUM_PUMPS; i++) {
    PumpConfig _config = mem.pumpConfigs[i];

    if (_config.frequency <= MIN_FREQUENCY || _config.frequency > MAX_FREQUENCY || _config.frequency % STEPS_FREQUENCY != 0) {
      _config.frequency = DEFAULT_FREQUENCY;
    }
    if (_config.secondsPump <= MIN_SECONDS_PUMP || _config.secondsPump > MAX_SECONDS_PUMP || _config.secondsPump % STEPS_SECONDS_PUMP != 0) {
      _config.secondsPump = DEFAULT_SECONDS_PUMP;
    }
    if (_config.soilSensor <= MIN_SOIL_SENSOR || _config.soilSensor > MAX_SOIL_SENSOR || _config.soilSensor % STEPS_SOIL_SENSOR != 0) {
      _config.soilSensor = DEFAULT_SOIL_SENSOR;
    }
    if (_config.lightSensor <= MIN_LIGHT_SENSOR || _config.lightSensor > MAX_LIGHT_SENSOR || _config.lightSensor % STEPS_LIGHT_SENSOR != 0) {
      _config.lightSensor = DEFAULT_LIGHT_SENSOR;
    }
    if (_config.soilSensorAirValue < MIN_SOIL_SENSOR_CALIBRATION || _config.soilSensorAirValue > MAX_SOIL_SENSOR_CALIBRATION) {
      _config.soilSensorAirValue = MIN_SOIL_SENSOR_CALIBRATION;
    }
    if (_config.soilSensorWaterValue < MIN_SOIL_SENSOR_CALIBRATION || _config.soilSensorWaterValue > MAX_SOIL_SENSOR_CALIBRATION) {
      _config.soilSensorWaterValue = MAX_SOIL_SENSOR_CALIBRATION;
    }
    pumps[i].config = _config;
  }
  sensorData.lightDayValue = mem.lightDayValue;
  constrain(sensorData.lightDayValue, MIN_LIGHT_SENSOR_CALIBRATION, MAX_LIGHT_SENSOR_CALIBRATION);
  sensorData.lightNightValue = mem.lightNightValue;
  constrain(sensorData.lightNightValue, MIN_LIGHT_SENSOR_CALIBRATION, MAX_LIGHT_SENSOR_CALIBRATION);
}

/**
 * Save all the in memory Pump configuration into the EEPROM
 */
void saveEEPROM() {
  EEPROM_MEM mem;
#ifdef EEPROM_EX
  EEPROM.readBlock(0, mem);
#endif
  for (uint8_t i = 0; i < NUM_PUMPS; i++) {
    Pump* pump = &pumps[i];
    mem.pumpConfigs[i] = pump->config;
    pump -> lastRunMs = 0ul;
  }
  mem.lightDayValue = sensorData.lightDayValue;
  mem.lightNightValue = sensorData.lightNightValue;
  
#ifdef EEPROM_EX
  EEPROM.updateBlock(0, mem);
#endif
}


void incFrequency(Pump* pump, bool up) {
  uint8_t value = pump->config.frequency + (up ? STEPS_FREQUENCY : -STEPS_FREQUENCY);
  pump->config.frequency = constrain(value, MIN_FREQUENCY, MAX_FREQUENCY);
}

void incSecondsPump(Pump* pump, bool up) {
  uint8_t value = pump->config.secondsPump + (up ? STEPS_SECONDS_PUMP : -STEPS_SECONDS_PUMP);
  pump->config.secondsPump = constrain(value, MIN_SECONDS_PUMP, MAX_SECONDS_PUMP);
}

void incSoilSensor(Pump* pump, bool up) {
  uint8_t value = pump->config.soilSensor + (up ? STEPS_SOIL_SENSOR : -STEPS_SOIL_SENSOR);
  pump->config.soilSensor = constrain(value, MIN_SOIL_SENSOR, MAX_SOIL_SENSOR);
}

void incLightSensor(Pump* pump, bool up) {
  uint8_t value = pump->config.lightSensor + (up ? STEPS_LIGHT_SENSOR : -STEPS_LIGHT_SENSOR);
  pump->config.lightSensor = constrain(value, MIN_LIGHT_SENSOR, MAX_LIGHT_SENSOR);
}

uint16_t secondsToNextRun(Pump* pump, unsigned long currentMillis) {
  if (currentMillis < pump->lastRunMs) {
    pump->lastRunMs = 0ul;
  }
  unsigned long nextRunMs = pump->lastRunMs + (pump->config.frequency * 60ul * 1000ul);
  return uint16_t((nextRunMs - currentMillis) / 1000ul);
}

bool isTimeToRun(Pump* pump, unsigned long currentMillis) {
  bool shouldRun = (currentMillis - pump->lastRunMs) >= (pump->config.frequency * 60ul * 1000ul);
  if (shouldRun) {
    pump->lastRunMs = currentMillis;
  }

  shouldRun = shouldRun &&
    (pump->soilMoisture <= pump->config.soilSensor) &&
    (sensorData.light >= pump->config.lightSensor);

  return shouldRun;
}

#define ALPHA 30
#define ALPHA_SCALE 100

int filterNoise(int lastMeasure, int newMeasure) {
  if (lastMeasure == 0) {
    return newMeasure;
  } else {
    return (ALPHA * newMeasure + (ALPHA_SCALE - ALPHA) * lastMeasure) / ALPHA_SCALE;
  }
}


#endif /* CONFIGURATION_H */
