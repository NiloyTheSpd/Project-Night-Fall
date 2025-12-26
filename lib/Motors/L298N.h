#ifndef L298N_H
#define L298N_H

#include <Arduino.h>

/**
 * L298N Motor Driver Abstraction
 * Supports 1 or 2 motors per driver via PWM speed control + direction pins
 */
class L298N
{
public:
    /**
     * Constructor for single motor control
     * @param ena PWM pin for motor speed
     * @param in1 Direction pin A
     * @param in2 Direction pin B
     * @param channel PWM channel to use
     */
    L298N(uint8_t ena, uint8_t in1, uint8_t in2, uint8_t channel);

    /**
     * Constructor for dual motor control
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

    // Single motor methods
    void setSpeed(int speed); // -255 to 255 (negative = reverse)
    void forward(uint8_t speed = 200);
    void backward(uint8_t speed = 200);
    void stop();

    // Dual motor methods
    void setMotor1Speed(int speed);
    void setMotor2Speed(int speed);
    void setMotors(int speed1, int speed2);
    void setMotorsForward(uint8_t speed = 200);
    void setMotorsBackward(uint8_t speed = 200);
    void turnLeft(uint8_t speed = 150);
    void turnRight(uint8_t speed = 150);
    void stopMotors();

    bool isMoving();
    int getMotor1Speed() { return _speed1; }
    int getMotor2Speed() { return _speed2; }

private:
    // Single motor config
    uint8_t _ena, _in1, _in2;
    uint8_t _channel;
    int _speed;

    // Dual motor config
    uint8_t _ena1, _in1a, _in1b;
    uint8_t _ena2, _in2a, _in2b;
    uint8_t _channel1, _channel2;
    int _speed1, _speed2;

    bool _isDualMotor;

    void setSingleMotorSpeed(uint8_t ena, uint8_t in1, uint8_t in2, int speed, uint8_t channel);
};

#endif // L298N_H
