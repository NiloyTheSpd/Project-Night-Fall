#ifndef PINS_H
#define PINS_H

// ============================================
// PROJECT NIGHTFALL - COMPLETE PIN DEFINITIONS
// Three-Board ESP32 System with L298N Motor Drivers
// ============================================
//
// IMPORTANT GPIO RESTRICTIONS (ESP32):
// - GPIO 0: Boot mode (pulled HIGH). Avoid using for critical outputs.
// - GPIO 1/3: TX0/RX0 (Serial). Reserved for programming/debugging.
// - GPIO 6-11: Connected to flash (DO NOT USE).
// - GPIO 12: Boot voltage selector (low at boot for 3.3V flash). Use with caution.
// - GPIO 15: Debug output at boot (outputs PWM signal). Avoid for sensitive inputs.
// - GPIO 34-39: Input only (no pullup/pulldown, ADC1 capable).
//
// PWM-CAPABLE PINS (ESP32): Any GPIO except 6-11, 34-39
// ADC1 (WiFi safe): 32, 33, 34, 35, 36, 39
// ADC2 (NOT WiFi safe): 0, 2, 4, 12-15, 25-27
//
// ============================================

// ============================================
// BACK ESP32 (MASTER/SENSOR NODE)
// Role: Navigation brain, sensor processing, rear traction
// ============================================
#ifdef BACK_CONTROLLER

// ----- L298N Rear Motor Driver -----
// Controls 2 DC motors (left/right rear wheels)
// Wiring: L298N → ESP32
//   OUT1/OUT2 → Rear Left Motor
//   OUT3/OUT4 → Rear Right Motor
//   +12V → Battery +12V, GND → Battery GND
//   +5V → ESP32 VIN (or separate 5V regulator)

// Left Motor (OUT1/OUT2)
#define MOTOR_REAR_LEFT_ENA 13 // PWM speed control (0-255) - PWM capable
#define MOTOR_REAR_LEFT_IN1 12 // Direction bit A - Safe GPIO
#define MOTOR_REAR_LEFT_IN2 14 // Direction bit B - PWM capable
// Logic: IN1=HIGH, IN2=LOW → Forward | IN1=LOW, IN2=HIGH → Reverse

// Right Motor (OUT3/OUT4)
#define MOTOR_REAR_RIGHT_ENB 25 // PWM speed control (0-255) - PWM capable, ADC2
#define MOTOR_REAR_RIGHT_IN3 26 // Direction bit A - PWM capable, ADC2
#define MOTOR_REAR_RIGHT_IN4 27 // Direction bit B - PWM capable, ADC2
// Logic: IN3=HIGH, IN4=LOW → Forward | IN3=LOW, IN4=HIGH → Reverse

// ----- Ultrasonic Sensors (HC-SR04) -----
// Wiring per sensor: VCC → 5V, GND → GND, TRIG → GPIO, ECHO → GPIO
// Note: ECHO outputs 5V. Use voltage divider (1kΩ + 2kΩ) or level shifter
// if ESP32 is strict 3.3V. Most ESP32 GPIO tolerate 5V on input.

// Front Ultrasonic (obstacle detection)
#define ULTRASONIC_FRONT_TRIG 5  // Trigger pulse output - Safe GPIO, PWM capable
#define ULTRASONIC_FRONT_ECHO 18 // Echo pulse input - Safe GPIO, PWM capable
// Trigger: 10μs HIGH pulse. Echo: HIGH duration = distance (58μs/cm)

// Rear Ultrasonic (cliff/backup detection)
#define ULTRASONIC_REAR_TRIG 19 // Trigger pulse output - Safe GPIO, PWM capable
#define ULTRASONIC_REAR_ECHO 21 // Echo pulse input - Safe GPIO, PWM capable

// ----- MQ-2 Gas/Smoke Sensor -----
// Wiring: VCC → 5V, GND → GND, A0 → ESP32 ADC, D0 → Digital alert (optional)
// Note: Requires warm-up (30-60s). Analog value increases with gas concentration.

#define GAS_SENSOR_ANALOG 32  // Analog reading (0-4095) - ADC1 channel 4, WiFi-safe
#define GAS_SENSOR_DIGITAL 33 // Digital alert threshold (optional) - ADC1 channel 5
// ADC1 is WiFi-safe. Use analogRead() for raw value, map to threshold.

// ----- Buzzer/Alarm -----
// Wiring: Buzzer+ → GPIO via 220Ω resistor, Buzzer- → GND
// Active buzzer: digitalWrite HIGH/LOW. Passive: Use PWM tone.

#define BUZZER_PIN 2 // Alarm output - Safe GPIO, PWM capable
// GPIO 2: Onboard LED on some boards. Can dual-purpose as status + buzzer.

// ----- Status LED (Optional) -----
#define STATUS_LED_PIN 23 // Optional status LED - Safe GPIO, PWM capable
// Wiring: LED+ → GPIO via 220Ω → GND. Use for heartbeat/error indication.

// ----- PWM Channel Assignments -----
// ESP32 has 16 independent PWM channels (0-15). Assign channels to avoid conflicts.
#define PWM_CHANNEL_REAR_LEFT 0  // Left motor speed
#define PWM_CHANNEL_REAR_RIGHT 1 // Right motor speed
#define PWM_CHANNEL_BUZZER 2     // Buzzer tone (if passive)
#define PWM_CHANNEL_STATUS_LED 3 // Status LED fade (optional)

// ----- Reserved/Unused Pins -----
// GPIO 1, 3: Serial TX/RX (keep free for debugging)
// GPIO 6-11: Flash memory (NEVER USE)
// GPIO 15: Outputs PWM at boot (avoid for inputs)

#endif // BACK_CONTROLLER

// ============================================
// FRONT ESP32 (MOTOR SLAVE NODE)
// Role: 4-motor traction control, receives commands via WebSocket
// ============================================
#ifdef FRONT_CONTROLLER

// ----- L298N Motor Driver #1 (Front Left Bank) -----
// Controls 2 DC motors (front-left and front-right-1)
// Wiring: L298N #1 → ESP32
//   OUT1/OUT2 → Front Left Motor 1
//   OUT3/OUT4 → Front Right Motor 1
//   +12V → Battery +12V, GND → Battery GND
//   +5V → ESP32 VIN (or separate 5V regulator)

// Motor 1: Front Left 1 (OUT1/OUT2)
#define MOTOR_FRONT_LEFT1_ENA 13 // PWM speed control - PWM capable
#define MOTOR_FRONT_LEFT1_IN1 12 // Direction bit A - Safe GPIO
#define MOTOR_FRONT_LEFT1_IN2 14 // Direction bit B - PWM capable
// Logic: IN1=HIGH, IN2=LOW → Forward | IN1=LOW, IN2=HIGH → Reverse

// Motor 2: Front Right 1 (OUT3/OUT4)
#define MOTOR_FRONT_RIGHT1_ENB 25 // PWM speed control - PWM capable, ADC2
#define MOTOR_FRONT_RIGHT1_IN3 26 // Direction bit A - PWM capable, ADC2
#define MOTOR_FRONT_RIGHT1_IN4 27 // Direction bit B - PWM capable, ADC2

// ----- L298N Motor Driver #2 (Front Right Bank) -----
// Controls 2 DC motors (front-left-2 and front-right-2)
// Wiring: L298N #2 → ESP32
//   OUT1/OUT2 → Front Left Motor 2
//   OUT3/OUT4 → Front Right Motor 2
//   +12V → Battery +12V, GND → Battery GND
//   +5V → ESP32 VIN (or separate 5V regulator)

// Motor 3: Front Left 2 (OUT1/OUT2)
#define MOTOR_FRONT_LEFT2_ENA 4  // PWM speed control - Safe GPIO, PWM capable, ADC2
#define MOTOR_FRONT_LEFT2_IN1 16 // Direction bit A - Safe GPIO, PWM capable
#define MOTOR_FRONT_LEFT2_IN2 17 // Direction bit B - Safe GPIO, PWM capable
// Note: GPIO 4 also used on ESP32-CAM. No conflict since different boards.

// Motor 4: Front Right 2 (OUT3/OUT4)
#define MOTOR_FRONT_RIGHT2_ENB 18 // PWM speed control - Safe GPIO, PWM capable
#define MOTOR_FRONT_RIGHT2_IN3 19 // Direction bit A - Safe GPIO, PWM capable
#define MOTOR_FRONT_RIGHT2_IN4 21 // Direction bit B - Safe GPIO, PWM capable

// ----- Status LED (Optional) -----
#define STATUS_LED_PIN 23 // Optional heartbeat LED - Safe GPIO, PWM capable
// Wiring: LED+ → GPIO 23 via 220Ω → GND

// ----- PWM Channel Assignments -----
// Assign 4 channels for motor speed control
#define PWM_CHANNEL_FRONT_LEFT1 0  // Motor 1 speed
#define PWM_CHANNEL_FRONT_RIGHT1 1 // Motor 2 speed
#define PWM_CHANNEL_FRONT_LEFT2 2  // Motor 3 speed
#define PWM_CHANNEL_FRONT_RIGHT2 3 // Motor 4 speed
#define PWM_CHANNEL_STATUS_LED 4   // Status LED (optional)

// ----- Motor Array Mapping (for code clarity) -----
// Use in loops: for(int i=0; i<4; i++) { motors[i].setSpeed(...); }
// Index 0: Front Left 1, 1: Front Right 1, 2: Front Left 2, 3: Front Right 2

#endif // FRONT_CONTROLLER

// ============================================
// ESP32-CAM (TELEMETRY BRIDGE + VISION)
// Role: WebSocket client, WebSocket server, camera streaming
// ============================================
#ifdef CAMERA_CONTROLLER

// ----- Built-in Camera Module (OV2640) -----
// ESP32-CAM AI-Thinker module has hardwired camera pins (internal).
// Do NOT reassign these GPIOs:
//   Y2_GPIO_NUM   → GPIO 5
//   Y3_GPIO_NUM   → GPIO 18
//   Y4_GPIO_NUM   → GPIO 19
//   Y5_GPIO_NUM   → GPIO 21
//   Y6_GPIO_NUM   → GPIO 36
//   Y7_GPIO_NUM   → GPIO 39
//   Y8_GPIO_NUM   → GPIO 34
//   Y9_GPIO_NUM   → GPIO 35
//   XCLK_GPIO_NUM → GPIO 0
//   PCLK_GPIO_NUM → GPIO 22
//   VSYNC_GPIO_NUM → GPIO 25
//   HREF_GPIO_NUM → GPIO 23
//   SIOD_GPIO_NUM → GPIO 26 (I2C SDA)
//   SIOC_GPIO_NUM → GPIO 27 (I2C SCL)
//   PWDN_GPIO_NUM → GPIO 32 (power down, active HIGH)
//   RESET_GPIO_NUM → -1 (not used)
// Camera config handled in camera driver. No manual pin setup needed.

// ----- LED Flash (White LED on GPIO 4) -----
#define FLASH_LED_PIN 4 // Built-in white LED - PWM capable for brightness
// Wiring: Already on board. digitalWrite(FLASH_LED_PIN, HIGH) to turn on.
// Can use PWM for dimming: ledcWrite(channel, 0-255).

// ----- microSD Card (Optional) -----
// ESP32-CAM supports SD_MMC (4-bit or 1-bit mode).
// 1-bit mode uses: CMD=GPIO15, CLK=GPIO14, DATA0=GPIO2
// 4-bit mode adds: DATA1=GPIO4, DATA2=GPIO12, DATA3=GPIO13
// Note: GPIO 4 conflicts with FLASH_LED. Use 1-bit mode if flash needed.
// Initialized via SD_MMC.begin("/sdcard", true) in code.

#define SD_MMC_CMD 15  // SD card command - Reserved for SD_MMC
#define SD_MMC_CLK 14  // SD card clock - Reserved for SD_MMC
#define SD_MMC_DATA0 2 // SD card data - Reserved for SD_MMC (1-bit mode)
// GPIO 2: Also onboard LED on some ESP32 boards. Shared with SD_MMC.

// ----- Status LED (GPIO 33 - Safe Alternative) -----
#define STATUS_LED_PIN 33 // External status LED - Safe GPIO, ADC1
// Wiring: LED+ → GPIO 33 via 220Ω → GND
// Use for ESP-NOW activity, WiFi status, etc.

// ----- Available Free GPIOs (if not using camera/SD) -----
// GPIO 1, 3: Serial (TX0/RX0) - Keep for debugging
// GPIO 12, 13: Available if not using SD_MMC 4-bit mode
// GPIO 16: RX2 (UART2) - Available
// GPIO 17: TX2 (UART2) - Available
// Note: Most GPIOs used by camera. Limited expansion.

// ----- PWM Channel Assignments -----
#define PWM_CHANNEL_FLASH_LED 0  // Flash LED brightness control
#define PWM_CHANNEL_STATUS_LED 1 // Status LED (optional)

// ----- Network Configuration -----
// WiFi and ESP-NOW handled in firmware (config.h).
// ESP32-CAM typically runs as WiFi AP or STA mode.
// WebSocket server on port 8888 (defined in config.h).

#endif // CAMERA_CONTROLLER

// ============================================
// GLOBAL NOTES
// ============================================
//
// Power Supply Recommendations:
// - ESP32: 3.3V logic, 5V VIN (onboard regulator)
// - L298N: 5-35V motor supply, 5V logic (from onboard 5V out or external)
// - HC-SR04: 5V VCC (ECHO may be 5V, use level shifter if needed)
// - MQ-2: 5V VCC (outputs 0-5V analog, within ESP32 ADC range)
// - Total current: ~2-3A for ESP32s, 5A+ for motors (use separate battery)
//
// Wiring Safety:
// - Always connect ESP32 GND to motor driver GND (common ground)
// - Use decoupling capacitors (100μF) near motor power inputs
// - Fuse motor battery (5-10A fast-blow fuse)
// - Use diodes (1N4007) across motor terminals for back-EMF protection
//
// Debugging Tips:
// - Use Serial.print() on TX0/RX0 (GPIO 1/3) for diagnostics
// - Monitor PWM signals with oscilloscope or LED indicators
// - Check voltage levels with multimeter (3.3V logic, 5V sensors, 12V motors)
// - Verify GPIO states at boot (some pins have pull-up/down resistors)
//
// ============================================

#endif // PINS_H
