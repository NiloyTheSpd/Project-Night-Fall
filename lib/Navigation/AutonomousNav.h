// lib/Navigation/AutonomousNav.h
#ifndef AUTONOMOUS_NAV_H
#define AUTONOMOUS_NAV_H

#include <Arduino.h>
#include "config.h"

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

private:
    NavigationState _currentState;
    NavigationState _previousState;

    float _frontDistance;
    float _rearDistance;
    float _lastFrontDistance;

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
};

#endif // AUTONOMOUS_NAV_H
