#include "configuration.h"

int clamp(int amt, int low, int high) {
  return ((amt) < (low) ? (low) : ((amt) > (high) ? (high) : (amt)));
}

int clamp(int amt, int low, int high, int step, int def) {
  return amt % step == 0 ? clamp(amt, low, high) : def;
}

void loadEEPROM(Pump* pumps, uint8_t pumpsCount, SensorConfig* sensorConfig)
{
  EEPROM_MEM mem;
  EEPROM.readBlock(0, mem);
  for (uint8_t i = 0; i < pumpsCount; i++)
  {
    PumpConfig menConfig = mem.pumpConfigs[i];
    // Check if the value is in the range, if not, set the default value
    menConfig.frequency = clamp(menConfig.frequency, MIN_FREQUENCY, MAX_FREQUENCY, STEPS_FREQUENCY, DEFAULT_FREQUENCY);
    menConfig.secondsPump = clamp(menConfig.secondsPump, MIN_SECONDS_PUMP, MAX_SECONDS_PUMP, STEPS_SECONDS_PUMP, DEFAULT_SECONDS_PUMP);
    menConfig.power = clamp(menConfig.power, MIN_PUMP_POWER, MAX_PUMP_POWER, STEPS_PUMP_POWER, DEFAULT_PUMP_POWER);
    menConfig.soilSensor = clamp(menConfig.soilSensor, MIN_SOIL_SENSOR, MAX_SOIL_SENSOR, STEPS_SOIL_SENSOR, DEFAULT_SOIL_SENSOR);
    menConfig.lightSensor = clamp(menConfig.lightSensor, MIN_LIGHT_SENSOR, MAX_LIGHT_SENSOR, STEPS_LIGHT_SENSOR, DEFAULT_LIGHT_SENSOR);
    pumps[i].setConfig(menConfig);
  }

  for (int i = 0; i < 3; i++) {
    sensorConfig->soilSensorDryValue[i] = clamp(mem.sensorConfig.soilSensorDryValue[i], MIN_SOIL_SENSOR_CALIBRATION, MAX_SOIL_SENSOR_CALIBRATION);
    sensorConfig->soilSensorWetValue[i] = clamp(mem.sensorConfig.soilSensorWetValue[i], MIN_SOIL_SENSOR_CALIBRATION, MAX_SOIL_SENSOR_CALIBRATION);
  }
  sensorConfig->lightSensorDayValue = clamp(mem.sensorConfig.lightSensorDayValue, MIN_LIGHT_SENSOR_CALIBRATION, MAX_LIGHT_SENSOR_CALIBRATION);
  sensorConfig->lightSensorNightValue = clamp(mem.sensorConfig.lightSensorNightValue, MIN_LIGHT_SENSOR_CALIBRATION, MAX_LIGHT_SENSOR_CALIBRATION);
}

void saveEEPROM(Pump* pumps, uint8_t pumpsCount, SensorConfig sensorConfig)
{
  EEPROM_MEM mem;
  EEPROM.readBlock(0, mem);

  for (uint8_t i = 0; i < pumpsCount; i++)
  {
    Pump *pump = &pumps[i];
    mem.pumpConfigs[i] = pump->getConfig();
    pump->setLastRunMs(0ul);
  }
  mem.sensorConfig = sensorConfig;

  EEPROM.updateBlock(0, mem);
}

int filterNoise(int lastMeasure, int newMeasure)
{
  if (lastMeasure == 0)
  {
    return newMeasure;
  }
  else
  {
    return (ALPHA * newMeasure + (ALPHA_SCALE - ALPHA) * lastMeasure) / ALPHA_SCALE;
  }
}