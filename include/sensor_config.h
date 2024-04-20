#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <Arduino.h>

#define MIN_SOIL_SENSOR_CALIBRATION 0
#define MAX_SOIL_SENSOR_CALIBRATION 1023

#define MIN_LIGHT_SENSOR_CALIBRATION 0
#define MAX_LIGHT_SENSOR_CALIBRATION 1023

struct SensorConfig
{
    int lightSensorDayValue;
    int lightSensorNightValue;
    int soilSensorDryValue[3];
    int soilSensorWetValue[3];
};

#endif /* SENSOR_CONFIG_H */
