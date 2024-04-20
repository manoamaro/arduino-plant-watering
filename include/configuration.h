#ifndef CONFIGURATION_H
#define CONFIGURATION_H

#include <Arduino.h>
#include <EEPROMex.h>

#include "pump.h"
#include "sensor_config.h"

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

#define ALPHA 30
#define ALPHA_SCALE 100

int filterNoise(int lastMeasure, int newMeasure);


#endif /* CONFIGURATION_H */
