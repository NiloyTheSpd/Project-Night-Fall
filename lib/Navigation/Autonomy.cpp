#include "Autonomy.h"

Autonomy::Autonomy() 
    : _frontDistance(0), _rearDistance(0), _leftSpeed(0), _rightSpeed(0), _navState(NAV_IDLE)
{
}

void Autonomy::update(float frontDistance, float rearDistance)
{
    _frontDistance = frontDistance;
    _rearDistance = rearDistance;
    updateLogic();
}

void Autonomy::updateLogic()
{
    // Simple Obstacle Avoidance Logic (migrated from main_rear.cpp)
    
    bool frontObstacle = (_frontDistance > 0 && _frontDistance < ULTRASONIC_THRESHOLD_OBSTACLE);

    if (frontObstacle)
    {
        // Stop
        _leftSpeed = 0;
        _rightSpeed = 0;
        _navState = NAV_OBSTACLE_DETECTED;
    }
    else
    {
        // Forward
        _leftSpeed = MOTOR_NORMAL_SPEED;
        _rightSpeed = MOTOR_NORMAL_SPEED;
        _navState = NAV_FORWARD;
    }
}

int Autonomy::getLeftSpeed() const
{
    return _leftSpeed;
}

int Autonomy::getRightSpeed() const
{
    return _rightSpeed;
}

NavigationState Autonomy::getNavState() const
{
    return _navState;
}

void Autonomy::reset()
{
    _leftSpeed = 0;
    _rightSpeed = 0;
    _navState = NAV_IDLE;
}
