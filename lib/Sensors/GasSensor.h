// lib/Sensors/GasSensor.h
#ifndef GAS_SENSOR_H
#define GAS_SENSOR_H

#include <Arduino.h>
#include "config.h"

class GasSensor
{
public:
    GasSensor(uint8_t analogPin, uint8_t digitalPin);

    void begin();
    void update();
    bool isDetected();
    int getAnalogValue();
    float getPPM();
    void calibrate();

private:
    uint8_t _analogPin;
    uint8_t _digitalPin;
    int _baseline;
    int _currentValue;
    bool _detected;
    unsigned long _lastUpdate;

    // Moving average filter
    static const int FILTER_SIZE = 10;
    int _readings[FILTER_SIZE];
    int _readIndex;
    int _total;

    int getFilteredReading();
};

#endif // GAS_SENSOR_H
