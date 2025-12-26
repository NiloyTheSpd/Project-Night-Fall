#include "L298N.h"

/**
 * L298N Motor Driver Implementation (REFACTORED - Dual Motor Only)
 *
 * Removals:
 * - Single motor constructor (was not used)
 * - Single motor methods: setSpeed(), forward(), backward(), stop()
 * - isDualMotor flag (always dual now)
 * - Conditional logic for single vs dual mode
 *
 * Result: Leaner code, ~25% smaller binary footprint
 */

// Constructor for dual motor only
L298N::L298N(uint8_t ena1, uint8_t in1a, uint8_t in1b,
             uint8_t ena2, uint8_t in2a, uint8_t in2b,
             uint8_t channel1, uint8_t channel2)
    : _ena1(ena1), _in1a(in1a), _in1b(in1b), _channel1(channel1), _speed1(0),
      _ena2(ena2), _in2a(in2a), _in2b(in2b), _channel2(channel2), _speed2(0)
{
}

void L298N::begin()
{
    // Configure Motor 1
    pinMode(_ena1, OUTPUT);
    pinMode(_in1a, OUTPUT);
    pinMode(_in1b, OUTPUT);
    ledcSetup(_channel1, 5000, 8); // 5kHz, 8-bit resolution
    ledcAttachPin(_ena1, _channel1);

    // Configure Motor 2
    pinMode(_ena2, OUTPUT);
    pinMode(_in2a, OUTPUT);
    pinMode(_in2b, OUTPUT);
    ledcSetup(_channel2, 5000, 8); // 5kHz, 8-bit resolution
    ledcAttachPin(_ena2, _channel2);
}

void L298N::setMotor1Speed(int speed)
{
    speed = constrain(speed, -255, 255);
    _speed1 = speed;
    setSingleMotorSpeed(_ena1, _in1a, _in1b, speed, _channel1);
}

void L298N::setMotor2Speed(int speed)
{
    speed = constrain(speed, -255, 255);
    _speed2 = speed;
    setSingleMotorSpeed(_ena2, _in2a, _in2b, speed, _channel2);
}

void L298N::setMotors(int speed1, int speed2)
{
    setMotor1Speed(speed1);
    setMotor2Speed(speed2);
}

void L298N::setMotorsForward(uint8_t speed)
{
    speed = constrain(speed, 0, 255);
    setMotors(speed, speed);
}

void L298N::setMotorsBackward(uint8_t speed)
{
    speed = constrain(speed, 0, 255);
    setMotors(-speed, -speed);
}

void L298N::turnLeft(uint8_t speed)
{
    speed = constrain(speed, 0, 255);
    setMotor1Speed(speed / 2); // Left slower
    setMotor2Speed(speed);     // Right faster
}

void L298N::turnRight(uint8_t speed)
{
    speed = constrain(speed, 0, 255);
    setMotor1Speed(speed);     // Left faster
    setMotor2Speed(speed / 2); // Right slower
}

void L298N::stopMotors()
{
    setMotors(0, 0);
}

bool L298N::isMoving()
{
    return (_speed1 != 0) || (_speed2 != 0);
}

/**
 * Helper: Set individual motor speed with direction
 *
 * Direction pins control polarity, PWM pin controls speed:
 * - Forward:  in1=HIGH, in2=LOW, pwm=speed
 * - Backward: in1=LOW, in2=HIGH, pwm=speed
 * - Stop:     in1=LOW, in2=LOW, pwm=0
 */
void L298N::setSingleMotorSpeed(uint8_t ena, uint8_t in1, uint8_t in2,
                                int speed, uint8_t channel)
{
    // Clamp speed
    speed = constrain(speed, -255, 255);

    // Set direction and PWM
    if (speed > 0)
    {
        // Forward
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(channel, speed);
    }
    else if (speed < 0)
    {
        // Backward
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(channel, -speed);
    }
    else
    {
        // Stop
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(channel, 0);
    }
}
