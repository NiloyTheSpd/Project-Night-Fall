#ifndef PINS_H
#define PINS_H

// ============================================
// PIN DEFINITIONS - PROJECT NIGHTFALL REFACTORED
// ============================================

// ===== BACK ESP32 (MASTER BRAIN) =====
#ifdef BACK_CONTROLLER

// Rear Motors (L298N Driver)
#define MOTOR_REAR_LEFT_ENA 13  // PWM
#define MOTOR_REAR_LEFT_IN1 12  // Direction A
#define MOTOR_REAR_LEFT_IN2 14  // Direction B
#define MOTOR_REAR_RIGHT_ENB 25 // PWM
#define MOTOR_REAR_RIGHT_IN3 26 // Direction A
#define MOTOR_REAR_RIGHT_IN4 27 // Direction B

// Ultrasonic Sensors
#define ULTRASONIC_FRONT_TRIG 5 // Front sensor
#define ULTRASONIC_FRONT_ECHO 18
#define ULTRASONIC_REAR_TRIG 19 // Rear sensor
#define ULTRASONIC_REAR_ECHO 21

// MQ-2 Gas Sensor
#define GAS_SENSOR_ANALOG 32  // ADC pin
#define GAS_SENSOR_DIGITAL 33 // Digital alert pin (optional)

// Buzzer
#define BUZZER_PIN 2

// PWM Channels
#define PWM_CHANNEL_REAR_LEFT 0
#define PWM_CHANNEL_REAR_RIGHT 1
#define PWM_CHANNEL_BUZZER 2

#endif // BACK_CONTROLLER

// ===== FRONT ESP32 (MOTOR SLAVE) =====
#ifdef FRONT_CONTROLLER

// Front Motors Bank 1 (L298N Driver)
#define MOTOR_FRONT_LEFT1_ENA 13  // PWM
#define MOTOR_FRONT_LEFT1_IN1 12  // Direction A
#define MOTOR_FRONT_LEFT1_IN2 14  // Direction B
#define MOTOR_FRONT_RIGHT1_ENB 25 // PWM
#define MOTOR_FRONT_RIGHT1_IN3 26 // Direction A
#define MOTOR_FRONT_RIGHT1_IN4 27 // Direction B

// Front Motors Bank 2 (Second L298N Driver)
#define MOTOR_FRONT_LEFT2_ENA 4   // PWM
#define MOTOR_FRONT_LEFT2_IN1 16  // Direction A
#define MOTOR_FRONT_LEFT2_IN2 17  // Direction B
#define MOTOR_FRONT_RIGHT2_ENB 18 // PWM
#define MOTOR_FRONT_RIGHT2_IN3 19 // Direction A
#define MOTOR_FRONT_RIGHT2_IN4 21 // Direction B

// PWM Channels
#define PWM_CHANNEL_FRONT_LEFT1 0
#define PWM_CHANNEL_FRONT_RIGHT1 1
#define PWM_CHANNEL_FRONT_LEFT2 2
#define PWM_CHANNEL_FRONT_RIGHT2 3

#endif // FRONT_CONTROLLER

// ===== ESP32-CAM (VISION + UI) =====
#ifdef CAMERA_CONTROLLER

// Camera pins are internally configured
// No additional pin definitions needed for camera itself

// Optional: LED flash
#define FLASH_LED 4

// microSD Card (SD_MMC 1-bit mode)
// Uses internal pins, no explicit definition needed

#endif // CAMERA_CONTROLLER

#endif // PINS_H
