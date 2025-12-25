// lib/Control/ControlUtils.h
#ifndef CONTROL_UTILS_H
#define CONTROL_UTILS_H

#include <Arduino.h>
#include "config.h"

struct PIDState
{
    float integral = 0.0f;
    float previousError = 0.0f;
    float outMin = SPEED_OUTPUT_MIN;
    float outMax = SPEED_OUTPUT_MAX;
};

namespace ControlUtils
{

    // Returns TTC in milliseconds, or -1 if invalid/not approaching
    inline float computeTTC(float distance_cm, float approach_speed_cm_s)
    {
        if (distance_cm <= 0.0f || approach_speed_cm_s <= 0.0f)
        {
            return -1.0f;
        }
        // time = distance / speed; convert seconds to milliseconds
        float ttc_ms = (distance_cm / approach_speed_cm_s) * 1000.0f;
        return ttc_ms;
    }

    // Basic PID step with clamped output
    inline float pidStep(float setpoint, float measurement, float dt_s,
                         PIDState &state, float kp, float ki, float kd)
    {
        float error = setpoint - measurement;
        state.integral += error * dt_s;

        // Anti-windup: clamp integral by output limits / ki when ki>0
        if (ki > 0.0f)
        {
            float maxIntegral = (state.outMax) / ki;
            float minIntegral = (state.outMin) / ki;
            if (state.integral > maxIntegral)
                state.integral = maxIntegral;
            if (state.integral < minIntegral)
                state.integral = minIntegral;
        }

        float derivative = dt_s > 0.0f ? (error - state.previousError) / dt_s : 0.0f;
        state.previousError = error;

        float output = kp * error + ki * state.integral + kd * derivative;

        // Clamp output
        if (output > state.outMax)
            output = state.outMax;
        if (output < state.outMin)
            output = state.outMin;

        return output;
    }

    // Slew-rate limiter for integer PWM values
    inline int rampSlew(int current, int target, float maxDeltaPerUpdate)
    {
        float delta = (float)target - (float)current;
        if (fabsf(delta) <= maxDeltaPerUpdate)
        {
            return target;
        }
        if (delta > 0)
        {
            return current + (int)maxDeltaPerUpdate;
        }
        else
        {
            return current - (int)maxDeltaPerUpdate;
        }
    }

} // namespace ControlUtils

#endif // CONTROL_UTILS_H
