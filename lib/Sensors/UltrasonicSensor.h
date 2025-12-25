// lib/Sensors/UltrasonicSensor.h
#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include <Arduino.h>
#include "config.h"

class UltrasonicSensor
{
public:
    struct SensorHealth
    {
        int totalReadings;
        int validReadings;
        int invalidReadings;
        float availabilityPercent;
        unsigned long lastReadTime;
        bool isHealthy;
    };

    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin);

    void begin();
    float getDistance(); // Returns distance in cm
    bool isObstacleDetected(float threshold = SAFE_DISTANCE);
    float getAverageDistance(uint8_t samples = 5);
    float getSmoothedDistance(); // EMA filtered distance
    SensorHealth getHealthStatus();

private:
    uint8_t _trigPin;
    uint8_t _echoPin;
    float _lastDistance;
    float _filteredDistance;
    unsigned long _lastReading;
    int _totalReadings;
    int _validReadings;
    int _invalidReadings;
    static constexpr float EMA_ALPHA = 0.3f; // 30% new value, 70% previous

    float measureDistance();
};

#endif // ULTRASONIC_SENSOR_H
