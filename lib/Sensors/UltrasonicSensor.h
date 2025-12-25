// lib/Sensors/UltrasonicSensor.h
#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "config.h"

class UltrasonicSensor
{
public:
    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin);

    void begin();
    float getDistance(); // Returns distance in cm
    bool isObstacleDetected(float threshold = SAFE_DISTANCE);
    float getAverageDistance(uint8_t samples = 5);

private:
    uint8_t _trigPin;
    uint8_t _echoPin;
    float _lastDistance;
    unsigned long _lastReading;

    float measureDistance();
};

#endif // ULTRASONIC_SENSOR_H
