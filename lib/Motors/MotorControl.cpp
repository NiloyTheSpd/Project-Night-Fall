#include "MotorControl.h"
#include "pins.h"

MotorControl::MotorControl(uint8_t ena, uint8_t in1, uint8_t in2,
                           uint8_t enb, uint8_t in3, uint8_t in4,
                           uint8_t pwm_ch_a, uint8_t pwm_ch_b)
    : _ena(ena), _in1(in1), _in2(in2),
      _enb(enb), _in3(in3), _in4(in4),
      _pwm_ch_a(pwm_ch_a), _pwm_ch_b(pwm_ch_b),
      _currentLeftSpeed(0), _currentRightSpeed(0), _isMoving(false) {}

void MotorControl::begin()
{
    // Configure motor pins as outputs
    pinMode(_in1, OUTPUT);
    pinMode(_in2, OUTPUT);
    pinMode(_in3, OUTPUT);
    pinMode(_in4, OUTPUT);

    // Setup PWM for motor speed control
    ledcSetup(_pwm_ch_a, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcSetup(_pwm_ch_b, MOTOR_PWM_FREQ, MOTOR_PWM_RESOLUTION);
    ledcAttachPin(_ena, _pwm_ch_a);
    ledcAttachPin(_enb, _pwm_ch_b);

    // Initialize motors to stopped state
    stop();

    DEBUG_PRINTLN("Motor Control Initialized");
}

void MotorControl::setSpeed(int leftSpeed, int rightSpeed)
{
    setLeftMotor(leftSpeed);
    setRightMotor(rightSpeed);

    _currentLeftSpeed = leftSpeed;
    _currentRightSpeed = rightSpeed;
    _isMoving = (leftSpeed != 0 || rightSpeed != 0);
}

void MotorControl::setLeftMotor(int speed)
{
    // Constrain speed to valid range
    speed = constrain(speed, -255, 255);

    if (speed > 0)
    {
        // Forward
        digitalWrite(_in1, HIGH);
        digitalWrite(_in2, LOW);
        ledcWrite(_pwm_ch_a, abs(speed));
    }
    else if (speed < 0)
    {
        // Reverse
        digitalWrite(_in1, LOW);
        digitalWrite(_in2, HIGH);
        ledcWrite(_pwm_ch_a, abs(speed));
    }
    else
    {
        // Stop
        digitalWrite(_in1, LOW);
        digitalWrite(_in2, LOW);
        ledcWrite(_pwm_ch_a, 0);
    }
}

void MotorControl::setRightMotor(int speed)
{
    speed = constrain(speed, -255, 255);

    if (speed > 0)
    {
        digitalWrite(_in3, HIGH);
        digitalWrite(_in4, LOW);
        ledcWrite(_pwm_ch_b, abs(speed));
    }
    else if (speed < 0)
    {
        digitalWrite(_in3, LOW);
        digitalWrite(_in4, HIGH);
        ledcWrite(_pwm_ch_b, abs(speed));
    }
    else
    {
        digitalWrite(_in3, LOW);
        digitalWrite(_in4, LOW);
        ledcWrite(_pwm_ch_b, 0);
    }
}

void MotorControl::forward(uint8_t speed)
{
    setSpeed(speed, speed);
    DEBUG_PRINT("Moving Forward: ");
    DEBUG_PRINTLN(speed);
}

void MotorControl::backward(uint8_t speed)
{
    setSpeed(-speed, -speed);
    DEBUG_PRINT("Moving Backward: ");
    DEBUG_PRINTLN(speed);
}

void MotorControl::turnLeft(uint8_t speed)
{
    // Left wheel slower/backward, right wheel faster/forward
    setSpeed(-speed / 2, speed);
    DEBUG_PRINTLN("Turning Left");
}

void MotorControl::turnRight(uint8_t speed)
{
    // Right wheel slower/backward, left wheel faster/forward
    setSpeed(speed, -speed / 2);
    DEBUG_PRINTLN("Turning Right");
}

void MotorControl::rotate360(bool clockwise)
{
    if (clockwise)
    {
        setSpeed(MOTOR_TURN_SPEED, -MOTOR_TURN_SPEED);
    }
    else
    {
        setSpeed(-MOTOR_TURN_SPEED, MOTOR_TURN_SPEED);
    }
    DEBUG_PRINTLN("Rotating 360");
    delay(ROTATION_360_DURATION);
    stop();
}

void MotorControl::stop()
{
    setSpeed(0, 0);
    DEBUG_PRINTLN("Motors Stopped");
}

void MotorControl::boostFront(uint16_t duration)
{
    DEBUG_PRINTLN("Front Boost Activated for Climbing");
    forward(MOTOR_CLIMB_SPEED);
    delay(duration);
    forward(MOTOR_NORMAL_SPEED);
}

bool MotorControl::isMoving()
{
    return _isMoving;
}

void MotorControl::emergencyStop()
{
    // Immediate stop without gradual deceleration
    digitalWrite(_in1, LOW);
    digitalWrite(_in2, LOW);
    digitalWrite(_in3, LOW);
    digitalWrite(_in4, LOW);
    ledcWrite(_pwm_ch_a, 0);
    ledcWrite(_pwm_ch_b, 0);
    _isMoving = false;
    DEBUG_PRINTLN("EMERGENCY STOP ACTIVATED!");
}