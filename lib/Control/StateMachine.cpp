#include "StateMachine.h"

StateMachine::StateMachine() 
    : _currentState(STATE_INIT), _emergencyLatched(false)
{
    // Start in IDLE after init
    _currentState = STATE_IDLE;
}

void StateMachine::transitionTo(RobotState newState)
{
    // Block transitions if in emergency (unless clearing)
    if (_emergencyLatched && newState != STATE_IDLE && newState != STATE_EMERGENCY) {
        return;
    }
    
    _currentState = newState;
}

void StateMachine::setIdle()
{
    transitionTo(STATE_IDLE);
}

void StateMachine::setManual()
{
    transitionTo(STATE_MANUAL);
}

void StateMachine::setAutonomous()
{
    transitionTo(STATE_AUTONOMOUS);
}

void StateMachine::triggerEmergency()
{
    _emergencyLatched = true;
    _currentState = STATE_EMERGENCY;
}

void StateMachine::clearEmergency()
{
    // Only way to exit emergency
    _emergencyLatched = false;
    _currentState = STATE_IDLE;
}

RobotState StateMachine::getState() const
{
    return _currentState;
}

bool StateMachine::isAutonomous() const
{
    return _currentState == STATE_AUTONOMOUS;
}

bool StateMachine::isManual() const
{
    return _currentState == STATE_MANUAL;
}

bool StateMachine::isEmergency() const
{
    return _currentState == STATE_EMERGENCY;
}

bool StateMachine::isIdle() const
{
    return _currentState == STATE_IDLE;
}

String StateMachine::getStateName() const
{
    switch (_currentState)
    {
        case STATE_INIT: return "INIT";
        case STATE_IDLE: return "IDLE";
        case STATE_AUTONOMOUS: return "AUTONOMOUS";
        case STATE_MANUAL: return "MANUAL";
        case STATE_EMERGENCY: return "EMERGENCY";
        case STATE_ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}
