#ifndef AUTONOMY_H
#define AUTONOMY_H

#include <Arduino.h>
#include "config.h"
#include "PIDController.h"

class Autonomy
{
public:
    Autonomy();

    // Inputs
    void update(float frontDistance, float rearDistance);

    // Outputs
    int getLeftSpeed() const;
    int getRightSpeed() const;
    NavigationState getNavState() const;
    
    // Status
    const char* getNavStateName() const;
    
    // Command
    void reset();
    
    // PID tuning (for runtime adjustment)
    void setApproachPID(float kP, float kI, float kD);
    void setPIDEnabled(bool enabled);
    bool isPIDEnabled() const;
    
    // PID status (for telemetry)
    float getPIDOutput() const;
    float getPIDError() const;
    float getPIDSetpoint() const;
    float getPIDProportional() const;
    float getPIDIntegral() const;
    float getPIDDerivative() const;

private:
    float _frontDistance;
    float _rearDistance;
    
    int _leftSpeed;
    int _rightSpeed;
    NavigationState _navState;
    
    // Timing for maneuvers
    unsigned long _maneuverStartTime;
    int _turnDirection;  // -1 = left, +1 = right
    int _stuckCounter;   // Counts consecutive obstacle detections
    
    // PID for smooth distance-based speed control
    PIDController _approachPID;
    bool _pidEnabled;

    void updateLogic();
    void setState(NavigationState newState);
};

#endif
