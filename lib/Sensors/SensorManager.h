#ifndef SENSOR_MANAGER_H
#define SENSOR_MANAGER_H

#include <Arduino.h>
#include "config.h"
#include "pins.h"
#include "UltrasonicSensor.h"
#include "MQ2Sensor.h"

class SensorManager
{
public:
    SensorManager();

    void begin();
    void update(); // Non-blocking update loop

    // Thread-safe accessors (returns cached values)
    float getFrontDistance() const;
    float getRearDistance() const;
    int getGasLevel() const;

private:
    // Sensor Objects
    UltrasonicSensor _frontSensor;
    UltrasonicSensor _rearSensor;
    MQ2Sensor _gasSensor;

    // Cached Data
    float _frontDist;
    float _rearDist;
    int _gasLevel;

    // Timing & Stagger
    unsigned long _lastUpdate;
    bool _readFrontNext;  // Alternates between front/rear to prevent crosstalk
};

#endif
