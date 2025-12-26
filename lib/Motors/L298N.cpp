#include "L298N.h"

// Constructor for single motor
L298N::L298N(uint8_t ena, uint8_t in1, uint8_t in2, uint8_t channel)
    : _ena(ena), _in1(in1), _in2(in2), _channel(channel),
      _speed(0), _isDualMotor(false), _ena1(0), _in1a(0), _in1b(0),
      _ena2(0), _in2a(0), _in2b(0), _channel1(0), _channel2(0),
      _speed1(0), _speed2(0)
{
}

// Constructor for dual motor
L298N::L298N(uint8_t ena1, uint8_t in1a, uint8_t in1b,
             uint8_t ena2, uint8_t in2a, uint8_t in2b,
             uint8_t channel1, uint8_t channel2)
    : _ena1(ena1), _in1a(in1a), _in1b(in1b),
      _ena2(ena2), _in2a(in2a), _in2b(in2b),
      _channel1(channel1), _channel2(channel2),
      _isDualMotor(true), _speed1(0), _speed2(0),
      _ena(0), _in1(0), _in2(0), _channel(0), _speed(0)
{
}

void L298N::begin()
{
    if (_isDualMotor)
    {
        pinMode(_ena1, OUTPUT);
        pinMode(_in1a, OUTPUT);
        pinMode(_in1b, OUTPUT);
        pinMode(_ena2, OUTPUT);
        pinMode(_in2a, OUTPUT);
        pinMode(_in2b, OUTPUT);

        ledcSetup(_channel1, 5000, 8);
        ledcAttachPin(_ena1, _channel1);
        ledcSetup(_channel2, 5000, 8);
        ledcAttachPin(_ena2, _channel2);
    }
    else
    {
        pinMode(_ena, OUTPUT);
        pinMode(_in1, OUTPUT);
        pinMode(_in2, OUTPUT);

        ledcSetup(_channel, 5000, 8);
        ledcAttachPin(_ena, _channel);
    }
}

// Single motor control
void L298N::setSpeed(int speed)
{
    if (!_isDualMotor)
    {
        speed = constrain(speed, -255, 255);
        _speed = speed;
        setSingleMotorSpeed(_ena, _in1, _in2, speed, _channel);
    }
}

void L298N::forward(uint8_t speed)
{
    if (!_isDualMotor)
        setSpeed(constrain(speed, 0, 255));
}

void L298N::backward(uint8_t speed)
{
    if (!_isDualMotor)
        setSpeed(-constrain(speed, 0, 255));
}

void L298N::stop()
{
    if (!_isDualMotor)
        setSpeed(0);
}

// Dual motor control
void L298N::setMotor1Speed(int speed)
{
    if (_isDualMotor)
    {
        speed = constrain(speed, -255, 255);
        _speed1 = speed;
        setSingleMotorSpeed(_ena1, _in1a, _in1b, speed, _channel1);
    }
}

void L298N::setMotor2Speed(int speed)
{
    if (_isDualMotor)
    {
        speed = constrain(speed, -255, 255);
        _speed2 = speed;
        setSingleMotorSpeed(_ena2, _in2a, _in2b, speed, _channel2);
    }
}

void L298N::setMotors(int speed1, int speed2)
{
    if (_isDualMotor)
    {
        setMotor1Speed(speed1);
        setMotor2Speed(speed2);
    }
}

void L298N::setMotorsForward(uint8_t speed)
{
    if (_isDualMotor)
        setMotors(constrain(speed, 0, 255), constrain(speed, 0, 255));
}

void L298N::setMotorsBackward(uint8_t speed)
{
    if (_isDualMotor)
        setMotors(-constrain(speed, 0, 255), -constrain(speed, 0, 255));
}

void L298N::turnLeft(uint8_t speed)
{
    if (_isDualMotor)
    {
        speed = constrain(speed, 0, 255);
        setMotor1Speed(speed / 2); // Left slower
        setMotor2Speed(speed);     // Right faster
    }
}

void L298N::turnRight(uint8_t speed)
{
    if (_isDualMotor)
    {
        speed = constrain(speed, 0, 255);
        setMotor1Speed(speed);     // Left faster
        setMotor2Speed(speed / 2); // Right slower
    }
}

void L298N::stopMotors()
{
    if (_isDualMotor)
        setMotors(0, 0);
}

bool L298N::isMoving()
{
    if (_isDualMotor)
        return (_speed1 != 0) || (_speed2 != 0);
    else
        return _speed != 0;
}

void L298N::setSingleMotorSpeed(uint8_t ena, uint8_t in1, uint8_t in2, int speed, uint8_t channel)
{
    // Clamp speed
    speed = constrain(speed, -255, 255);

    // Set direction
    if (speed > 0)
    {
        digitalWrite(in1, HIGH);
        digitalWrite(in2, LOW);
        ledcWrite(channel, speed);
    }
    else if (speed < 0)
    {
        digitalWrite(in1, LOW);
        digitalWrite(in2, HIGH);
        ledcWrite(channel, -speed);
    }
    else
    {
        digitalWrite(in1, LOW);
        digitalWrite(in2, LOW);
        ledcWrite(channel, 0);
    }
}
