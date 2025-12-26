#ifndef L298N_H
#define L298N_H

#include <Arduino.h>

/**
 * L298N Motor Driver Abstraction (REFACTORED - Dual Motor Only)
 *
 * Supports 2 motors per driver via PWM speed control + direction pins
 * Removed single-motor mode (constructors, methods) - not used in any codebase
 *
 * Improvements:
 * - Simpler code, reduced maintenance burden
 * - Smaller memory footprint
 * - Faster compilation
 * - Matches actual usage pattern (dual motors everywhere)
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

    // Motor speed control (speed: -255 to 255, negative = reverse)
    void setMotor1Speed(int speed);
    void setMotor2Speed(int speed);
    void setMotors(int speed1, int speed2);

    // Convenience movement methods
    void setMotorsForward(uint8_t speed = 200);
    void setMotorsBackward(uint8_t speed = 200);
    void turnLeft(uint8_t speed = 150);
    void turnRight(uint8_t speed = 150);
    void stopMotors();

    // Status queries
    bool isMoving();
    int getMotor1Speed() const { return _speed1; }
    int getMotor2Speed() const { return _speed2; }

private:
    // Motor 1
    uint8_t _ena1, _in1a, _in1b;
    uint8_t _channel1;
    int _speed1;

    // Motor 2
    uint8_t _ena2, _in2a, _in2b;
    uint8_t _channel2;
    int _speed2;

    // Helper: control individual motor with direction and PWM
    void setSingleMotorSpeed(uint8_t ena, uint8_t in1, uint8_t in2,
                             int speed, uint8_t channel);
};

#endif // L298N_H
