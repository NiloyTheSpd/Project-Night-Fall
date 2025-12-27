// include/pins.h - CORRECTED for WiFi Architecture
#ifndef PINS_H
#define PINS_H

// ============================================
// BACK ESP32 (Master/Brain/Sensor Node)
// ============================================
#ifdef BACK_CONTROLLER

// L298N Motor Driver (Rear Motors)
#define MOTOR_REAR_L_ENA 13 // PWM - Left motor speed ✅
#define MOTOR_REAR_L_IN1 23 // Digital - Left motor direction A ✅ FIXED
#define MOTOR_REAR_L_IN2 22 // Digital - Left motor direction B ✅ FIXED
#define MOTOR_REAR_R_ENB 25 // PWM - Right motor speed ✅
#define MOTOR_REAR_R_IN3 26 // Digital - Right motor direction A ✅
#define MOTOR_REAR_R_IN4 27 // Digital - Right motor direction B ✅

// HC-SR04 Ultrasonic Sensors
#define US_FRONT_TRIG 14 // Digital output - Trigger pulse ✅ moved off strap GPIO15
#define US_FRONT_ECHO 18 // Digital input - Echo (with voltage divider!) ✅
#define US_REAR_TRIG 19  // Digital output - Trigger pulse ✅
#define US_REAR_ECHO 21  // Digital input - Echo (with voltage divider!) ✅

// Aliases for SensorManager compatibility
#define ULTRASONIC_FRONT_TRIG US_FRONT_TRIG
#define ULTRASONIC_FRONT_ECHO US_FRONT_ECHO
#define ULTRASONIC_REAR_TRIG US_REAR_TRIG
#define ULTRASONIC_REAR_ECHO US_REAR_ECHO

// MQ-2 Gas/Smoke Sensor
#define GAS_ANALOG 32  // ADC1 - Analog gas level (0-4095) ✅
#define GAS_DIGITAL 33 // Digital - Threshold output (optional) ✅

// Aliases for SensorManager compatibility
#define GAS_SENSOR_ANALOG GAS_ANALOG
#define GAS_SENSOR_DIGITAL GAS_DIGITAL

// Buzzer
#define BUZZER_PIN 4 // PWM or Digital output ✅ FIXED

// WiFi Configuration (Access Point)
#define WIFI_AP_SSID "ProjectNightfall"
#define WIFI_AP_PASSWORD "rescue2025"
#define WIFI_AP_CHANNEL 6
#define WEBSOCKET_PORT 8888

// PWM Channels (for motor speed control)
#define PWM_CHANNEL_REAR_L 0
#define PWM_CHANNEL_REAR_R 1
#define PWM_CHANNEL_BUZZER 2

#endif // BACK_CONTROLLER

// ============================================
// FRONT ESP32 (Motor Slave Node)
// ============================================
#ifdef FRONT_CONTROLLER

// L298N Motor Driver #1 (Motors 1 & 2)
#define MOTOR_1_ENA 13 // PWM - Motor 1 speed ✅
#define MOTOR_1_IN1 23 // Digital - Motor 1 direction A ✅ FIXED
#define MOTOR_1_IN2 22 // Digital - Motor 1 direction B ✅ FIXED
#define MOTOR_2_ENB 25 // PWM - Motor 2 speed ✅
#define MOTOR_2_IN3 26 // Digital - Motor 2 direction A ✅
#define MOTOR_2_IN4 27 // Digital - Motor 2 direction B ✅

// L298N Motor Driver #2 (Motors 3 & 4)
#define MOTOR_3_ENA 14 // PWM - Motor 3 speed ✅ FIXED
#define MOTOR_3_IN1 32 // Digital - Motor 3 direction A ✅ FIXED
#define MOTOR_3_IN2 33 // Digital - Motor 3 direction B ✅ FIXED
#define MOTOR_4_ENB 15 // PWM - Motor 4 speed ✅ FIXED
#define MOTOR_4_IN3 19 // Digital - Motor 4 direction A ✅
#define MOTOR_4_IN4 21 // Digital - Motor 4 direction B ✅

// WiFi Configuration (Station/Client)
#define WIFI_SSID "ProjectNightfall"
#define WIFI_PASSWORD "rescue2025"
#define WEBSOCKET_SERVER "ws://192.168.4.1:8888"

// PWM Channels
#define PWM_CHANNEL_M1 0
#define PWM_CHANNEL_M2 1
#define PWM_CHANNEL_M3 2
#define PWM_CHANNEL_M4 3

#endif // FRONT_CONTROLLER

// ============================================
// CAMERA ESP32 (ESP32-CAM Telemetry Bridge)
// ============================================
#ifdef CAMERA_CONTROLLER

// Built-in Flash LED
#define FLASH_LED_PIN 4 // Built-in white LED (PWM for brightness) ✅

// Status LED (Optional external)
#define STATUS_LED_PIN 33 // External LED (via 220Ω resistor) ✅ moved off strap GPIO12

// WiFi Configuration (Station/Client)
#define WIFI_SSID "ProjectNightfall"
#define WIFI_PASSWORD "rescue2025"
#define WEBSOCKET_SERVER "ws://192.168.4.1:8888"

// Camera pins (DO NOT USE - Reserved for OV2640)
// GPIO 0, 5, 18, 19, 21, 22, 23, 25, 26, 27
// GPIO 32, 34, 35, 36, 39

// Programming pins
#define UART_TX 1   // TX0 - Connect to FTDI RX
#define UART_RX 3   // RX0 - Connect to FTDI TX
#define GPIO_BOOT 0 // Hold LOW during programming

#endif // CAMERA_CONTROLLER

// ============================================
// COMMON DEFINITIONS
// ============================================

// PWM Configuration
#define PWM_FREQUENCY 5000 // 5 kHz for motor drivers
#define PWM_RESOLUTION 8   // 8-bit (0-255)

// Motor Speed Limits
#define MOTOR_MIN_SPEED 0   // Minimum PWM value
#define MOTOR_MAX_SPEED 255 // Maximum PWM value
#define MOTOR_STOP 0        // Stop value

// Sensor Thresholds
#define US_SAFE_DISTANCE 20 // cm - Minimum safe distance
#define US_MAX_DISTANCE 400 // cm - Maximum sensor range
#define GAS_THRESHOLD 400   // Analog value for gas alert

// Timing
#define SENSOR_UPDATE_INTERVAL 100 // ms - Sensor reading rate
#define TELEMETRY_INTERVAL 500     // ms - Telemetry broadcast rate
#define HEARTBEAT_INTERVAL 1000    // ms - Heartbeat/keepalive

// WebSocket Message Types
#define MSG_TYPE_MOTOR "motor"
#define MSG_TYPE_SENSOR "sensor"
#define MSG_TYPE_STATUS "status"
#define MSG_TYPE_ALERT "alert"
#define MSG_TYPE_COMMAND "command"

#endif // PINS_H
