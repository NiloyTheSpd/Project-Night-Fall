#ifndef PID_CONTROLLER_H
#define PID_CONTROLLER_H

#include <Arduino.h>

/**
 * PID Controller for Project Nightfall
 * 
 * Generic proportional-integral-derivative controller that can be used for:
 * - Speed control (with encoders)
 * - Heading hold (with IMU)
 * - Distance-based speed regulation
 * - Any closed-loop control scenario
 * 
 * Features:
 * - Anti-windup protection
 * - Configurable output limits
 * - Derivative kick prevention
 * - Time-based or call-based computation
 */
class PIDController
{
public:
    /**
     * Create a PID controller with specified gains
     * @param kP Proportional gain
     * @param kI Integral gain
     * @param kD Derivative gain
     */
    PIDController(float kP = 1.0f, float kI = 0.0f, float kD = 0.0f);

    // ========================================
    // CONFIGURATION
    // ========================================
    
    /**
     * Set PID gains (can be tuned at runtime)
     */
    void setTunings(float kP, float kI, float kD);
    
    /**
     * Set the target setpoint
     */
    void setSetpoint(float setpoint);
    
    /**
     * Set output limits to prevent excessive values
     * Default: -255 to 255 (motor PWM range)
     */
    void setOutputLimits(float minOutput, float maxOutput);
    
    /**
     * Set integral limits for anti-windup
     * Default: same as output limits
     */
    void setIntegralLimits(float minIntegral, float maxIntegral);

    // ========================================
    // COMPUTATION
    // ========================================
    
    /**
     * Compute PID output based on current input
     * Uses actual time delta for accurate derivative/integral
     * @param input Current measured value
     * @return Control output (clamped to output limits)
     */
    float compute(float input);
    
    /**
     * Compute with explicit time delta (for testing or fixed-rate loops)
     * @param input Current measured value
     * @param dt Time delta in seconds
     * @return Control output
     */
    float computeWithDt(float input, float dt);
    
    /**
     * Reset controller state (integral, last error, timing)
     * Call when switching modes or after a pause
     */
    void reset();

    // ========================================
    // STATUS
    // ========================================
    
    float getSetpoint() const { return _setpoint; }
    float getError() const { return _lastError; }
    float getOutput() const { return _lastOutput; }
    float getProportional() const { return _lastP; }
    float getIntegral() const { return _integral; }
    float getDerivative() const { return _lastD; }
    
    // Check if at setpoint (within tolerance)
    bool atSetpoint(float tolerance = 1.0f) const;

private:
    // Gains
    float _kP, _kI, _kD;
    
    // Setpoint and limits
    float _setpoint;
    float _outputMin, _outputMax;
    float _integralMin, _integralMax;
    
    // State
    float _integral;
    float _lastError;
    float _lastInput;  // For derivative-on-measurement
    float _lastOutput;
    unsigned long _lastTime;
    bool _firstRun;
    
    // Debug/status
    float _lastP, _lastD;
};

#endif // PID_CONTROLLER_H
