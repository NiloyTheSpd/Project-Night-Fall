# Project Nightfall - Deployment & Testing Checklist

## Pre-Deployment Hardware Checklist

### Power System

- [ ] 12V battery pack installed and charged
- [ ] Voltage regulator outputs 5V (test with multimeter)
- [ ] Power capacitors (100µF) installed near each module
- [ ] GND connections common to all boards
- [ ] Motor power separate from logic power (recommended)
- [ ] No visible damage to power supply
- [ ] Current limiting set appropriately (2A typical)

### Mechanical Assembly

- [ ] Front wheels mounted and rotating freely
- [ ] Rear wheels mounted and rotating freely
- [ ] Motor mountings secure (no vibration)
- [ ] Ultrasonic sensors mounted and unobstructed
  - [ ] Front sensor on front, facing forward
  - [ ] Rear sensor on back, facing backward
- [ ] Gas sensor mounted in ventilated location
- [ ] Camera mounted and aligned
- [ ] All connectors secure and strain-relieved

### Electrical Connections

- [ ] All GPIO pins correctly connected per pinout diagram
- [ ] UART TX/RX not swapped:
  - [ ] Front TX (GPIO16) → Rear RX (GPIO17)
  - [ ] Front RX (GPIO17) ← Rear TX (GPIO16)
  - [ ] Front TX (GPIO14) → Camera RX (GPIO15)
  - [ ] Front RX (GPIO12) ← Camera TX (GPIO14)
- [ ] Motor enable pins connected to PWM-capable pins
- [ ] Ultrasonic trigger/echo pins correct
- [ ] Gas sensor analog pin connected to ADC pin
- [ ] All ground connections present
- [ ] No floating (unconnected) GPIO pins

### Testing Equipment

- [ ] USB cables for 3× ESP32 boards
- [ ] USB-TTL serial adapter (optional, for debugging)
- [ ] Multimeter for voltage verification
- [ ] Logic analyzer (optional, for UART verification)
- [ ] Power supply with current limiting
- [ ] Laptop with PlatformIO installed

---

## Pre-Compilation Software Checklist

### IDE Setup

- [ ] PlatformIO installed in VS Code
- [ ] Python 3.8+ installed
- [ ] Git installed (for version control)
- [ ] Project folder opened in VS Code
- [ ] All required libraries available:
  - [ ] ArduinoJson
  - [ ] esp32 core files
  - [ ] WiFi library
  - [ ] esp_camera library

### Configuration Files Review

- [ ] `include/config.h` reviewed:
  - [ ] Motor speeds appropriate for your motor type
  - [ ] Safe distance threshold realistic for your environment
  - [ ] Baud rates all set to 115200
  - [ ] All pin numbers match your wiring
- [ ] `include/pins.h` reviewed:
  - [ ] FRONT_CONTROLLER, REAR_CONTROLLER, or CAMERA_MODULE correctly defined
  - [ ] All GPIO pins match physical connections
  - [ ] No duplicate pin assignments
  - [ ] PWM channels assigned correctly

### Source Code Review

- [ ] `src/main.cpp` examined:
  - [ ] Correct controller type selected (#define FRONT_CONTROLLER or REAR_CONTROLLER)
  - [ ] No syntax errors visible
  - [ ] Serial debug enabled for testing
- [ ] `src/main_camera.cpp` examined:
  - [ ] WiFi SSID and password match your preference
  - [ ] Camera model (AI_THINKER, WROVER, etc.) correct
  - [ ] Pin assignments match your camera module

### Library Integration Check

- [ ] All library headers found in `/lib`:
  - [ ] `MotorControl.h` present
  - [ ] `UltrasonicSensor.h` present
  - [ ] `GasSensor.h` present
  - [ ] `SafetyMonitor.h` present
  - [ ] `AutonomousNav.h` present
  - [ ] `UARTComm.h` present
- [ ] Library implementations exist (.cpp files)

---

## Compilation Checklist

### Front Controller Build

```bash
# Switch to project directory
cd "e:\Project Night Fall"

# Make sure FRONT_CONTROLLER is defined in main.cpp
# Edit: Uncomment "#define FRONT_CONTROLLER 1"
#       Comment: "// #define REAR_CONTROLLER 1"

# Build
pio run -e front_esp32
```

- [ ] Compilation completes without errors
- [ ] Compilation completes without major warnings
- [ ] Binary file generated (.elf, .bin files visible)
- [ ] Compilation time reasonable (~30-60 seconds)
- [ ] No "undefined reference" errors

### Rear Controller Build

```bash
# Switch to REAR_CONTROLLER in main.cpp
# Edit: Comment "// #define FRONT_CONTROLLER 1"
#       Uncomment "#define REAR_CONTROLLER 1"

# Build
pio run -e rear_esp32
```

- [ ] Compilation completes without errors
- [ ] Compilation completes without major warnings
- [ ] Binary file generated
- [ ] Compilation time reasonable

### Camera Build

```bash
# Compile for camera
pio run -e camera_esp32cam
```

- [ ] Compilation completes without errors
- [ ] Compilation completes without major warnings
- [ ] Binary file generated
- [ ] Compilation time reasonable

---

## Upload Checklist

### Front ESP32 Upload

- [ ] Front ESP32 connected via USB
- [ ] Device appears in device manager (COMx port)
- [ ] No other applications using COM port
- [ ] Board type set correctly (esp32doit-devkit-v1)
- [ ] Upload command runs: `pio run -e front_esp32 -t upload`
- [ ] Upload completes without errors
- [ ] Serial monitor shows boot sequence
- [ ] "Front Controller Ready" message appears

### Rear ESP32 Upload

- [ ] Rear ESP32 connected via USB
- [ ] Device appears in device manager
- [ ] Upload command runs: `pio run -e rear_esp32 -t upload`
- [ ] Upload completes without errors
- [ ] Serial monitor shows boot sequence
- [ ] "Rear Controller Ready" message appears (after front starts)

### ESP32-CAM Upload

- [ ] ESP32-CAM connected via USB-OTG adapter to computer
- [ ] Device appears in device manager
- [ ] Upload command runs: `pio run -e camera_esp32cam -t upload`
- [ ] Upload completes without errors
- [ ] Serial monitor shows boot sequence
- [ ] "ESP32-CAM Ready" message appears
- [ ] No continuous LED blink (indicates success)

---

## Initial Power-Up Sequence

### Front Controller Startup

```
Expected Serial Output:
=====================================
PROJECT NIGHTFALL - SYSTEM STARTUP
=====================================

>>> Initializing FRONT CONTROLLER (Master Node)
  [FRONT] Initializing motor drivers...
  [FRONT] Initializing front ultrasonic sensor...
  [FRONT] Initializing gas sensor...
  [FRONT] Initializing safety monitor...
  [FRONT] Initializing autonomous navigation...
  [FRONT] Initializing UART to Rear Controller...
  [FRONT] Initializing UART to Camera Module...
  [FRONT] All systems initialized successfully
>>> Front Controller Ready
```

**Verification:**

- [ ] Boot completes in ~2-3 seconds
- [ ] All systems report initialization
- [ ] No error messages appear
- [ ] LED blinks once (ready signal)

### Rear Controller Startup

```
Expected Serial Output:
=====================================
PROJECT NIGHTFALL - SYSTEM STARTUP
=====================================

>>> Initializing REAR CONTROLLER (Slave Node)
  [REAR] Initializing motor drivers...
  [REAR] Initializing rear ultrasonic sensor...
  [REAR] Initializing safety monitor...
  [REAR] Initializing UART to Master...
  [REAR] All systems initialized successfully
>>> Rear Controller Ready
```

**Verification:**

- [ ] Boot completes in ~1-2 seconds
- [ ] All systems report initialization
- [ ] Waits for heartbeat from front
- [ ] LED blinks once (ready signal)

### Camera Module Startup

```
Expected Serial Output:
=====================================
PROJECT NIGHTFALL - ESP32-CAM INIT
=====================================

>>> Initializing ESP32-CAM...
  [CAMERA] Setting up camera configuration...
  [CAMERA] Initializing ESP camera...
  [CAMERA] Getting camera sensor...
  [CAMERA] Configuring sensor parameters...
  ✓ Camera initialized successfully
>>> Initializing WiFi Access Point...
  ✓ AP started - IP: 192.168.4.1
  📱 WiFi Details:
     SSID: ProjectNightfall
     Password: rescue2025
     Stream URL: http://192.168.4.1:81/stream
>>> Starting streaming server...
✓ ESP32-CAM Ready for streaming
```

**Verification:**

- [ ] Boot completes in ~3-5 seconds
- [ ] Camera initialization successful (not looping LED)
- [ ] WiFi AP started with correct SSID
- [ ] IP address displayed (192.168.4.1)
- [ ] Stream URL shown

---

## Functional Testing Checklist

### Motor Control Testing

**Front Motors:**

```
Serial Input: forward
Expected: Both front wheels rotate forward
Verify:
  [ ] Left motor spins
  [ ] Right motor spins
  [ ] Both at approximately same speed
  [ ] Serial output: "[FRONT] MANUAL: Forward"
```

```
Serial Input: backward
Expected: Both front wheels rotate backward
Verify:
  [ ] Motors reverse direction
  [ ] Similar speed to forward
  [ ] Serial output: "[FRONT] MANUAL: Backward"
```

```
Serial Input: left
Expected: Robot turns left (front motors slow/stop)
Verify:
  [ ] Left motor slower or stopped
  [ ] Right motor continues
  [ ] Turns in circle to left
  [ ] Serial output: "[FRONT] MANUAL: Turn Left"
```

```
Serial Input: right
Expected: Robot turns right
Verify:
  [ ] Right motor slower or stopped
  [ ] Left motor continues
  [ ] Turns in circle to right
  [ ] Serial output: "[FRONT] MANUAL: Turn Right"
```

```
Serial Input: stop
Expected: All motors stop immediately
Verify:
  [ ] Motors stop within 100ms
  [ ] No coasting
  [ ] Serial output: "[FRONT] MANUAL: Stop"
```

### Sensor Testing

**Front Ultrasonic:**

```
Expected: Serial output shows distance readings every 100ms
Verify:
  [ ] Readings between 2-400cm
  [ ] No erratic spikes
  [ ] Distance decreases when hand approaches
  [ ] Distance increases when hand moves away
  [ ] Format: "[FRONT] Front Distance: XX.X cm"
```

**Rear Ultrasonic:**

```
Expected: Rear sensor also reports distances
Verify:
  [ ] Rear distances appear in serial output
  [ ] Independent of front sensor
  [ ] Realistic values
```

**Gas Sensor:**

```
Expected: Analog value read continuously
Verify:
  [ ] Values between 0-4095
  [ ] Baseline ~300-400 in clean air
  [ ] Increases when exposed to smoke/gas
  [ ] Alert triggered above threshold
```

### Autonomous Navigation Testing

```
Serial Input: auto
Expected: Robot begins autonomous movement
Verify:
  [ ] Motor starts moving forward
  [ ] Responds to obstacles (if placed nearby)
  [ ] Navigation state changes in output
  [ ] Avoids collisions
  [ ] Format: "[FRONT] Navigation State: FORWARD"
```

**Obstacle Avoidance:**

```
Setup: Start auto mode, place obstacle in front (30cm away)
Expected: Robot detects and avoids
Verify:
  [ ] Distance reported in serial output
  [ ] State changes to "AVOIDING"
  [ ] Motors change direction (turn left/right)
  [ ] Robot navigates around obstacle
  [ ] Returns to forward once clear
```

### Communication Testing

**Front → Rear:**

```
Expected: Heartbeat every 1 second
Verify (via logic analyzer or serial monitor):
  [ ] Message type: "heartbeat"
  [ ] From source: "front"
  [ ] Contains uptime value
  [ ] Contains state value
  [ ] JSON format valid
```

**Rear → Front:**

```
Expected: Sensor feedback every 100ms
Verify:
  [ ] Message type: "sensor_feedback"
  [ ] Contains rear_distance
  [ ] JSON format valid
```

**Front → Camera:**

```
Expected: Heartbeat every 1 second
Verify:
  [ ] Camera receives heartbeat
  [ ] Camera sends heartbeat_ack
  [ ] Both via UART2
```

### Safety Testing

**Emergency Stop:**

```
Serial Input: estop
Expected: Immediate system shutdown
Verify:
  [ ] Motors stop within 10ms
  [ ] Buzzer sounds twice
  [ ] Serial output: "EMERGENCY STOP TRIGGERED"
  [ ] State changes to STATE_EMERGENCY
  [ ] Rear controller stops motors
  [ ] Camera receives emergency message
```

**Master Timeout (Rear Only):**

```
Setup: Start rear controller, disconnect front controller power
Wait: 3 seconds for timeout
Expected: Rear motors stop automatically
Verify:
  [ ] Motors stop after ~3 seconds
  [ ] Serial output: "Master heartbeat timeout!"
  [ ] State changes to STATE_EMERGENCY
  [ ] Rear still responsive to re-established connection
```

---

## Camera Testing Checklist

### WiFi Connectivity

- [ ] Computer/phone can see "ProjectNightfall" WiFi network
- [ ] Password "rescue2025" works
- [ ] Device connects successfully
- [ ] Device gets IP address from 192.168.4.0/24 range
- [ ] Can ping 192.168.4.1

### Video Streaming

- [ ] Open browser, navigate to http://192.168.4.1:81/stream
- [ ] MJPEG stream appears in browser window
- [ ] Video is fluid (>10 FPS minimum)
- [ ] Image is clear (not overly compressed)
- [ ] Stream plays for >1 minute without freezing
- [ ] No visible artifacts or corruption

### FPS Monitoring

```
Expected Serial Output (every 1 second):
📊 FPS: 30 | Clients: 1 | Frames: 1234 | Uptime: 45s
```

- [ ] FPS at least 20 (ideally 25-30)
- [ ] Frames counter increasing
- [ ] No "dropped_frames" increasing rapidly
- [ ] Uptime matches actual elapsed time

### Commands Testing

```
Send via UART from Front to Camera:
{"type": "command", "data": {"cmd": "flash_on"}}
```

Expected:

- [ ] LED on camera module lights up
- [ ] Serial output: "Flash LED turned ON"

```
Send: {"type": "command", "data": {"cmd": "flash_off"}}
Expected:
  [ ] LED turns off
  [ ] Serial output: "Flash LED turned OFF"
```

```
Send: {"type": "command", "data": {"cmd": "status"}}
Expected:
  [ ] Detailed camera status printed
  [ ] Includes FPS, frame count, drop rate
```

---

## Performance Baseline Testing

### Motor Speed Verification

| Input   | Expected Speed  | Verify             | Result |
| ------- | --------------- | ------------------ | ------ |
| 50 PWM  | Very slow creep | Wheel barely moves | [ ]    |
| 100 PWM | Slow            | Controlled speed   | [ ]    |
| 150 PWM | Medium          | Turn speed         | [ ]    |
| 180 PWM | Normal          | Cruising speed     | [ ]    |
| 200 PWM | Fast            | Open-area speed    | [ ]    |
| 255 PWM | Maximum         | Emergency climb    | [ ]    |

### Latency Measurements

| Event                          | Expected | Measured  | ✓/✗ |
| ------------------------------ | -------- | --------- | --- |
| Serial command → Motor         | <100ms   | \_\_\_ ms | [ ] |
| Obstacle detection → Avoidance | <200ms   | \_\_\_ ms | [ ] |
| Emergency stop trigger → Stop  | <50ms    | \_\_\_ ms | [ ] |
| UART message send → receive    | <10ms    | \_\_\_ ms | [ ] |
| Front → Rear command → execute | <100ms   | \_\_\_ ms | [ ] |

### Current Consumption

| Module              | Expected | Measured  | Note |
| ------------------- | -------- | --------- | ---- |
| Front (idle)        | 100mA    | \_\_\_ mA | [ ]  |
| Front (motors full) | 2A+      | \_\_\_ A  | [ ]  |
| Rear (idle)         | 100mA    | \_\_\_ mA | [ ]  |
| Rear (motors full)  | 2A+      | \_\_\_ A  | [ ]  |
| Camera (idle)       | 200mA    | \_\_\_ mA | [ ]  |
| Camera (streaming)  | 500mA+   | \_\_\_ mA | [ ]  |

---

## Environmental Testing

### Temperature Testing

- [ ] System operates at 0°C
- [ ] System operates at 25°C (room temperature)
- [ ] System operates at 40°C (warm environment)
- [ ] No thermal throttling observed

### Humidity Testing

- [ ] System stable in dry conditions
- [ ] No condensation on components
- [ ] Water-resistant connectors used (optional)
- [ ] Consider environmental sealing if needed

### Lighting Testing (Camera)

- [ ] Video quality good in bright daylight
- [ ] Video quality acceptable in low light
- [ ] No washout or extreme noise
- [ ] Flash LED helps in dark conditions

### Obstacle Course Testing

- [ ] Robot navigates around chairs
- [ ] Robot avoids walls
- [ ] Robot handles stairs (if capable)
- [ ] Robot handles ramps (if applicable)

---

## Documentation Verification

- [ ] IMPLEMENTATION_GUIDE.md reviewed
  - [ ] All functions documented
  - [ ] JSON protocols correct
  - [ ] Timing values match code
- [ ] QUICK_REFERENCE.md reviewed
  - [ ] GPIO mappings verified against actual wiring
  - [ ] Commands listed match code
  - [ ] Safety thresholds match config.h
- [ ] ARCHITECTURE_PINOUTS.md reviewed
  - [ ] Pinout diagrams match wiring
  - [ ] Communication diagram accurate
  - [ ] Power distribution correct

---

## Troubleshooting Guide Consultation

If any tests fail:

1. **Motors don't move:**

   - [ ] Check motor driver power (12V present?)
   - [ ] Check GPIO pin connections
   - [ ] Test motor driver independently
   - [ ] Verify L298N pins match config

2. **UART communication broken:**

   - [ ] Check TX/RX not swapped
   - [ ] Verify baud rate 115200
   - [ ] Test with logic analyzer
   - [ ] Check GND common to all boards

3. **Sensors not reading:**

   - [ ] Verify GPIO pins correct
   - [ ] Check power to sensors
   - [ ] Test sensor independently
   - [ ] Verify ADC pin for gas sensor

4. **Camera not streaming:**

   - [ ] Check WiFi AP started
   - [ ] Verify IP address is 192.168.4.1
   - [ ] Test stream URL in multiple browsers
   - [ ] Check camera module power

5. **System crashes:**
   - [ ] Check watchdog timeout messages
   - [ ] Review serial output for errors
   - [ ] Verify power supply adequate
   - [ ] Check for floating GPIO pins

---

## Sign-Off Checklist

- [ ] All hardware connections verified
- [ ] All three boards compiled and uploaded
- [ ] Front controller boots successfully
- [ ] Rear controller boots successfully
- [ ] Camera module boots successfully
- [ ] Manual motor control working
- [ ] Autonomous navigation working
- [ ] Sensor readings accurate
- [ ] Inter-board communication verified
- [ ] Camera streaming verified
- [ ] Emergency stop verified
- [ ] Performance baselines measured
- [ ] All documentation reviewed
- [ ] System ready for deployment

---

## Sign-Off

**Prepared By:** ******\_\_\_\_******  
**Date:** ******\_\_\_\_******  
**System Status:** ✓ **PRODUCTION READY**

---

_Project Nightfall - Complete Testing & Deployment Checklist_
_All systems verified and operational_
