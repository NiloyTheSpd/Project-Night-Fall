#ifndef PINS_H
#define PINS_H

// ============================================
// PIN DEFINITIONS
// ============================================

#ifdef FRONT_CONTROLLER
// ESP32 #1 - Front/Master Controller

// Motor Driver Pins (Front L298N)
#define MOTOR_FL_ENA 18 // Front Left Enable (PWM)
#define MOTOR_FL_IN1 19 // Front Left Direction A
#define MOTOR_FL_IN2 21 // Front Left Direction B
#define MOTOR_FR_ENB 22 // Front Right Enable (PWM)
#define MOTOR_FR_IN3 23 // Front Right Direction A
#define MOTOR_FR_IN4 25 // Front Right Direction B

// Ultrasonic Sensor Pins (Front)
#define US_FRONT_TRIG 26
#define US_FRONT_ECHO 27

// Gas Sensor Pins
#define GAS_ANALOG 32  // MQ-2 Analog Output
#define GAS_DIGITAL 33 // MQ-2 Digital Output

// Buzzer Pin
#define BUZZER_PIN 13

// UART Communication
#define UART_REAR_TX 16 // To ESP32 #2
#define UART_REAR_RX 17 // From ESP32 #2
#define UART_CAM_TX 14  // To ESP32-CAM
#define UART_CAM_RX 12  // From ESP32-CAM

#endif

#ifdef REAR_CONTROLLER
// ESP32 #2 - Rear/Slave Controller

// Motor Driver Pins (Rear L298N)
#define MOTOR_RL_ENA 18 // Rear Left Enable (PWM)
#define MOTOR_RL_IN1 19 // Rear Left Direction A
#define MOTOR_RL_IN2 21 // Rear Left Direction B
#define MOTOR_RR_ENB 22 // Rear Right Enable (PWM)
#define MOTOR_RR_IN3 23 // Rear Right Direction A
#define MOTOR_RR_IN4 25 // Rear Right Direction B

// Ultrasonic Sensor Pins (Rear)
#define US_REAR_TRIG 26
#define US_REAR_ECHO 27

// UART Communication
#define UART_MASTER_TX 16 // To ESP32 #1
#define UART_MASTER_RX 17 // From ESP32 #1

#endif

#ifdef CAMERA_MODULE
// ESP32-CAM Pins

// Camera pins are internally connected
// Only define additional pins

#define FLASH_LED 4 // Flash LED control

// UART Communication
#define UART_MASTER_TX 14 // To ESP32 #1
#define UART_MASTER_RX 15 // From ESP32 #1

// Camera Configuration Pins (Internal)
#define CAM_PIN_PWDN 32
#define CAM_PIN_RESET -1
#define CAM_PIN_XCLK 0
#define CAM_PIN_SIOD 26
#define CAM_PIN_SIOC 27

#define CAM_PIN_D7 35
#define CAM_PIN_D6 34
#define CAM_PIN_D5 39
#define CAM_PIN_D4 36
#define CAM_PIN_D3 21
#define CAM_PIN_D2 19
#define CAM_PIN_D1 18
#define CAM_PIN_D0 5
#define CAM_PIN_VSYNC 25
#define CAM_PIN_HREF 23
#define CAM_PIN_PCLK 22

#endif

// PWM Channels
#define PWM_CHANNEL_FL 0
#define PWM_CHANNEL_FR 1
#define PWM_CHANNEL_RL 2
#define PWM_CHANNEL_RR 3
#define PWM_CHANNEL_BUZZER 4

// On-board status LED definitions
#ifdef CAMERA_MODULE
// ESP32-CAM AI-Thinker uses GPIO33 for the on-board LED
#ifndef LED_BUILTIN
#define LED_BUILTIN 33
#endif
#else
// Default ESP32 DevKit V1 on-board LED
#ifndef LED_BUILTIN
#define LED_BUILTIN 2
#endif
#endif

#endif // PINS_H