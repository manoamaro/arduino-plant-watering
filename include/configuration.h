#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>
#include <EEPROMex.h>

#include "pump.h"
#include "sensor_config.h"

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

#define MAX_PUMP_POWER 100
#define MIN_PUMP_POWER 1
#define STEPS_PUMP_POWER 5
#define DEFAULT_PUMP_POWER 80

struct EEPROM_MEM {
  PumpConfig pumpConfigs[3];
  SensorConfig sensorConfig;
};

/**
 * Load all Pumps Config from the EEPROM to the memory.
 * If the Pump was already initialized, just loads the config;
 */
void loadEEPROM(Pump* pumps, uint8_t pumpsCount, SensorConfig* sensorConfig);

/**
 * Save all the in memory Pump configuration into the EEPROM
 */
void saveEEPROM(Pump* pumps, uint8_t pumpsCount, SensorConfig sensorConfig);


void incFrequency(Pump* pump, bool up);

void incSecondsPump(Pump* pump, bool up);

void incPumpPower(Pump* pump, bool up);

void incSoilSensor(Pump* pump, bool up);

void incLightSensor(Pump* pump, bool up);

#define ALPHA 30
#define ALPHA_SCALE 100

int filterNoise(int lastMeasure, int newMeasure);


#endif /* CONFIGURATION_H */
