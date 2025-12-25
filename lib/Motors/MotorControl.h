#ifndef MOTOR_CONTROL_H
#define MOTOR_CONTROL_H

#include <Arduino.h>
#include "config.h"

class MotorControl
{
public:
    MotorControl(uint8_t ena, uint8_t in1, uint8_t in2,
                 uint8_t enb, uint8_t in3, uint8_t in4,
                 uint8_t pwm_ch_a, uint8_t pwm_ch_b);

    void begin();
    void setSpeed(int leftSpeed, int rightSpeed);
    void forward(uint8_t speed = MOTOR_NORMAL_SPEED);
    void backward(uint8_t speed = MOTOR_NORMAL_SPEED);
    void turnLeft(uint8_t speed = MOTOR_TURN_SPEED);
    void turnRight(uint8_t speed = MOTOR_TURN_SPEED);
    void rotate360(bool clockwise = true);
    void stop();
    void boostFront(uint8_t duration = CLIMB_BOOST_DURATION);

    bool isMoving();
    void emergencyStop();

private:
    uint8_t _ena, _in1, _in2;
    uint8_t _enb, _in3, _in4;
    uint8_t _pwm_ch_a, _pwm_ch_b;

    int _currentLeftSpeed;
    int _currentRightSpeed;
    bool _isMoving;

    void setLeftMotor(int speed);
    void setRightMotor(int speed);
};

#endif // MOTOR_CONTROL_H