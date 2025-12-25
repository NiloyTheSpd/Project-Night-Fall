// lib/Sensors/GasSensor.cpp
#include "GasSensor.h"

GasSensor::GasSensor(uint8_t analogPin, uint8_t digitalPin)
    : _analogPin(analogPin), _digitalPin(digitalPin),
      _baseline(0), _currentValue(0), _detected(false),
      _lastUpdate(0), _readIndex(0), _total(0)
{

    // Initialize filter array
    for (int i = 0; i < FILTER_SIZE; i++)
    {
        _readings[i] = 0;
    }
}

void GasSensor::begin()
{
    pinMode(_analogPin, INPUT);
    pinMode(_digitalPin, INPUT);

    DEBUG_PRINTLN("Gas sensor initializing...");
    DEBUG_PRINTLN("Pre-heating sensor (60 seconds recommended)");

    // Initial warm-up delay
    delay(5000);

    // Establish baseline
    calibrate();

    DEBUG_PRINTLN("Gas sensor ready");
}

void GasSensor::update()
{
    // Update at specified interval
    if (millis() - _lastUpdate < GAS_SAMPLE_INTERVAL)
    {
        return;
    }

    _lastUpdate = millis();

    // Read filtered analog value
    _currentValue = getFilteredReading();

    // Check digital output
    int digitalValue = digitalRead(_digitalPin);

    // Determine detection (both analog threshold and digital signal)
    _detected = (_currentValue > GAS_THRESHOLD_ANALOG) || (digitalValue == HIGH);

    if (_detected)
    {
        DEBUG_PRINT("Gas detected! Value: ");
        DEBUG_PRINTLN(_currentValue);
    }
}

bool GasSensor::isDetected()
{
    return _detected;
}

int GasSensor::getAnalogValue()
{
    return _currentValue;
}

float GasSensor::getPPM()
{
    // Simplified PPM calculation
    // For accurate PPM, sensor-specific calibration curve is needed
    // This is an approximation for MQ-2

    float sensorVoltage = (_currentValue / 4095.0) * 3.3; // ESP32 ADC
    float RS_air = 10000.0;                               // Resistance in clean air (approximate)
    float R0 = RS_air / 9.8;                              // Rs/R0 ratio in clean air for MQ-2

    float RS_gas = ((3.3 * 10000.0) / sensorVoltage) - 10000.0;
    float ratio = RS_gas / R0;

    // Approximate PPM calculation (logarithmic relationship)
    float ppm = pow(10, ((log10(ratio) - 0.42) / -0.46));

    return ppm;
}

void GasSensor::calibrate()
{
    DEBUG_PRINTLN("Calibrating gas sensor in clean air...");

    long sum = 0;
    int samples = 50;

    for (int i = 0; i < samples; i++)
    {
        sum += analogRead(_analogPin);
        delay(100);
    }

    _baseline = sum / samples;

    DEBUG_PRINT("Baseline established: ");
    DEBUG_PRINTLN(_baseline);
    DEBUG_PRINT("Detection threshold: ");
    DEBUG_PRINTLN(_baseline + 100);
}

int GasSensor::getFilteredReading()
{
    // Subtract the last reading
    _total -= _readings[_readIndex];

    // Read new value
    _readings[_readIndex] = analogRead(_analogPin);

    // Add to total
    _total += _readings[_readIndex];

    // Advance to next position
    _readIndex = (_readIndex + 1) % FILTER_SIZE;

    // Calculate average
    return _total / FILTER_SIZE;
}