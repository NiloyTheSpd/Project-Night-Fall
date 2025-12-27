#include "Autonomy.h"

// Timing constants for maneuvers
static const unsigned long TURN_DURATION_MS = 400;    // Time to turn when avoiding obstacle
static const unsigned long BACKUP_DURATION_MS = 300;  // Time to reverse before turn
static const int STUCK_THRESHOLD = 3;                 // Obstacle hits before trying backup

// PID tuning for distance-based approach control
// Setpoint is the safe distance, output is speed adjustment
static const float APPROACH_KP = 4.0f;   // Proportional: higher = more aggressive
static const float APPROACH_KI = 0.0f;   // Integral: usually 0 for distance control
static const float APPROACH_KD = 1.0f;   // Derivative: dampens oscillation

Autonomy::Autonomy() 
    : _frontDistance(0), _rearDistance(0), 
      _leftSpeed(0), _rightSpeed(0), _navState(NAV_IDLE),
      _maneuverStartTime(0), _turnDirection(1), _stuckCounter(0),
      _approachPID(APPROACH_KP, APPROACH_KI, APPROACH_KD), _pidEnabled(true)
{
    // Configure PID for approach control
    // Setpoint: target distance (safe zone)
    // Input: current distance
    // Output: speed (0 to MOTOR_NORMAL_SPEED)
    _approachPID.setSetpoint(ULTRASONIC_THRESHOLD_SAFE);
    _approachPID.setOutputLimits(0, MOTOR_NORMAL_SPEED);
}

void Autonomy::update(float frontDistance, float rearDistance)
{
    _frontDistance = frontDistance;
    _rearDistance = rearDistance;
    updateLogic();
}

void Autonomy::setState(NavigationState newState)
{
    if (_navState != newState)
    {
        _navState = newState;
        _maneuverStartTime = millis();
    }
}

void Autonomy::updateLogic()
{
    unsigned long now = millis();
    unsigned long elapsed = now - _maneuverStartTime;
    
    // Distance checks
    bool frontObstacle = (_frontDistance > 0 && _frontDistance < ULTRASONIC_THRESHOLD_OBSTACLE);
    bool frontClose = (_frontDistance > 0 && _frontDistance < ULTRASONIC_THRESHOLD_SAFE);
    bool rearClear = (_rearDistance <= 0 || _rearDistance > ULTRASONIC_THRESHOLD_OBSTACLE);
    
    switch (_navState)
    {
        // ========================================
        // IDLE - Waiting to start
        // ========================================
        case NAV_IDLE:
            _leftSpeed = 0;
            _rightSpeed = 0;
            // External trigger starts movement (set by main when autonomous enabled)
            setState(NAV_FORWARD);
            break;
            
        // ========================================
        // FORWARD - Normal cruising
        // ========================================
        case NAV_FORWARD:
            if (frontObstacle)
            {
                // Obstacle detected - decide action
                _stuckCounter++;
                
                if (_stuckCounter >= STUCK_THRESHOLD)
                {
                    // Been stuck too many times, try backing up
                    setState(NAV_BACKING_UP);
                }
                else
                {
                    // Normal obstacle - turn to avoid
                    setState(NAV_OBSTACLE_DETECTED);
                }
            }
            else
            {
                // Path clear - drive forward
                if (frontClose)
                {
                    if (_pidEnabled)
                    {
                        // PID approach: smooth speed based on distance to safe zone
                        // Input = current distance, Setpoint = safe distance
                        // Output = speed (clamped 0 to MOTOR_NORMAL_SPEED)
                        float pidSpeed = _approachPID.compute(_frontDistance);
                        int approachSpeed = (int)constrain(pidSpeed, 40, MOTOR_NORMAL_SPEED);
                        _leftSpeed = approachSpeed;
                        _rightSpeed = approachSpeed;
                    }
                    else
                    {
                        // Fallback: simple proportional control (for comparison)
                        float speedFactor = _frontDistance / (float)ULTRASONIC_THRESHOLD_SAFE;
                        speedFactor = constrain(speedFactor, 0.4f, 1.0f);
                        int approachSpeed = (int)(MOTOR_NORMAL_SPEED * speedFactor);
                        _leftSpeed = approachSpeed;
                        _rightSpeed = approachSpeed;
                    }
                }
                else
                {
                    // Full speed ahead - reset PID for next approach
                    _approachPID.reset();
                    _leftSpeed = MOTOR_NORMAL_SPEED;
                    _rightSpeed = MOTOR_NORMAL_SPEED;
                    _stuckCounter = 0;  // Reset stuck counter on clear path
                }
            }
            break;
            
        // ========================================
        // OBSTACLE_DETECTED - Brief pause before turn
        // ========================================
        case NAV_OBSTACLE_DETECTED:
            _leftSpeed = 0;
            _rightSpeed = 0;
            
            // Alternate turn direction each time
            _turnDirection = -_turnDirection;
            
            // Immediately start turning
            if (_turnDirection > 0)
            {
                setState(NAV_AVOID_RIGHT);
            }
            else
            {
                setState(NAV_AVOID_LEFT);
            }
            break;
            
        // ========================================
        // AVOID_LEFT - Turning left to avoid obstacle
        // ========================================
        case NAV_AVOID_LEFT:
            // Spin left: left motor backward, right motor forward
            _leftSpeed = -MOTOR_TURN_SPEED;
            _rightSpeed = MOTOR_TURN_SPEED;
            
            if (elapsed >= TURN_DURATION_MS)
            {
                // Turn complete, try forward again
                setState(NAV_FORWARD);
            }
            break;
            
        // ========================================
        // AVOID_RIGHT - Turning right to avoid obstacle
        // ========================================
        case NAV_AVOID_RIGHT:
            // Spin right: left motor forward, right motor backward
            _leftSpeed = MOTOR_TURN_SPEED;
            _rightSpeed = -MOTOR_TURN_SPEED;
            
            if (elapsed >= TURN_DURATION_MS)
            {
                setState(NAV_FORWARD);
            }
            break;
            
        // ========================================
        // BACKING_UP - Reverse when stuck
        // ========================================
        case NAV_BACKING_UP:
            if (rearClear)
            {
                _leftSpeed = -MOTOR_NORMAL_SPEED / 2;
                _rightSpeed = -MOTOR_NORMAL_SPEED / 2;
                
                if (elapsed >= BACKUP_DURATION_MS)
                {
                    _stuckCounter = 0;  // Reset stuck counter
                    // After backing up, turn to avoid
                    _turnDirection = -_turnDirection;
                    setState(_turnDirection > 0 ? NAV_AVOID_RIGHT : NAV_AVOID_LEFT);
                }
            }
            else
            {
                // Can't back up either - truly stuck
                setState(NAV_STUCK);
            }
            break;
            
        // ========================================
        // STUCK - Cannot move forward or backward
        // ========================================
        case NAV_STUCK:
            _leftSpeed = 0;
            _rightSpeed = 0;
            
            // Try to recover after a moment
            if (elapsed >= 1000)
            {
                _stuckCounter = 0;
                setState(NAV_FORWARD);  // Retry
            }
            break;
            
        // ========================================
        // CLIMBING - Reserved for future use
        // ========================================
        case NAV_CLIMBING:
            // Not implemented yet - future: detect incline and boost torque
            _leftSpeed = MOTOR_CLIMB_SPEED;
            _rightSpeed = MOTOR_CLIMB_SPEED;
            break;
            
        default:
            setState(NAV_IDLE);
            break;
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

const char* Autonomy::getNavStateName() const
{
    switch (_navState)
    {
        case NAV_FORWARD:           return "forward";
        case NAV_OBSTACLE_DETECTED: return "obstacle";
        case NAV_AVOID_LEFT:        return "avoid_left";
        case NAV_AVOID_RIGHT:       return "avoid_right";
        case NAV_BACKING_UP:        return "backing_up";
        case NAV_CLIMBING:          return "climbing";
        case NAV_STUCK:             return "stuck";
        case NAV_IDLE:              return "idle";
        default:                    return "unknown";
    }
}

void Autonomy::reset()
{
    _leftSpeed = 0;
    _rightSpeed = 0;
    _navState = NAV_IDLE;
    _stuckCounter = 0;
    _turnDirection = 1;
    _approachPID.reset();
}

void Autonomy::setApproachPID(float kP, float kI, float kD)
{
    _approachPID.setTunings(kP, kI, kD);
}

void Autonomy::setPIDEnabled(bool enabled)
{
    _pidEnabled = enabled;
    if (!enabled) {
        _approachPID.reset();  // Clear PID state when disabling
    }
}

bool Autonomy::isPIDEnabled() const
{
    return _pidEnabled;
}

// PID status getters for telemetry
float Autonomy::getPIDOutput() const
{
    return _approachPID.getOutput();
}

float Autonomy::getPIDError() const
{
    return _approachPID.getError();
}

float Autonomy::getPIDSetpoint() const
{
    return _approachPID.getSetpoint();
}

float Autonomy::getPIDProportional() const
{
    return _approachPID.getProportional();
}

float Autonomy::getPIDIntegral() const
{
    return _approachPID.getIntegral();
}

float Autonomy::getPIDDerivative() const
{
    return _approachPID.getDerivative();
}
