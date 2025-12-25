// lib/Navigation/AutonomousNav.h
#ifndef AUTONOMOUS_NAV_H
#define AUTONOMOUS_NAV_H

#include <Arduino.h>
#include "config.h"
#include "ControlUtils.h"

enum NavigationState
{
    NAV_FORWARD,
    NAV_AVOIDING,
    NAV_TURNING_LEFT,
    NAV_TURNING_RIGHT,
    NAV_BACKING_UP,
    NAV_CLIMBING,
    NAV_STUCK,
    NAV_SCANNING
};

class AutonomousNav
{
public:
    AutonomousNav();

    void begin();
    void updateSensorData(float frontDistance, float rearDistance);
    MovementCommand getNextMove();
    NavigationState getCurrentState();
    void reset();
    // Optional: expose computed TTC for telemetry
    float getTtcMs() const { return _ttcMs; }
    ControlMode getControlMode() const { return _controlMode; }

private:
    NavigationState _currentState;
    NavigationState _previousState;

    ControlMode _controlMode;

    float _frontDistance;
    float _rearDistance;
    float _lastFrontDistance;
    unsigned long _lastFrontUpdateTime;
    float _ttcMs; // Time-to-collision estimate in ms (optional)

    unsigned long _stateStartTime;
    unsigned long _lastDecisionTime;
    int _stuckCounter;
    int _turnDirection; // -1 = left, 1 = right

    // Hysteresis for obstacle detection
    float _obstacleThreshold;
    float _clearanceThreshold;
    bool _obstacleState;

    // Navigation methods
    MovementCommand handleForward();
    MovementCommand handleAvoiding();
    MovementCommand handleTurning();
    MovementCommand handleBackingUp();
    MovementCommand handleClimbing();
    MovementCommand handleStuck();
    MovementCommand handleScanning();

    // Helper methods
    bool isObstacleDetected();
    bool isClimbableObstacle();
    bool isStuck();
    void changeState(NavigationState newState);
    int chooseBestTurnDirection();

    // Advanced helpers (scaffolding)
    float estimateApproachSpeedCmS(unsigned long now);
    MovementCommand microScanWiggle();
    bool turnUntilClear(unsigned long timeoutMs);
};

#endif // AUTONOMOUS_NAV_H
