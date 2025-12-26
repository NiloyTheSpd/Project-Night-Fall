#include "SafetyManager.h"

SafetyManager::SafetyManager() 
    : _emergencyActive(false), _currentHazard(HAZARD_NONE), _hazardDesc("OK")
{
}

bool SafetyManager::check(int gasLevel, float frontDist)
{
    // If already in emergency, stay there unless reset manually (latched safety)
    if (_emergencyActive) return false;

    // Check 1: Gas
    if (gasLevel >= GAS_THRESHOLD_EMERGENCY) {
        _emergencyActive = true;
        _currentHazard = HAZARD_GAS;
        _hazardDesc = "GAS DETECTED - EMERGENCY STOP";
        return false;
    }

    // Check 2: Critical Distance (Optional - prevent crashing)
    // if (frontDist < 5.0 && frontDist > 0) { ... } 

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
