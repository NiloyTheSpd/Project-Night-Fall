# Project Nightfall - Quick Reference Guide

## File Overview

### Main Files (src/)

| File              | Purpose                           | Boards       | Lines |
| ----------------- | --------------------------------- | ------------ | ----- |
| `main.cpp`        | Dual-board control (Front & Rear) | ESP32 #1, #2 | 812   |
| `main_camera.cpp` | Vision streaming module           | ESP32-CAM    | 662   |

### Library Files (lib/)

| Library            | Purpose                   | Key Classes                                       |
| ------------------ | ------------------------- | ------------------------------------------------- |
| `MotorControl`     | L298N driver control      | `MotorControl::forward()`, `backward()`, `stop()` |
| `UltrasonicSensor` | Distance measurement      | `getDistance()`, `getAverageDistance()`           |
| `GasSensor`        | Gas/smoke detection       | `isDetected()`, `getAnalogValue()`, `getPPM()`    |
| `SafetyMonitor`    | Safety system             | `isSafe()`, `isEmergency()`, `raiseAlert()`       |
| `AutonomousNav`    | Movement logic            | `getNextMove()`, `getCurrentState()`              |
| `UARTComm`         | Inter-board communication | `sendMessage()`, `receiveMessage()`               |
| `WaypointNav`      | Path planning             | `addWaypoint()`, `startMission()`                 |

---

## Quick Start

### Option 1: Compile for Front Controller

```bash
# Edit platformio.ini or main.cpp
# Uncomment: #define FRONT_CONTROLLER 1
# Comment: //#define REAR_CONTROLLER 1

pio run -e front_esp32 -t upload
```

### Option 2: Compile for Rear Controller

```bash
# Edit platformio.ini or main.cpp
# Uncomment: #define REAR_CONTROLLER 1
# Comment: //#define FRONT_CONTROLLER 1

pio run -e rear_esp32 -t upload
```

### Option 3: Compile for Camera

```bash
pio run -e camera_esp32cam -t upload
```

---

## Control Commands

### Serial Console Commands (Front Controller)

```
forward     → Move forward at normal speed
backward    → Reverse at normal speed
left        → Turn left
right       → Turn right
stop        → Stop motors
auto        → Start autonomous navigation
estop       → Emergency stop
status      → Print system status
```

### Camera Commands (UART from Front)

```
flash_on    → Enable LED flash
flash_off   → Disable LED flash
capture     → Capture single frame
status      → Print camera status
```

### WiFi Access (Camera)

```
SSID: ProjectNightfall
Password: rescue2025
URL: http://192.168.4.1:81/stream
```

---

## GPIO Pin Mapping

### Front Controller (ESP32 #1)

```
Motor Control (L298N):
  - Left Motor Enable (PWM):  GPIO18 (ENA)
  - Left Motor Dir A:         GPIO19 (IN1)
  - Left Motor Dir B:         GPIO21 (IN2)
  - Right Motor Enable (PWM): GPIO22 (ENB)
  - Right Motor Dir A:        GPIO23 (IN3)
  - Right Motor Dir B:        GPIO25 (IN4)

Ultrasonic (Front):
  - Trigger:  GPIO26
  - Echo:     GPIO27

Gas Sensor:
  - Analog:   GPIO32
  - Digital:  GPIO33

UART:
  - To Rear TX:    GPIO16
  - From Rear RX:  GPIO17
  - To Camera TX:  GPIO14
  - From Camera RX: GPIO12

Other:
  - Buzzer: GPIO13
```

### Rear Controller (ESP32 #2)

```
Motor Control (L298N):
  - Left Motor Enable (PWM):  GPIO18 (ENA)
  - Left Motor Dir A:         GPIO19 (IN1)
  - Left Motor Dir B:         GPIO21 (IN2)
  - Right Motor Enable (PWM): GPIO22 (ENB)
  - Right Motor Dir A:        GPIO23 (IN3)
  - Right Motor Dir B:        GPIO25 (IN4)

Ultrasonic (Rear):
  - Trigger:  GPIO26
  - Echo:     GPIO27

UART:
  - To Front TX:   GPIO16
  - From Front RX: GPIO17
```

### Camera Module (ESP32-CAM)

```
Camera Pins (OV2640):
  - PWDN:   GPIO32
  - RESET:  GPIO-1 (none)
  - XCLK:   GPIO0
  - SIOD:   GPIO26
  - SIOC:   GPIO27
  - D0-D7:  GPIO 5,18,19,21,36,39,34,35
  - VSYNC:  GPIO25
  - HREF:   GPIO23
  - PCLK:   GPIO22

UART:
  - To Master TX:   GPIO14
  - From Master RX: GPIO15

Other:
  - Flash LED: GPIO4
```

---

## Update Frequencies

### Front Controller

```
Sensor Updates:      10 Hz (100ms)
Safety Checks:        5 Hz (200ms)
Navigation Updates:   2 Hz (500ms) - autonomous only
Gas Readings:         2 Hz (500ms)
UART Communication: Continuous
Heartbeat:            1 Hz (1000ms)
```

### Rear Controller

```
Sensor Updates:      10 Hz (100ms)
Safety Checks:        5 Hz (200ms)
UART Communication: Continuous
Heartbeat Check:   Continuous (3 sec timeout)
```

### Camera

```
MJPEG Streaming:   Continuous (limited by WiFi/USB)
Status Updates:       5 Hz (200ms)
Heartbeat ACK:     On reception (1Hz from Front)
```

---

## State Machine Diagram

### Robot States

```
STATE_INIT → STATE_IDLE ↔ STATE_MANUAL
             ↓
           STATE_AUTONOMOUS
             ↓
           STATE_TURNING, STATE_CLIMBING, STATE_AVOIDING

Any State → STATE_EMERGENCY (from SafetyMonitor)
             ↓
           Manual reset required
```

### Navigation States

```
NAV_FORWARD → [Obstacle Detected]
              ↓
           NAV_AVOIDING
              ↓
           NAV_TURNING_LEFT or NAV_TURNING_RIGHT
              ↓
           NAV_SCANNING
              ↓
           NAV_FORWARD (resume)

OR

NAV_FORWARD → [Climbable Obstacle]
              ↓
           NAV_CLIMBING
              ↓
           [Check if climbed]
              ↓
           NAV_FORWARD (resume)

OR

NAV_FORWARD → [Stuck]
              ↓
           NAV_STUCK
              ↓
           NAV_ROTATING (360 degrees)
              ↓
           NAV_FORWARD (resume)
```

---

## Safety Thresholds (config.h)

```cpp
#define SAFE_DISTANCE 20 cm              // Min safe distance before action
#define OBSTACLE_THRESHOLD 30 cm         // Distance to trigger avoidance
#define EMERGENCY_STOP_DISTANCE 10 cm    // Immediate stop distance
#define CLIMB_DETECT_THRESHOLD 10 cm     // Sudden drop = climbable

#define GAS_THRESHOLD_ANALOG 400         // Analog threshold
#define MAX_TILT_ANGLE 45°               // Max safe tilt
#define LOW_BATTERY_VOLTAGE 12.0 V       // Low battery alert

#define WATCHDOG_TIMEOUT 5000 ms         // ESP32 watchdog
#define HEARTBEAT_INTERVAL 1000 ms       // Master heartbeat
#define MASTER_TIMEOUT 3000 ms           // Rear waits for master
```

---

## Motor Speed Mapping

```cpp
Speed Value    Performance
0-50          Creep (very slow, high torque)
50-150        Careful (obstacle approach)
150-180       Normal (default cruising)
180-230       Fast (open area)
230-255       Maximum (emergency climb)

Predefined Constants:
MOTOR_NORMAL_SPEED  = 180  // Regular movement
MOTOR_CLIMB_SPEED   = 255  // Maximum power climb
MOTOR_TURN_SPEED    = 150  // Controlled turning
```

---

## JSON Message Reference

### Message Types

| Type              | Direction      | Frequency  | Purpose                  |
| ----------------- | -------------- | ---------- | ------------------------ |
| `heartbeat`       | Front→Rear/Cam | 1Hz        | System alive signal      |
| `sensor_update`   | Front→Rear     | 10Hz       | Sensor data sharing      |
| `sensor_feedback` | Rear→Front     | 10Hz       | Rear sensor data         |
| `motor_command`   | Front→Rear     | 2Hz        | Movement commands        |
| `status`          | Cam→Front      | 5Hz        | Camera status report     |
| `heartbeat_ack`   | Cam→Front      | 1Hz        | Heartbeat acknowledge    |
| `command`         | Front→Cam      | On demand  | Flash/capture control    |
| `emergency`       | Front→All      | On trigger | Emergency stop broadcast |

### Message Size Limits

```
Sensor Update:    ~256 bytes
Motor Command:    ~256 bytes
Heartbeat:        ~300 bytes
Camera Status:    ~512 bytes
Max UART Buffer:  ~1024 bytes
```

---

## Debugging Checklist

### System Won't Start

- [ ] Watchdog timeout? Add esp_task_wdt_reset()
- [ ] UART initialization? Check baud rate = 115200
- [ ] Motor driver power? Check 5V supply
- [ ] GPIO conflicts? Review pin_definitions

### Motors Not Responding

- [ ] Is motor driver powered? (Check LED)
- [ ] Are pins configured correctly?
- [ ] Is emergencyStopTriggered true?
- [ ] Try manual: frontMotors.forward(200);

### Sensors Not Reading

- [ ] Pin wiring correct?
- [ ] Timeout values appropriate?
- [ ] Sensor power supplied?
- [ ] Try single read vs average

### Communication Broken

- [ ] UART TX/RX swapped?
- [ ] Baud rate mismatch?
- [ ] JSON parse failing? (check format)
- [ ] 3-second heartbeat timeout? (rear only)

### Camera Streaming Issues

- [ ] WiFi AP started? (check serial output)
- [ ] Client connected to AP?
- [ ] Browser supports multipart MJPEG?
- [ ] Check FPS and dropped frames

---

## Performance Optimization Tips

1. **Reduce UART Message Frequency**

   - Sensor updates: 10Hz → 5Hz
   - Status updates: 5Hz → 2Hz

2. **Optimize Navigation Loop**

   - 2Hz is safe, can reduce to 1Hz for slower navigation
   - Increase sensor averaging from 3 to 5 samples

3. **Camera Streaming**

   - Reduce JPEG quality (12 → 16) for faster encoding
   - Reduce frame size (VGA → QVGA 320x240)
   - Limit WiFi clients to 1

4. **Memory**

   - Remove unused library includes
   - Use const char\* instead of String where possible
   - Reduce StaticJsonDocument sizes

5. **Power**
   - Disable SERIAL_DEBUG in production
   - Reduce PWM frequency if EMI acceptable
   - Use lower motor speeds in idle state

---

## Typical Startup Sequence

### Front Controller Boot

```
1. Serial begin (115200)
2. Watchdog init (5 sec)
3. Motor drivers init
4. Ultrasonic init
5. Gas sensor init
6. Safety monitor init
7. Autonomous nav init
8. UART to Rear init
9. UART to Camera init
10. LED signal ready
11. STATE = IDLE
12. Await manual commands
```

**Timing**: ~2-3 seconds

### Rear Controller Boot

```
1. Serial begin (115200)
2. Watchdog init (5 sec)
3. Motor drivers init
4. Ultrasonic init
5. Safety monitor init
6. UART to Master init
7. LED signal ready
8. STATE = IDLE
9. Await heartbeat from Front
```

**Timing**: ~1-2 seconds

### Camera Boot

```
1. Serial begin (115200)
2. Watchdog init (5 sec)
3. Camera init + sensor config
4. WiFi AP start
5. Streaming server start
6. UART to Master init
7. LED signal ready
8. Await clients / commands
```

**Timing**: ~3-5 seconds (slowest)

---

## Common Modifications

### Change Motor Speed

```cpp
// In config.h
#define MOTOR_NORMAL_SPEED 200  // Was 180
#define MOTOR_TURN_SPEED 180    // Was 150
```

### Adjust Obstacle Detection

```cpp
// In config.h
#define SAFE_DISTANCE 25        // Was 20 (more cautious)
#define OBSTACLE_THRESHOLD 35   // Was 30
```

### Disable Gas Detection

```cpp
// In main.cpp, comment out:
// gasSensor.begin();
// gasSensor.update();
// if (gasSensor.isDetected()) { ... }
```

### Change Camera Resolution

```cpp
// In config.h
#define CAMERA_FRAME_SIZE FRAMESIZE_QVGA  // 320x240 (was VGA)
```

### Increase Heartbeat Frequency

```cpp
// In main.cpp, change:
if (currentTime - lastHeartbeat >= 500)  // Was 1000 (2Hz)
```

---

## References & Resources

### Hardware

- ESP32 Datasheet: https://espressif.com
- L298N Motor Driver: Standard 2-channel, dual H-bridge
- OV2640 Camera: Common ESP32-CAM sensor
- HC-SR04 Ultrasonic: Standard 4-pin ultrasonic module

### Libraries

- ArduinoJson: https://arduinojson.org
- ESP32 Arduino Core: https://github.com/espressif/arduino-esp32

### PlatformIO

- Documentation: https://docs.platformio.org
- IDE: VS Code extension: `platformio.platformio-ide`

---

_Quick Reference - Project Nightfall_
_Complete Implementation Guide available in IMPLEMENTATION_GUIDE.md_
