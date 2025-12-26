#ifndef STATE_MACHINE_H
#define STATE_MACHINE_H

#include <Arduino.h>
#include "config.h"

class StateMachine
{
public:
    StateMachine();

    // State Transitions (Events)
    void setIdle();
    void setManual();
    void setAutonomous();
    
    void triggerEmergency();
    void clearEmergency(); // Explicit reset required

    // State Queries
    RobotState getState() const;
    bool isAutonomous() const;
    bool isManual() const;
    bool isEmergency() const;
    bool isIdle() const;

    // Helper for telemetry/logging
    String getStateName() const;

private:
    RobotState _currentState;
    bool _emergencyLatched;

    void transitionTo(RobotState newState);
};

#endif
