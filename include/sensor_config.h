#ifndef SENSOR_CONFIG_H
#define SENSOR_CONFIG_H

#include <Arduino.h>

struct SensorConfig
{
    int lightSensorDayValue;
    int lightSensorNightValue;
    int soilSensorDryValue[3];
    int soilSensorWetValue[3];
};

#endif /* SENSOR_CONFIG_H */
