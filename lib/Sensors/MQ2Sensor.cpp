#include "MQ2Sensor.h"

MQ2Sensor::MQ2Sensor(uint8_t analogPin, uint8_t digitalPin)
    : _analogPin(analogPin), _digitalPin(digitalPin),
      _lastReading(0), _smoothedReading(0),
      _lastUpdateTime(0), _alert(false), _trend(0)
{
}

void MQ2Sensor::begin()
{
    pinMode(_analogPin, INPUT);
    if (_digitalPin != 0xFF)
        pinMode(_digitalPin, INPUT);

    // Initial reading
    _lastReading = analogRead(_analogPin);
    _smoothedReading = _lastReading;
}

int MQ2Sensor::getReading()
{
    return _lastReading;
}

void MQ2Sensor::update()
{
    unsigned long now = millis();

    if (now - _lastUpdateTime >= UPDATE_INTERVAL_MS)
    {
        _lastUpdateTime = now;

        // Read new value
        int reading = analogRead(_analogPin);

        // Track trend
        if (reading > _smoothedReading + 20)
            _trend = 1; // increasing
        else if (reading < _smoothedReading - 20)
            _trend = -1; // decreasing
        else
            _trend = 0; // stable

        _lastReading = reading;

        // Running average
        _smoothedReading = (_smoothedReading * (SAMPLES - 1) + reading) / SAMPLES;
    }
}

bool MQ2Sensor::isGasDetected(int threshold)
{
    bool detected = _smoothedReading > threshold;
    if (detected != _alert)
    {
        _alert = detected;
    }
    return detected;
}

void MQ2Sensor::reset()
{
    _lastReading = 0;
    _smoothedReading = 0;
    _alert = false;
    _trend = 0;
}
