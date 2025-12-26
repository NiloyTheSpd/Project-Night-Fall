#include "SensorManager.h"

SensorManager::SensorManager()
    : _frontSensor(ULTRASONIC_FRONT_TRIG, ULTRASONIC_FRONT_ECHO),
      _rearSensor(ULTRASONIC_REAR_TRIG, ULTRASONIC_REAR_ECHO),
      _gasSensor(GAS_SENSOR_ANALOG, GAS_SENSOR_DIGITAL),
      _frontDist(0), _rearDist(0), _gasLevel(0), _lastUpdate(0)
{
}

void SensorManager::begin()
{
    _frontSensor.begin();
    _rearSensor.begin();
    _gasSensor.begin();
}

void SensorManager::update()
{
    unsigned long now = millis();
    if (now - _lastUpdate >= SENSOR_UPDATE_INTERVAL_MS)
    {
        _lastUpdate = now;

        // Trigger updates
        _frontSensor.update();
        _rearSensor.update();
        _gasSensor.update();

        // Cache values
        _frontDist = _frontSensor.getDistance();
        _rearDist = _rearSensor.getDistance();
        _gasLevel = _gasSensor.getSmoothedReading();

        // Basic Filtering (e.g. clamp negative or invalid)
        if (_frontDist < 0) _frontDist = 0;
        if (_rearDist < 0) _rearDist = 0;
    }
}

float SensorManager::getFrontDistance() const
{
    return _frontDist;
}

float SensorManager::getRearDistance() const
{
    return _rearDist;
}

int SensorManager::getGasLevel() const
{
    return _gasLevel;
}
