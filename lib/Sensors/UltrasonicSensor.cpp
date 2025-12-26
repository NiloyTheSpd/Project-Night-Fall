#include "UltrasonicSensor.h"

UltrasonicSensor::UltrasonicSensor(uint8_t trigPin, uint8_t echoPin)
    : _trigPin(trigPin), _echoPin(echoPin),
      _lastDistance(0), _smoothedDistance(0),
      _lastMeasureTime(0), _pulseStart(0),
      _state(IDLE)
{
}

void UltrasonicSensor::begin()
{
    pinMode(_trigPin, OUTPUT);
    pinMode(_echoPin, INPUT);
    digitalWrite(_trigPin, LOW);
}

float UltrasonicSensor::getDistance()
{
    return _lastDistance;
}

void UltrasonicSensor::update()
{
    unsigned long now = micros();

    switch (_state)
    {
    case IDLE:
        // Time to measure?
        if (millis() - _lastMeasureTime >= MEASURE_INTERVAL_MS)
        {
            // Send trigger pulse
            digitalWrite(_trigPin, HIGH);
            delayMicroseconds(10);
            digitalWrite(_trigPin, LOW);
            _pulseStart = micros();
            _state = WAITING_FOR_ECHO;
            _lastMeasureTime = millis();
        }
        break;

    case WAITING_FOR_ECHO:
        // Wait for echo to go HIGH
        if (digitalRead(_echoPin) == HIGH)
        {
            _pulseStart = micros();
            _state = MEASURING;
        }
        else if (now - _pulseStart > ECHO_TIMEOUT_US)
        {
            // Timeout
            _lastDistance = -1;
            _state = IDLE;
        }
        break;

    case MEASURING:
        // Wait for echo to go LOW
        if (digitalRead(_echoPin) == LOW)
        {
            unsigned long pulseDuration = micros() - _pulseStart;

            // Check timeout
            if (pulseDuration > ECHO_TIMEOUT_US)
            {
                _lastDistance = -1;
            }
            else
            {
                // Convert to cm (speed of sound = 343 m/s = 0.0343 cm/us)
                float distance = (pulseDuration / 2.0f) * 0.0343f;

                // Validate
                if (distance > 2.0f && distance < 400.0f)
                {
                    _lastDistance = distance;
                    // Apply EMA filter
                    _smoothedDistance = EMA_ALPHA * distance + (1.0f - EMA_ALPHA) * _smoothedDistance;
                }
                else
                {
                    _lastDistance = -1;
                }
            }
            _state = IDLE;
        }
        else if (now - _pulseStart > ECHO_TIMEOUT_US)
        {
            _lastDistance = -1;
            _state = IDLE;
        }
        break;
    }
}

bool UltrasonicSensor::isObstacleDetected(float thresholdCm)
{
    return (_lastDistance > 0) && (_lastDistance < thresholdCm);
}

void UltrasonicSensor::reset()
{
    _lastDistance = 0;
    _smoothedDistance = 0;
    _state = IDLE;
}