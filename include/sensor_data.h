#ifndef SENSOR_DATA_H
#define SENSOR_DATA_H

#include <Arduino.h>

struct SensorData
{
    int8_t temperature;
    uint8_t humidity;
    uint8_t light;
    uint8_t soilMoisture[3];
};

#endif /* SENSOR_DATA_H */