#ifndef CONFIG_H
#define CONFIG_H

// ============================================
// GLOBAL CONFIGURATION
// ============================================

// Network Configuration
#define WIFI_SSID "ProjectNightfall"
#define WIFI_PASSWORD "rescue2025"
#define CAMERA_STREAM_PORT 81

// Communication Settings
#define UART_BAUD_RATE 115200
#define COMM_UPDATE_RATE 50     // ms between updates
#define HEARTBEAT_INTERVAL 1000 // ms between heartbeats

// Motor Settings
#define MOTOR_PWM_FREQ 5000       // Hz
#define MOTOR_PWM_RESOLUTION 8    // 8-bit (0-255)
#define MOTOR_NORMAL_SPEED 180    // Normal cruising speed
#define MOTOR_CLIMB_SPEED 255     // Maximum climbing speed
#define MOTOR_TURN_SPEED 150      // Speed during turns
#define CLIMB_BOOST_DURATION 2000 // ms to boost during climb

// Sensor Settings
#define ULTRASONIC_TIMEOUT 30000  // microseconds
#define SAFE_DISTANCE 20          // cm minimum safe distance
#define OBSTACLE_THRESHOLD 30     // cm to trigger avoidance
#define CLIMB_DETECT_THRESHOLD 10 // cm sudden drop indicating obstacle

#define GAS_THRESHOLD_ANALOG 400 // Analog threshold for gas detection
#define GAS_SAMPLE_INTERVAL 500  // ms between gas readings

// Navigation Settings
#define TURN_DURATION 500          // ms for 90-degree turn
#define BACKUP_DURATION 1000       // ms to backup when stuck
#define STUCK_THRESHOLD 5          // Consecutive blocked readings
#define ROTATION_360_DURATION 2000 // ms for full 360 rotation

// Safety Settings
#define EMERGENCY_STOP_DISTANCE 10 // cm for immediate stop
#define MAX_TILT_ANGLE 45          // degrees max safe tilt
#define LOW_BATTERY_VOLTAGE 12.0   // V threshold for low battery
#define WATCHDOG_TIMEOUT 5000      // ms before watchdog reset

// Camera Settings
#define CAMERA_FRAME_SIZE FRAMESIZE_VGA // 640x480
#define CAMERA_JPEG_QUALITY 12          // 0-63, lower = higher quality
#define CAMERA_FB_COUNT 1               // Frame buffers

// Debug Settings
#ifdef SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

// System States
enum RobotState
{
    STATE_INIT,
    STATE_IDLE,
    STATE_AUTONOMOUS,
    STATE_MANUAL,
    STATE_EMERGENCY,
    STATE_CLIMBING,
    STATE_TURNING,
    STATE_AVOIDING
};

// Movement Commands
enum MovementCommand
{
    CMD_STOP,
    CMD_FORWARD,
    CMD_BACKWARD,
    CMD_TURN_LEFT,
    CMD_TURN_RIGHT,
    CMD_ROTATE_360,
    CMD_CLIMB_BOOST
};

#endif // CONFIG_H