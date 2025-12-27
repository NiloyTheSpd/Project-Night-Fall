#ifndef L298N_H
#define L298N_H

#include <Arduino.h>

/**
 * L298N Motor Driver Abstraction (REFACTORED - Dual Motor Only)
 *
 * Supports 2 motors per driver via PWM speed control + direction pins
 * 
 * Features:
 * - Instant speed control (setMotors)
 * - Ramped speed control (setMotorsRamped + update)
 * - Configurable ramp rate for smooth acceleration/deceleration
 */
class L298N
{
public:
    /**
     * Constructor for dual motor control
     *
     * @param ena1 PWM pin for motor 1 speed
     * @param in1a Direction pin A (motor 1)
     * @param in1b Direction pin B (motor 1)
     * @param ena2 PWM pin for motor 2 speed
     * @param in2a Direction pin A (motor 2)
     * @param in2b Direction pin B (motor 2)
     * @param channel1 PWM channel for motor 1
     * @param channel2 PWM channel for motor 2
     */
    L298N(uint8_t ena1, uint8_t in1a, uint8_t in1b,
          uint8_t ena2, uint8_t in2a, uint8_t in2b,
          uint8_t channel1, uint8_t channel2);

    void begin();

    // ========================================
    // INSTANT SPEED CONTROL (immediate change)
    // ========================================
    void setMotor1Speed(int speed);
    void setMotor2Speed(int speed);
    void setMotors(int speed1, int speed2);

    // Convenience movement methods (instant)
    void setMotorsForward(uint8_t speed = 200);
    void setMotorsBackward(uint8_t speed = 200);
    void turnLeft(uint8_t speed = 150);
    void turnRight(uint8_t speed = 150);
    void stopMotors();

    // ========================================
    // RAMPED SPEED CONTROL (gradual change)
    // ========================================
    /**
     * Set target speeds with ramping. Call update() in loop to apply.
     * @param target1 Target speed for motor 1 (-255 to 255)
     * @param target2 Target speed for motor 2 (-255 to 255)
     */
    void setMotorsRamped(int target1, int target2);
    
    /**
     * Set ramp rate (PWM units per update call)
     * Default: 10 (reaches 255 in ~25 updates at 20Hz = 1.25s ramp)
     */
    void setRampRate(uint8_t rate);
    
    /**
     * Update motor speeds towards targets. Call every loop iteration.
     * @return true if motors are still ramping, false if at target
     */
    bool update();
    
    /**
     * Check if motors are currently ramping towards target
     */
    bool isRamping() const;

    // ========================================
    // STATUS QUERIES
    // ========================================
    bool isMoving();
    int getMotor1Speed() const { return _speed1; }
    int getMotor2Speed() const { return _speed2; }
    int getTarget1Speed() const { return _target1; }
    int getTarget2Speed() const { return _target2; }

private:
    // Motor 1
    uint8_t _ena1, _in1a, _in1b;
    uint8_t _channel1;
    int _speed1;
    int _target1;

    // Motor 2
    uint8_t _ena2, _in2a, _in2b;
    uint8_t _channel2;
    int _speed2;
    int _target2;
    
    // Ramping
    uint8_t _rampRate;

    // Helper: control individual motor with direction and PWM
    void setSingleMotorSpeed(uint8_t ena, uint8_t in1, uint8_t in2,
                             int speed, uint8_t channel);
    
    // Helper: move value towards target by step
    static int moveTowards(int current, int target, int step);
};

#endif // L298N_H
