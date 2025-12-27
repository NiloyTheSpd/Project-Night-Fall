#include "SensorManager.h"

SensorManager::SensorManager()
    : _frontSensor(ULTRASONIC_FRONT_TRIG, ULTRASONIC_FRONT_ECHO),
      _rearSensor(ULTRASONIC_REAR_TRIG, ULTRASONIC_REAR_ECHO),
      _gasSensor(GAS_SENSOR_ANALOG, GAS_SENSOR_DIGITAL),
      _frontDist(0), _rearDist(0), _gasLevel(0), 
      _lastUpdate(0), _readFrontNext(true)
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
    
    // Staggered ultrasonic updates to prevent crosstalk
    // Each sensor updates at half the rate, but they alternate
    // Result: Same effective update rate per sensor, no interference
    const unsigned long STAGGER_INTERVAL = SENSOR_UPDATE_INTERVAL_MS / 2;  // 50ms each
    
    if (now - _lastUpdate >= STAGGER_INTERVAL)
    {
        _lastUpdate = now;

        // Alternate between front and rear ultrasonic
        if (_readFrontNext)
        {
            _frontSensor.update();
            float reading = _frontSensor.getDistance();
            if (reading > 0) {
                _frontDist = reading;
            }
            // else: keep last valid reading
        }
        else
        {
            _rearSensor.update();
            float reading = _rearSensor.getDistance();
            if (reading > 0) {
                _rearDist = reading;
            }
        }
        
        _readFrontNext = !_readFrontNext;  // Toggle for next update
        
        // Gas sensor updates every cycle (no interference with ultrasonics)
        _gasSensor.update();
        _gasLevel = _gasSensor.getSmoothedReading();
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
