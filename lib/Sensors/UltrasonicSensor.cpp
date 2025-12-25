// lib/Sensors/UltrasonicSensor.cpp
#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin)
    : _trigPin(trigPin), _echoPin(echoPin), _lastDistance(0), _lastReading(0) {}

void UltrasonicSensor::begin()
{
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    digitalWrite(_trigPin, LOW);

    DEBUG_PRINT("Ultrasonic sensor initialized: TRIG=");
    DEBUG_PRINT(_trigPin);
    DEBUG_PRINT(", ECHO=");
    DEBUG_PRINTLN(_echoPin);
}

float UltrasonicSensor::getDistance()
{
    // Prevent too frequent readings (minimum 60ms between readings)
    if (millis() - _lastReading < 60)
    {
        return _lastDistance;
    }

    _lastDistance = measureDistance();
    _lastReading = millis();

    return _lastDistance;
}

float UltrasonicSensor::measureDistance()
{
    // Send trigger pulse
    digitalWrite(_trigPin, LOW);
    delayMicroseconds(2);
    digitalWrite(_trigPin, HIGH);
    delayMicroseconds(10);
    digitalWrite(_trigPin, LOW);

    // Measure echo pulse duration
    long duration = pulseIn(_echoPin, HIGH, ULTRASONIC_TIMEOUT);

    // Calculate distance (speed of sound = 343 m/s = 0.0343 cm/μs)
    // Distance = (duration * speed) / 2 (round trip)
    float distance = duration * 0.0343 / 2.0;

    // Validate reading (HC-SR04 effective range: 2-400cm)
    if (distance < 2.0 || distance > 400.0)
    {
        return _lastDistance; // Return last valid reading if out of range
    }

    return distance;
}

bool UltrasonicSensor::isObstacleDetected(float threshold)
{
    float dist = getDistance();
    return (dist > 0 && dist < threshold);
}

float UltrasonicSensor::getAverageDistance(uint8_t samples)
{
    float sum = 0;
    uint8_t validSamples = 0;

    for (uint8_t i = 0; i < samples; i++)
    {
        float dist = measureDistance();

        if (dist >= 2.0 && dist <= 400.0)
        {
            sum += dist;
            validSamples++;
        }

        delay(60); // Wait between readings
    }

    if (validSamples > 0)
    {
        return sum / validSamples;
    }

    return _lastDistance;
}