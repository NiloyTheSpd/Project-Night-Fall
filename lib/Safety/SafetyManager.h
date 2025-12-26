#ifndef SAFETY_MANAGER_H
#define SAFETY_MANAGER_H

#include <Arduino.h>
#include "config.h"

enum HazardType {
    HAZARD_NONE,
    HAZARD_GAS,
    HAZARD_OBSTACLE_CRITICAL, // Too close to move
    HAZARD_CONNECTION_LOST    // Future usage
};

class SafetyManager
{
public:
    SafetyManager();

    // Main check method
    // Returns true if SAFE, false if HAZARD
    bool check(int gasLevel, float frontDist = 100.0f);

    bool isEmergency() const;
    HazardType getHazardType() const;
    String getHazardDescription() const;

    void reset();

private:
    bool _emergencyActive;
    HazardType _currentHazard;
    String _hazardDesc;
};

#endif
