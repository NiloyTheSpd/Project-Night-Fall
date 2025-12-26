# Project Nightfall - Flashing & Debugging Guide

## Quick Reference

### Device Roles & Flashing

| Board         | Firmware     | Role           | Port              | Flash Cmd                           |
| ------------- | ------------ | -------------- | ----------------- | ----------------------------------- |
| **ESP32 Dev** | back_esp32   | Master Brain   | 8888 (TCP server) | `pio run -e back_esp32 -t upload`   |
| **ESP32 Dev** | front_esp32  | Motor Slave    | - (client)        | `pio run -e front_esp32 -t upload`  |
| **ESP32-CAM** | camera_esp32 | Vision Gateway | 81 (MJPEG/HTTP)   | `pio run -e camera_esp32 -t upload` |

---

## Flashing Instructions

### Prerequisites

```bash
# Ensure PlatformIO is installed
pip install platformio

# Verify USB serial ports available
pio device list
```

### Flash Back ESP32 (Master)

```bash
cd "e:\Project Night Fall"
pio run -e back_esp32 -t upload
# Wait for: "Leaving... Hard resetting via RTS pin..."
```

### Flash Front ESP32 (Motor Slave)

```bash
# Unplug Back ESP32 (to avoid ID conflicts)
pio run -e front_esp32 -t upload
# Wait for successful completion
```

### Flash ESP32-CAM (Vision)

```bash
# Requires specific pins for serial programming
# Connect: GPIO0 → GND during upload
pio run -e camera_esp32 -t upload
# After upload: Release GND, click EN button to restart
```

---

## Startup Sequence

### 1. Power On Back ESP32 (Wait ~2 seconds)

```
[*] Back ESP32 starting...
[*] WiFi AP created: ProjectNightfall
[*] TCP server listening on port 8888
[*] Sensors initialized
[*] Safety monitor active
```

### 2. Power On Front ESP32 (Should connect within 5 seconds)

```
[*] Front ESP32 starting...
[*] Connecting to ProjectNightfall...
[*] Connected! IP: 192.168.4.2
[*] Motors initialized (PWM channels 0-3)
[*] Ready for commands
```

### 3. Power On ESP32-CAM (Should connect within 5 seconds)

```
[*] Camera ESP32 starting...
[*] OV2640 initialized
[*] Connecting to ProjectNightfall...
[*] Connected! IP: 192.168.4.3
[*] MJPEG server running on port 81
[*] Web dashboard at http://192.168.4.3
```

---

## Serial Monitor Output

### Back ESP32 Console

```bash
pio device monitor -p COM3 -b 115200 -e back_esp32
```

**Expected Output:**

```
[Back] Sensor Update: Front=45.2cm, Rear=38.1cm, Gas=250
[Back] Motor Command Received: forward, speed=180
[Back] Front ESP32 Status: Connected
[Back] Telemetry Sent to Clients
```

### Front ESP32 Console

```bash
pio device monitor -p COM4 -b 115200 -e front_esp32
```

**Expected Output:**

```
[Front] Motor Bank 1: SET FORWARD (180 PWM)
[Front] Motor Bank 2: SET FORWARD (180 PWM)
[Front] Sent Status ACK to Back
```

### Camera Console

```bash
pio device monitor -p COM5 -b 115200 -e camera_esp32
```

**Expected Output:**

```
[Camera] OV2640 ready, resolution: 1024x768
[Camera] MJPEG streaming active (8 FPS)
[Camera] Telemetry: Front=45.2cm, Rear=38.1cm, Autonomous=Off
```

---

## Network Communication

### WiFi Network Status

```
SSID: ProjectNightfall
Gateway: 192.168.4.1
Back ESP32:  192.168.4.1:8888
Front ESP32: 192.168.4.2
ESP32-CAM:   192.168.4.3:81
```

### Test Connectivity

```bash
# From any device on network
ping 192.168.4.1    # Back ESP32
ping 192.168.4.2    # Front ESP32
ping 192.168.4.3    # ESP32-CAM

# Test motor commands (from any device)
curl "http://192.168.4.3/cmd?action=forward"
curl "http://192.168.4.3/cmd?action=stop"
curl "http://192.168.4.3/cmd?action=auto_on"
```

### View Camera Stream

```
Browser: http://192.168.4.3
- Live MJPEG stream
- Sensor readings (distance, gas)
- Motor control buttons
- Autonomous mode toggle
```

---

## JSON Message Format Reference

### Motor Command (Front ← Back)

```json
{
  "type": "motor_cmd",
  "action": "forward",
  "speed": 180
}
```

**Actions:** forward, backward, left, right, stop, emergency_stop, climb

### Telemetry (Back → Clients)

```json
{
  "type": "telemetry",
  "front_dist": 45.2,
  "rear_dist": 38.1,
  "gas_level": 250,
  "autonomous": false,
  "temperature": 42.5
}
```

### Hazard Alert (Back → Clients)

```json
{
  "type": "hazard_alert",
  "hazard_type": "obstacle",
  "location": "front",
  "severity": "high"
}
```

### UI Command (Camera → Back)

```json
{
  "type": "ui_cmd",
  "action": "auto_on"
}
```

---

## Debugging Tips

### Motor Not Responding

1. **Check Front ESP32 Connection:**

   ```bash
   # Back ESP32 console should show: [WiFiServer] Client connected
   # If not, verify WiFi credentials in config.h
   ```

2. **Verify Motor Pins:**

   ```cpp
   // Edit include/pins.h and verify MOTOR_FRONT_* definitions
   // Test with manual commands via camera dashboard
   ```

3. **Check L298N Driver:**
   - Verify power supply (5V+ for motors)
   - Test ENA/ENB PWM pins with oscilloscope (should vary 0-5V)
   - Ensure IN1/IN2/IN3/IN4 direction pins switch properly

### Sensors Not Reading

1. **Ultrasonic Sensor (HC-SR04):**

   ```bash
   # Back ESP32 console should show periodic: "Front=XXcm, Rear=YYcm"
   # Check pins: TRIG_FRONT (GPIO22), ECHO_FRONT (GPIO23)
   # Verify 5V power supply and GND connection
   ```

2. **Gas Sensor (MQ2):**
   ```bash
   # Should show gas_level value 0-4095
   # Requires warm-up time (first 60 seconds)
   # Check ADC pin: GAS_SENSOR_PIN (GPIO32)
   ```

### WiFi Connection Issues

1. **Front/Camera won't connect:**

   - Verify SSID: "ProjectNightfall"
   - Verify password: "rescue2025"
   - Check Back ESP32 is powered and AP is running
   - Re-flash Front ESP32 if connection drops

2. **Signal strength low:**
   - Move devices closer (WiFi range ~50m indoors)
   - Check for interference on 2.4GHz band
   - Reduce WiFi TX power in platformio.ini if needed

### Emergency Stop Not Working

1. **Verify E-STOP button:**

   - Check wiring to GPIO (TBD - add pin definition)
   - Verify back_esp32 firmware has ESTOP handler

2. **Verify Buzzer:**
   - Should beep when E-STOP activated
   - Check buzzer pin: BUZZER_PIN (GPIO33)

---

## Performance Monitoring

### CPU Load

```bash
# Back ESP32 should maintain <80% load
# Monitor with: pio device monitor
# Look for: [Loop Time] XX ms (should be ~50ms)
```

### Memory Usage

```
back_esp32:   13.5% RAM (44KB used)
front_esp32:  13.8% RAM (45KB used)
camera_esp32: 15.5% RAM (51KB used)
```

### Network Latency

```bash
# Motor commands should respond in <100ms
# Telemetry updates every 500ms
# Camera stream ~8 FPS (125ms per frame)
```

---

## Common Error Messages

| Error                                               | Cause                  | Solution                              |
| --------------------------------------------------- | ---------------------- | ------------------------------------- |
| `E (100) esp_task_wdt: Task watchdog got triggered` | Loop too long          | Reduce polling intervals in main      |
| `[WiFiClient] Connection refused`                   | Back ESP32 not running | Power cycle Back ESP32                |
| `Camera init failed`                                | OV2640 not detected    | Check camera ribbon cable             |
| `No route to host`                                  | WiFi network down      | Check Back ESP32 is on and AP running |
| `CORRUPT HEAP`                                      | Memory overflow        | Check array bounds in motor control   |

---

## Advanced Troubleshooting

### Enable Verbose Logging

```cpp
// In include/config.h
#define DEBUG 1  // Enable all debug prints
```

### Performance Profiling

```bash
# Compile with profiling enabled
pio run -e back_esp32 -v
# Check build log for timing information
```

### UART Monitor (All Devices Simultaneously)

```bash
# Window 1: Back ESP32
pio device monitor -p COM3 -b 115200 -f time

# Window 2: Front ESP32
pio device monitor -p COM4 -b 115200 -f time

# Window 3: Camera
pio device monitor -p COM5 -b 115200 -f time
```

---

## Production Checklist

- [ ] All three firmware targets compiled successfully
- [ ] Flash binaries transferred to all boards
- [ ] Back ESP32 WiFi AP broadcasts "ProjectNightfall"
- [ ] Front ESP32 connects to Back AP within 5 seconds
- [ ] ESP32-CAM connects to Back AP within 5 seconds
- [ ] Motor Bank 1 (rear) responds to forward/backward commands
- [ ] Motor Bank 2 (front) responds to left/right commands
- [ ] Ultrasonic sensors report distance values (15-100cm)
- [ ] Gas sensor warms up and reports readings
- [ ] Camera stream visible at http://192.168.4.3
- [ ] Emergency stop halts all motors instantly
- [ ] Telemetry updates every 500ms
- [ ] No watchdog timeouts on any board

---

## Support Resources

- **PlatformIO Docs:** https://docs.platformio.org/
- **ESP32 Arduino:** https://github.com/espressif/arduino-esp32
- **ArduinoJson:** https://arduinojson.org/
- **AsyncTCP:** https://github.com/me-no-dev/AsyncTCP
- **ESPAsyncWebServer:** https://github.com/me-no-dev/ESPAsyncWebServer

---

**Last Updated:** 2024-01-XX  
**Firmware Version:** 1.0 (Refactored WiFi Architecture)
