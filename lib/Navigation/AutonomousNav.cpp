// lib/Navigation/AutonomousNav.cpp
#include "AutonomousNav.h"

AutonomousNav::AutonomousNav()
    : _currentState(NAV_FORWARD), _previousState(NAV_FORWARD),
      _controlMode(CONTROL_CRUISE),
      _frontDistance(0), _rearDistance(0), _lastFrontDistance(0),
      _lastFrontUpdateTime(0), _ttcMs(-1.0f),
      _stateStartTime(0), _lastDecisionTime(0),
      _stuckCounter(0), _turnDirection(1),
      _obstacleThreshold(OBSTACLE_THRESHOLD),
      _clearanceThreshold(OBSTACLE_THRESHOLD + 10.0f),
      _obstacleState(false) {}

void AutonomousNav::begin()
{
    _currentState = NAV_FORWARD;
    _controlMode = CONTROL_CRUISE;
    _stateStartTime = millis();
    _lastDecisionTime = millis();
    _lastFrontUpdateTime = _lastDecisionTime;
    _ttcMs = -1.0f;

    DEBUG_PRINTLN("Autonomous Navigation initialized");
}

void AutonomousNav::updateSensorData(float frontDistance, float rearDistance)
{
    _lastFrontDistance = _frontDistance;
    _frontDistance = frontDistance;
    _rearDistance = rearDistance;
    _lastFrontUpdateTime = millis();
}

MovementCommand AutonomousNav::getNextMove()
{
    // Limit decision rate to prevent rapid state changes
    if (millis() - _lastDecisionTime < 100)
    {
        return CMD_FORWARD; // Continue current action
    }

    _lastDecisionTime = millis();

    // State machine
    MovementCommand cmd;

    switch (_currentState)
    {
    case NAV_FORWARD:
        cmd = handleForward();
        break;

    case NAV_AVOIDING:
        cmd = handleAvoiding();
        break;

    case NAV_TURNING_LEFT:
    case NAV_TURNING_RIGHT:
        cmd = handleTurning();
        break;

    case NAV_BACKING_UP:
        cmd = handleBackingUp();
        break;

    case NAV_CLIMBING:
        cmd = handleClimbing();
        break;

    case NAV_STUCK:
        cmd = handleStuck();
        break;

    case NAV_SCANNING:
        cmd = handleScanning();
        break;

    default:
        cmd = CMD_STOP;
        break;
    }

    return cmd;
}

MovementCommand AutonomousNav::handleForward()
{
    _controlMode = CONTROL_CRUISE;

    // Optional TTC braking scaffolding (disabled by default)
#if ENABLE_TTC_BRAKING
    // Estimate approach speed from distance trend
    float approachSpeed = estimateApproachSpeedCmS(millis());
    _ttcMs = ControlUtils::computeTTC(_frontDistance, approachSpeed);
    if (_ttcMs > 0.0f && _ttcMs < TTC_BRAKE_THRESHOLD_MS)
    {
        _controlMode = CONTROL_APPROACH;
        // Future: reduce speed via PID/ramp; for now, stop briefly
        return CMD_STOP;
    }
#else
    _ttcMs = -1.0f;
#endif

    // Check for obstacles
    if (isObstacleDetected())
    {
        // Check if it's a climbable obstacle
        if (isClimbableObstacle())
        {
            changeState(NAV_CLIMBING);
            return CMD_CLIMB_BOOST;
        }
        else
        {
            changeState(NAV_AVOIDING);
            return CMD_STOP;
        }
    }

    // Check if stuck (no progress)
    if (isStuck())
    {
        changeState(NAV_STUCK);
        return CMD_STOP;
    }

    // Continue forward
    return CMD_FORWARD;
}

MovementCommand AutonomousNav::handleAvoiding()
{
    _controlMode = CONTROL_ESCAPE;
    // Stop momentarily
    if (millis() - _stateStartTime < 500)
    {
        return CMD_STOP;
    }

    // If path is clear, return to forward
    if (!isObstacleDetected())
    {
        changeState(NAV_FORWARD);
        return CMD_FORWARD;
    }

    // Choose best turn direction
    _turnDirection = chooseBestTurnDirection();

    if (_turnDirection < 0)
    {
        changeState(NAV_TURNING_LEFT);
        return CMD_TURN_LEFT;
    }
    else
    {
        changeState(NAV_TURNING_RIGHT);
        return CMD_TURN_RIGHT;
    }
}

MovementCommand AutonomousNav::handleTurning()
{
    // Turn for specified duration
    if (millis() - _stateStartTime < TURN_DURATION)
    {
        if (_currentState == NAV_TURNING_LEFT)
        {
            return CMD_TURN_LEFT;
        }
        else
        {
            return CMD_TURN_RIGHT;
        }
    }

    // Check if path is now clear
    if (!isObstacleDetected())
    {
        changeState(NAV_FORWARD);
        return CMD_FORWARD;
    }
    else
    {
        // Still blocked, continue turning or back up
        if (millis() - _stateStartTime > TURN_DURATION * 3)
        {
            changeState(NAV_BACKING_UP);
            return CMD_BACKWARD;
        }
    }

    // Continue turning
    if (_currentState == NAV_TURNING_LEFT)
    {
        return CMD_TURN_LEFT;
    }
    else
    {
        return CMD_TURN_RIGHT;
    }
}

MovementCommand AutonomousNav::handleBackingUp()
{
    _controlMode = CONTROL_ESCAPE;
    // Back up for specified duration
    if (millis() - _stateStartTime < BACKUP_DURATION)
    {
        return CMD_BACKWARD;
    }

    // After backing up, try turning again
    changeState(NAV_SCANNING);
    return CMD_STOP;
}

MovementCommand AutonomousNav::handleClimbing()
{
    _controlMode = CONTROL_APPROACH;
    // Boost front motors for climbing duration
    if (millis() - _stateStartTime < CLIMB_BOOST_DURATION)
    {
        return CMD_CLIMB_BOOST;
    }

    // After climb, return to normal forward
    changeState(NAV_FORWARD);
    return CMD_FORWARD;
}

MovementCommand AutonomousNav::handleStuck()
{
    _controlMode = CONTROL_ESCAPE;
    // Perform 360-degree scan
    changeState(NAV_SCANNING);
    return CMD_ROTATE_360;
}

MovementCommand AutonomousNav::handleScanning()
{
    // After 360 scan, choose best direction
    if (millis() - _stateStartTime < ROTATION_360_DURATION)
    {
        return CMD_ROTATE_360;
    }

    // Reset stuck counter
    _stuckCounter = 0;

    // Return to forward motion
    changeState(NAV_FORWARD);
    return CMD_FORWARD;
}

bool AutonomousNav::isObstacleDetected()
{
    // Hysteresis prevents oscillation at threshold
    // Obstacle detected at 30cm, but must exceed 40cm to clear
    if (!_obstacleState && _frontDistance > 0 && _frontDistance < _obstacleThreshold)
    {
        _obstacleState = true; // Obstacle detected
        DEBUG_PRINT("[NAV] Obstacle detected at ");
        DEBUG_PRINT(_frontDistance);
        DEBUG_PRINTLN("cm");
        return true;
    }
    else if (_obstacleState && _frontDistance > _clearanceThreshold)
    {
        _obstacleState = false; // Path cleared
        DEBUG_PRINT("[NAV] Path cleared at ");
        DEBUG_PRINT(_frontDistance);
        DEBUG_PRINTLN("cm");
        return false;
    }

    return _obstacleState; // Maintain current state in deadzone (30-40cm)
}

bool AutonomousNav::isClimbableObstacle()
{
    // Detect sudden distance drop indicating step/obstacle
    float distanceChange = _lastFrontDistance - _frontDistance;

    // If distance suddenly decreased significantly, it might be climbable
    if (distanceChange > CLIMB_DETECT_THRESHOLD &&
        _frontDistance < 15 &&
        _frontDistance > 3)
    {
        DEBUG_PRINTLN("Climbable obstacle detected");
        return true;
    }

    return false;
}

bool AutonomousNav::isStuck()
{
    // If in forward state but distance not changing significantly
    if (_currentState == NAV_FORWARD)
    {
        float distanceChange = abs(_frontDistance - _lastFrontDistance);

        if (distanceChange < 1.0 && _frontDistance < SAFE_DISTANCE)
        {
            _stuckCounter++;
        }
        else
        {
            _stuckCounter = 0;
        }

        if (_stuckCounter > STUCK_THRESHOLD)
        {
            DEBUG_PRINTLN("Robot appears stuck!");
            return true;
        }
    }

    return false;
}

void AutonomousNav::changeState(NavigationState newState)
{
    if (_currentState != newState)
    {
        _previousState = _currentState;
        _currentState = newState;
        _stateStartTime = millis();

        DEBUG_PRINT("Navigation state changed to: ");
        DEBUG_PRINTLN(newState);
    }
}

int AutonomousNav::chooseBestTurnDirection()
{
    // Simple strategy: alternate turns if no clear preference
    // In advanced implementation, could use camera or additional sensors

    // Check rear distance for backing up option
    if (_rearDistance > 50)
    {
        // Plenty of room to back up, alternate turns
        _turnDirection *= -1; // Flip direction
    }
    else
    {
        // Limited rear space, prefer right turn (arbitrary choice)
        _turnDirection = 1;
    }

    return _turnDirection;
}

NavigationState AutonomousNav::getCurrentState()
{
    return _currentState;
}

void AutonomousNav::reset()
{
    _currentState = NAV_FORWARD;
    _previousState = NAV_FORWARD;
    _controlMode = CONTROL_CRUISE;
    _stuckCounter = 0;
    _turnDirection = 1;
    _stateStartTime = millis();
    _lastFrontUpdateTime = _stateStartTime;
    _ttcMs = -1.0f;
}

// === Advanced helpers (scaffolding) ===
float AutonomousNav::estimateApproachSpeedCmS(unsigned long now)
{
    // Estimate speed from distance delta over decision interval
    unsigned long dt_ms = now - _lastDecisionTime;
    if (dt_ms == 0)
        return 0.0f;

    float delta_cm = _lastFrontDistance - _frontDistance; // positive when approaching
    float speed_cm_s = (delta_cm) * (1000.0f / (float)dt_ms);
    if (speed_cm_s < 0.0f)
        return 0.0f; // moving away or stationary
    return speed_cm_s;
}

MovementCommand AutonomousNav::microScanWiggle()
{
    // Placeholder: small left-right wiggle to probe environment
    // For now, just stop to avoid unintended motion
    return CMD_STOP;
}

bool AutonomousNav::turnUntilClear(unsigned long timeoutMs)
{
    // Placeholder: keep turning until obstacle clears or timeout
    // Not implemented yet; always return false to indicate not cleared
    (void)timeoutMs;
    return false;
}