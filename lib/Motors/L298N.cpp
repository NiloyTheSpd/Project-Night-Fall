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
    : _ena1(ena1), _in1a(in1a), _in1b(in1b), _channel1(channel1), _speed1(0), _target1(0),
      _ena2(ena2), _in2a(in2a), _in2b(in2b), _channel2(channel2), _speed2(0), _target2(0),
      _rampRate(10)  // Default: 10 PWM units per update (~1.25s full ramp at 20Hz)
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

// ============================================
// RAMPED SPEED CONTROL
// ============================================

void L298N::setMotorsRamped(int target1, int target2)
{
    _target1 = constrain(target1, -255, 255);
    _target2 = constrain(target2, -255, 255);
}

void L298N::setRampRate(uint8_t rate)
{
    _rampRate = (rate > 0) ? rate : 1;  // Minimum rate of 1
}

bool L298N::update()
{
    // Move current speeds towards targets
    int newSpeed1 = moveTowards(_speed1, _target1, _rampRate);
    int newSpeed2 = moveTowards(_speed2, _target2, _rampRate);
    
    // Only update hardware if speed changed
    if (newSpeed1 != _speed1)
    {
        _speed1 = newSpeed1;
        setSingleMotorSpeed(_ena1, _in1a, _in1b, _speed1, _channel1);
    }
    
    if (newSpeed2 != _speed2)
    {
        _speed2 = newSpeed2;
        setSingleMotorSpeed(_ena2, _in2a, _in2b, _speed2, _channel2);
    }
    
    // Return true if still ramping
    return isRamping();
}

bool L298N::isRamping() const
{
    return (_speed1 != _target1) || (_speed2 != _target2);
}

int L298N::moveTowards(int current, int target, int step)
{
    if (current < target)
    {
        return min(current + step, target);
    }
    else if (current > target)
    {
        return max(current - step, target);
    }
    return current;
}
