#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// PROJECT NIGHTFALL - REFACTORED CONFIG
// ============================================

// WiFi Configuration
#define WIFI_SSID "ProjectNightfall"
#define WIFI_PASSWORD "rescue2025"
#define WIFI_SERVER_PORT 8888
#define WIFI_CAMERA_PORT 81

// Serial Configuration
#define SERIAL_BAUD_RATE 115200

// Motor Settings
#define MOTOR_PWM_FREQ 5000         // Hz
#define MOTOR_PWM_RESOLUTION 8      // 8-bit (0-255)
#define MOTOR_NORMAL_SPEED 180      // Normal cruising speed
#define MOTOR_CLIMB_SPEED 255       // Maximum climbing speed
#define MOTOR_TURN_SPEED 150        // Speed during turns
#define MOTOR_BACK_NORMAL_SPEED 150 // Rear motor default speed

// Sensor Settings
#define ULTRASONIC_THRESHOLD_SAFE 30     // cm - safe distance
#define ULTRASONIC_THRESHOLD_OBSTACLE 20 // cm - obstacle detected
#define ULTRASONIC_THRESHOLD_CLIFF 10    // cm - cliff/climbable edge

#define GAS_THRESHOLD_ALERT 400     // Analog value threshold (0-4095)
#define GAS_THRESHOLD_EMERGENCY 500 // Emergency threshold

// Safety & Navigation
#define ENABLE_AUTONOMOUS 1               // Toggle autonomous mode
#define NAVIGATION_UPDATE_INTERVAL_MS 200 // ms between nav updates
#define SENSOR_UPDATE_INTERVAL_MS 100     // ms between sensor reads
#define TELEMETRY_INTERVAL_MS 500         // ms between telemetry broadcasts

// Buzzer Settings
#define BUZZER_FREQUENCY 2000        // Hz
#define BUZZER_ALERT_DURATION_MS 100 // ms per alert pulse

// Watchdog & Timing
#define MAIN_LOOP_RATE_MS 50 // Main loop target frequency (20 Hz)
#define WATCHDOG_TIMEOUT_MS 5000

// Debug Settings
#define ENABLE_SERIAL_DEBUG 1

#if ENABLE_SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#define DEBUG_PRINTF(fmt, ...) Serial.printf(fmt, ##__VA_ARGS__)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTF(fmt, ...)
#endif

// ============================================
// STATE ENUMS
// ============================================

enum RobotState
{
    STATE_INIT,
    STATE_IDLE,
    STATE_AUTONOMOUS,
    STATE_MANUAL,
    STATE_EMERGENCY,
    STATE_ERROR
};

enum NavigationState
{
    NAV_FORWARD,
    NAV_OBSTACLE_DETECTED,
    NAV_AVOID_LEFT,
    NAV_AVOID_RIGHT,
    NAV_BACKING_UP,
    NAV_CLIMBING,
    NAV_STUCK,
    NAV_IDLE
};

#endif // CONFIG_H
