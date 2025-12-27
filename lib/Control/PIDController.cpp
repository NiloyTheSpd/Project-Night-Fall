#include "PIDController.h"

PIDController::PIDController(float kP, float kI, float kD)
    : _kP(kP), _kI(kI), _kD(kD),
      _setpoint(0),
      _outputMin(-255), _outputMax(255),
      _integralMin(-255), _integralMax(255),
      _integral(0), _lastError(0), _lastInput(0), _lastOutput(0),
      _lastTime(0), _firstRun(true),
      _lastP(0), _lastD(0)
{
}

// ============================================
// CONFIGURATION
// ============================================

void PIDController::setTunings(float kP, float kI, float kD)
{
    _kP = kP;
    _kI = kI;
    _kD = kD;
}

void PIDController::setSetpoint(float setpoint)
{
    _setpoint = setpoint;
}

void PIDController::setOutputLimits(float minOutput, float maxOutput)
{
    if (minOutput > maxOutput) return;
    _outputMin = minOutput;
    _outputMax = maxOutput;
    
    // Also update integral limits if not explicitly set
    _integralMin = minOutput;
    _integralMax = maxOutput;
}

void PIDController::setIntegralLimits(float minIntegral, float maxIntegral)
{
    if (minIntegral > maxIntegral) return;
    _integralMin = minIntegral;
    _integralMax = maxIntegral;
}

// ============================================
// COMPUTATION
// ============================================

float PIDController::compute(float input)
{
    unsigned long now = millis();
    
    if (_firstRun)
    {
        _lastInput = input;
        _lastTime = now;
        _firstRun = false;
        return 0;
    }
    
    // Calculate time delta in seconds
    float dt = (now - _lastTime) / 1000.0f;
    _lastTime = now;
    
    // Minimum dt to avoid division issues
    if (dt <= 0) dt = 0.001f;
    
    return computeWithDt(input, dt);
}

float PIDController::computeWithDt(float input, float dt)
{
    // P1 Fix #8: Enhanced dt validation
    // Clamp dt to reasonable range (5Hz to 1000Hz)
    if (dt < 0.001f || dt > 0.2f) {
        // dt out of range - likely system issue (WiFi freeze, etc.)
        #ifdef DEBUG
        DEBUG_PRINTF("[PID] WARNING: dt=%.3f out of range, using fallback\n", dt);
        #endif
        dt = 0.05f;  // Assume 20Hz
        reset();  // Clear integral to avoid bad state
    }
    
    // Calculate error
    float error = _setpoint - input;
    
    // ========================================
    // PROPORTIONAL
    // ========================================
    float P = _kP * error;
    _lastP = P;
    
    // ========================================
    // INTEGRAL with anti-windup
    // ========================================
    _integral += error * dt;
    
    // Anti-windup: clamp integral to prevent excessive accumulation
    _integral = constrain(_integral, _integralMin / max(_kI, 0.001f), _integralMax / max(_kI, 0.001f));
    
    float I = _kI * _integral;
    
    // ========================================
    // DERIVATIVE (on measurement, not error)
    // ========================================
    // Using derivative on measurement prevents "derivative kick" when setpoint changes
    float dInput = (input - _lastInput) / dt;
    float D = -_kD * dInput;  // Negative because we want to resist change
    _lastD = D;
    _lastInput = input;
    
    // ========================================
    // COMBINE AND CLAMP OUTPUT
    // ========================================
    float output = P + I + D;
    output = constrain(output, _outputMin, _outputMax);
    
    _lastError = error;
    _lastOutput = output;
    
    return output;
}

void PIDController::reset()
{
    _integral = 0;
    _lastError = 0;
    _lastInput = 0;
    _lastOutput = 0;
    _lastP = 0;
    _lastD = 0;
    _firstRun = true;
}

// ============================================
// STATUS
// ============================================

bool PIDController::atSetpoint(float tolerance) const
{
    return abs(_lastError) <= tolerance;
}
