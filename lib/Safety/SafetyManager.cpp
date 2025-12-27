#include "SafetyManager.h"

SafetyManager::SafetyManager() 
    : _emergencyActive(false), _currentHazard(HAZARD_NONE), _hazardDesc("OK")
{
}

bool SafetyManager::check(int gasLevel, float frontDist)
{
    // If already in emergency, stay there unless reset manually (latched safety)
    if (_emergencyActive) return false;

    // Check 1: Gas/Smoke Detection
    if (gasLevel >= GAS_THRESHOLD_EMERGENCY) {
        _emergencyActive = true;
        _currentHazard = HAZARD_GAS;
        _hazardDesc = "GAS DETECTED - EMERGENCY STOP";
        return false;
    }

    // Check 2: Critical Distance - Collision Imminent
    // P1 Fix #4: Increased to 10cm to account for EMA filter lag
    // At 2m/s approach speed, 100ms filter lag = 20cm positioning error
    // frontDist > 0 ensures we have a valid reading (not timeout/error)
    if (frontDist > 0.0f && frontDist < 10.0f) {
        _emergencyActive = true;
        _currentHazard = HAZARD_OBSTACLE_CRITICAL;
        _hazardDesc = "COLLISION IMMINENT - EMERGENCY STOP";
        return false;
    }

    // All clear
    _currentHazard = HAZARD_NONE;
    _hazardDesc = "OK";
    return true;
}

bool SafetyManager::isEmergency() const
{
    return _emergencyActive;
}

HazardType SafetyManager::getHazardType() const
{
    return _currentHazard;
}

String SafetyManager::getHazardDescription() const
{
    return _hazardDesc;
}

void SafetyManager::reset()
{
    _emergencyActive = false;
    _currentHazard = HAZARD_NONE;
    _hazardDesc = "OK";
}
