# Project Nightfall Wiring Guide (Updated)

This guide reflects the current, actual pin connections used by the firmware across all boards:

- Back ESP32 (Master/AP, sensors, rear motors)
- Front ESP32 (Motor slave)
- ESP32-CAM (Telemetry client)

It includes boot-strap pin warnings, voltage level requirements, and clear tables for wiring. Follow this exactly for first-time hardware wiring and initial motor validation.

---

## Power & Ground

- Battery → L298N motor supply (e.g., 12V)
- Buck converter 12V→5V (≥5A) powers logic: Back ESP32, Front ESP32, ESP32-CAM, HC-SR04 sensors, MQ-2.
- Common ground bus is mandatory:
  - Battery GND → Buck GND → All ESP32 GND → All sensor GND → All L298N GND.

---

## Boot-Strap Pin Warnings (ESP32)

Certain GPIOs define boot mode at reset. Avoid hard-wiring these pins so they are pulled strongly at boot:

- GPIO0: Boot mode (hold LOW to upload). Keep floating otherwise.
- GPIO2: Strap (must be high at boot). Avoid external pulls; ensure connected devices are high-impedance at reset.
- GPIO12: Strap (flash voltage). Do not bias at boot. We moved status LED off this pin.
- GPIO15: Strap (boot selection). Do not bias at boot. We moved the front ultrasonic trigger off this pin.

Safe reassignments implemented in firmware:

- Ultrasonic front trigger moved to GPIO14.
- ESP32-CAM external status LED moved to GPIO33.
- Rear-right encoder moved to GPIO34/35 (input-only, safe).

---

## Back ESP32 (Master/AP, sensors, rear motors)

Role: `BACK_CONTROLLER`

Power: VIN 5V, GND common.

### Rear Motors (L298N)

- Left motor (driver side A):
  - ENA: GPIO13
  - IN1: GPIO23
  - IN2: GPIO22
- Right motor (driver side B):
  - ENB: GPIO25
  - IN3: GPIO26
  - IN4: GPIO27

### Ultrasonic Sensors (HC-SR04)

- Front:
  - TRIG: GPIO14
  - ECHO: GPIO18 (requires 5V→3.3V divider)
- Rear:
  - TRIG: GPIO19
  - ECHO: GPIO21 (requires 5V→3.3V divider)

Voltage divider example (Echo): 1kΩ (top) to Echo, 2kΩ (bottom) to GND → ~3.33V.

### MQ-2 Gas Sensor

- VCC: 5V, GND common
- Analog out: GPIO32 (ADC1)
- Digital out: GPIO33

Note: Many MQ-2 modules output up to 5V on analog; ensure analog stays ≤3.3V. If your module’s analog can reach 5V, use a resistor divider to ~3.3V.

### Buzzer

- GPIO4

### Rear Wheel Encoders (PCNT)

- Rear Left:
  - A: GPIO16
  - B: GPIO17
- Rear Right (moved off strap pins):
  - A: GPIO34
  - B: GPIO35

---

## Front ESP32 (Motor Slave)

Role: `FRONT_CONTROLLER`

Power: VIN 5V, GND common.

### Motor Drivers (Two L298N boards)

- Driver #1 (Motors 1 & 2):
  - Motor 1: ENA GPIO13, IN1 GPIO23, IN2 GPIO22
  - Motor 2: ENB GPIO25, IN3 GPIO26, IN4 GPIO27
- Driver #2 (Motors 3 & 4):
  - Motor 3: ENA GPIO14, IN1 GPIO32, IN2 GPIO33
  - Motor 4: ENB GPIO15, IN3 GPIO19, IN4 GPIO21

Ground L298N GND to ESP32 GND.

---

## ESP32-CAM (Telemetry Client)

Role: `CAMERA_CONTROLLER`

Power: 5V, GND common.

### Status & Flash LEDs

- External status LED: GPIO33 (via 220Ω resistor to LED+ → LED− to GND)
- Built-in flash LED: GPIO4

### Programming (FTDI)

- FTDI 5V → ESP32-CAM 5V
- FTDI GND → ESP32-CAM GND
- FTDI TX → ESP32-CAM RX0 (GPIO3)
- FTDI RX → ESP32-CAM TX0 (GPIO1)
- To upload: connect GPIO0 → GND, press reset, upload, then disconnect GPIO0 from GND and reset to boot.

### Camera Pins (Reserved)

The camera uses various GPIOs internally; do not repurpose camera-reserved pins.

---

## Voltage Level Requirements

- ESP32 GPIO logic is 3.3V max.
- HC-SR04 Echo outputs 5V → must be level-shifted or divided to ~3.3V on GPIO18 and GPIO21.
- MQ-2 Analog: confirm module analog range; if up to 5V, divide to ≤3.3V for GPIO32.
- Digital inputs (e.g., MQ-2 digital): ensure 3.3V logic compatibility.

---

## Grounding & Noise Tips

- Single common ground bus reduces measurement noise.
- Keep sensor grounds close to ESP32 ground reference.
- Route motor supply separately from logic 5V; avoid sharing thin traces.

---

## Quick Wiring Tables

### Back ESP32 Summary

| Function       | GPIO |
| -------------- | ---- |
| Rear Left ENA  | 13   |
| Rear Left IN1  | 23   |
| Rear Left IN2  | 22   |
| Rear Right ENB | 25   |
| Rear Right IN3 | 26   |
| Rear Right IN4 | 27   |
| US Front TRIG  | 14   |
| US Front ECHO  | 18   |
| US Rear TRIG   | 19   |
| US Rear ECHO   | 21   |
| MQ-2 Analog    | 32   |
| MQ-2 Digital   | 33   |
| Buzzer         | 4    |
| Encoder RL A   | 16   |
| Encoder RL B   | 17   |
| Encoder RR A   | 34   |
| Encoder RR B   | 35   |

### Front ESP32 Summary

| Function    | GPIO |
| ----------- | ---- |
| Motor 1 ENA | 13   |
| Motor 1 IN1 | 23   |
| Motor 1 IN2 | 22   |
| Motor 2 ENB | 25   |
| Motor 2 IN3 | 26   |
| Motor 2 IN4 | 27   |
| Motor 3 ENA | 14   |
| Motor 3 IN1 | 32   |
| Motor 3 IN2 | 33   |
| Motor 4 ENB | 15   |
| Motor 4 IN3 | 19   |
| Motor 4 IN4 | 21   |

### ESP32-CAM Summary

| Function           | GPIO |
| ------------------ | ---- |
| Status LED         | 33   |
| Flash LED          | 4    |
| RX0                | 3    |
| TX0                | 1    |
| Boot (upload only) | 0    |

---

## Safety Checklist Before Power-Up

- Wheels off ground OR disconnect motor 12V.
- All grounds connected to common bus.
- Ultrasonic Echo dividers installed (GPIO18, GPIO21).
- ESP32-CAM GPIO0 disconnected from GND (after programming).
- No shorts between VCC and GND (check with multimeter).
- Battery voltage ≥ 11.5V; buck converter 5V stable.

---

## Notes

- If you must use strap pins, ensure connected devices are high-impedance at reset or add series resistors (1–4.7 kΩ).
- IMU: Not wired in current firmware; if planned, choose non-strap pins and 3.3V logic.

This guide matches the current codebase (pins in `include/pins.h` and encoders in `lib/Encoders/EncoderManager.cpp`).# Project Nightfall - Corrected Wiring Guide

## Pure WiFi Architecture (3 ESP32s)

**Version:** 4.0 - WiFi Architecture with Safe Pin Assignments  
**Last Updated:** December 2025

---

## 🏗️ System Architecture

```
┌─────────────────────────────────────────────────────────┐
│                   Back ESP32 (Master)                   │
│  • WiFi AP: "ProjectNightfall"                          │
│  • WebSocket Server: ws://192.168.4.1:8888              │
│  • Controls: Rear Motors (2), All Sensors, FSM Logic   │
└─────────────────┬───────────────────────────────────────┘
                  │ WiFi
        ┌─────────┴─────────┐
        │                   │
┌───────▼──────┐    ┌───────▼──────┐
│ Front ESP32  │    │ Camera ESP32 │
│ (WiFi Client)│    │ (WiFi Client)│
│ 4 Motors     │    │ Telemetry    │
│ (2x L298N)   │    │ + Camera     │
└──────────────┘    └──────────────┘
```

**Key Advantages:**

- ✅ No physical UART wiring needed
- ✅ All boards on same WiFi network
- ✅ Dashboard connects to same AP
- ✅ Easy to add more ESP32s
- ✅ WebSocket for real-time data

---

## 🔌 Power Supply Architecture

### Power Distribution

```
Battery (12V 10Ah LiPo or Lead-Acid)
    │
    ├─[20A Fuse]─┬─→ L298N #1 (Rear) VCC (12V)
    │            ├─→ L298N #2 (Front) VCC (12V)
    │            └─→ L298N #3 (Front) VCC (12V)
    │
    └─[LM2596 Buck 12V→5V @ 5A]──┬─→ Back ESP32 VIN (5V)
                                  ├─→ Front ESP32 VIN (5V)
                                  ├─→ Camera ESP32 5V pin (5V)
                                  ├─→ HC-SR04 × 2 VCC (5V)
                                  └─→ MQ-2 VCC (5V)

Common Ground Bus (CRITICAL):
    Battery GND ─┬─→ All L298N GND
                 ├─→ Buck Converter GND
                 ├─→ All ESP32 GND
                 ├─→ All Sensor GND
                 └─→ All Motor GND
```

**Power Requirements:**

- **ESP32 × 3:** 1.5A total (500mA each)
- **Motors × 6:** 12-18A peak (2-3A each under load)
- **Sensors:** 0.2A total
- **L298N losses:** 2-3A (heat dissipation)
- **TOTAL PEAK:** 18-22A

**Recommended Components:**

- Battery: 12V 10Ah (not 5Ah!)
- Fuse: 20A fast-blow (not 10A!)
- Buck Converter: 5V 5A rated (not 3A!)
- Decoupling: 100µF capacitor across battery terminals

---

## 📟 Back ESP32 (Master/Brain) - CORRECTED PINS

### Role

- **WiFi:** Access Point "ProjectNightfall"
- **WebSocket:** Server on port 8888
- **Controls:** Rear motors (2), All sensors, FSM logic
- **Actuators:** Buzzer for alerts

### Safe Pin Assignments ✅

| Function              | Pin  | GPIO | Wire To                       | Notes                |
| --------------------- | ---- | ---- | ----------------------------- | -------------------- |
| **L298N Rear Driver** |      |      |                               |                      |
| Left Motor Speed      | ENA  | 13   | L298N ENA                     | PWM 0-255 ✅         |
| Left Motor Dir A      | IN1  | 23   | L298N IN1                     | ✅ Changed from 12   |
| Left Motor Dir B      | IN2  | 22   | L298N IN2                     | ✅ Changed from 14   |
| Right Motor Speed     | ENB  | 25   | L298N ENB                     | PWM 0-255 ✅         |
| Right Motor Dir A     | IN3  | 26   | L298N IN3                     | ✅                   |
| Right Motor Dir B     | IN4  | 27   | L298N IN4                     | ✅                   |
| **Front Ultrasonic**  |      |      |                               |                      |
| Trigger               | TRIG | 15   | HC-SR04 Trig                  | ✅ Changed from 5    |
| Echo                  | ECHO | 18   | HC-SR04 Echo + Divider        | ✅ See circuit below |
| **Rear Ultrasonic**   |      |      |                               |                      |
| Trigger               | TRIG | 19   | HC-SR04 Trig                  | ✅                   |
| Echo                  | ECHO | 21   | HC-SR04 Echo + Divider        | ✅ See circuit below |
| **MQ-2 Gas Sensor**   |      |      |                               |                      |
| Analog                | A0   | 32   | MQ-2 A0                       | ADC1 (0-4095) ✅     |
| Digital               | D0   | 33   | MQ-2 D0                       | Optional ✅          |
| **Buzzer**            |      |      |                               |                      |
| Output                | PWM  | 4    | Buzzer+ via 220Ω              | ✅ Changed from 2    |
| **WiFi**              |      |      |                               |                      |
| Built-in              | -    | -    | Creates AP "ProjectNightfall" | -                    |
| **Power**             |      |      |                               |                      |
| VIN                   | 5V   | VIN  | Buck converter +5V            | ✅                   |
| GND                   | GND  | GND  | Common ground                 | **CRITICAL**         |

### Echo Pin Protection Circuit (REQUIRED)

HC-SR04 outputs 5V but ESP32 GPIO is 3.3V max!

```
HC-SR04 Echo Pin (5V)
    │
    ├─── 1kΩ resistor ─── ESP32 GPIO (18 or 21)
    │
    └─── 2kΩ resistor ─── GND

Result: 5V × (2kΩ / 3kΩ) = 3.33V ✅ Safe!
```

---

## 🎮 Front ESP32 (Motor Slave) - CORRECTED PINS

### Role

- **WiFi:** Client → Connects to "ProjectNightfall"
- **WebSocket:** Client → ws://192.168.4.1:8888
- **Controls:** 4 front motors via 2× L298N drivers

### Safe Pin Assignments ✅

| Function            | Pin | GPIO | Wire To                        | Notes                |
| ------------------- | --- | ---- | ------------------------------ | -------------------- |
| **L298N Driver #1** |     |      |                                |                      |
| Motor 1 Speed       | ENA | 13   | L298N #1 ENA                   | PWM 0-255 ✅         |
| Motor 1 Dir A       | IN1 | 23   | L298N #1 IN1                   | ✅ Changed           |
| Motor 1 Dir B       | IN2 | 22   | L298N #1 IN2                   | ✅ Changed           |
| Motor 2 Speed       | ENB | 25   | L298N #1 ENB                   | PWM 0-255 ✅         |
| Motor 2 Dir A       | IN3 | 26   | L298N #1 IN3                   | ✅                   |
| Motor 2 Dir B       | IN4 | 27   | L298N #1 IN4                   | ✅                   |
| **L298N Driver #2** |     |      |                                |                      |
| Motor 3 Speed       | ENA | 14   | L298N #2 ENA                   | PWM 0-255 ✅ Changed |
| Motor 3 Dir A       | IN1 | 32   | L298N #2 IN1                   | ✅ Changed           |
| Motor 3 Dir B       | IN2 | 33   | L298N #2 IN2                   | ✅ Changed           |
| Motor 4 Speed       | ENB | 15   | L298N #2 ENB                   | PWM 0-255 ✅ Changed |
| Motor 4 Dir A       | IN3 | 19   | L298N #2 IN3                   | ✅                   |
| Motor 4 Dir B       | IN4 | 21   | L298N #2 IN4                   | ✅                   |
| **WiFi**            |     |      |                                |                      |
| Built-in            | -   | -    | Connects to "ProjectNightfall" | -                    |
| **Power**           |     |      |                                |                      |
| VIN                 | 5V  | VIN  | Buck converter +5V             | ✅                   |
| GND                 | GND | GND  | Common ground                  | **CRITICAL**         |

---

## 📷 ESP32-CAM (Telemetry Bridge) - CORRECTED

### Role

- **WiFi:** Client → Connects to "ProjectNightfall"
- **WebSocket:** Client → ws://192.168.4.1:8888
- **Reports:** System health, status
- **Future:** Camera streaming, ML detection

### Pin Assignments (AI-Thinker ESP32-CAM)

| Function                     | Pin       | GPIO                     | Notes                          |
| ---------------------------- | --------- | ------------------------ | ------------------------------ |
| **Built-in Camera (OV2640)** |           |                          |                                |
| Camera Pins                  | Various   | 0,5,18,19,21-27,34-36,39 | **DO NOT USE**                 |
| **Flash LED**                |           |                          |                                |
| White LED                    | Built-in  | 4                        | PWM for brightness             |
| **Status LED (Optional)**    |           |                          |                                |
| External LED                 | Output    | 12                       | LED+ via 220Ω → GND ✅         |
| **Programming (FTDI)**       |           |                          |                                |
| TX0                          | Serial    | 1                        | → FTDI RX                      |
| RX0                          | Serial    | 3                        | → FTDI TX                      |
| GPIO 0                       | Boot Mode | 0                        | Hold LOW to upload             |
| **WiFi**                     |           |                          |                                |
| Built-in                     | -         | -                        | Connects to "ProjectNightfall" |
| **Power**                    |           |                          |                                |
| 5V                           | Power     | 5V                       | Buck converter +5V ✅          |
| GND                          | Ground    | GND                      | Common ground                  |

### ESP32-CAM Programming Procedure

```
Step 1: Connect FTDI Adapter
  FTDI 5V  → ESP32-CAM 5V
  FTDI GND → ESP32-CAM GND
  FTDI TX  → ESP32-CAM RX0 (GPIO 3)
  FTDI RX  → ESP32-CAM TX0 (GPIO 1)

Step 2: Enter Programming Mode
  - Connect GPIO 0 to GND (use jumper wire)
  - Power on OR press RESET button
  - Upload code via PlatformIO

Step 3: Normal Operation
  - Disconnect GPIO 0 from GND
  - Press RESET button
  - ESP32-CAM boots and connects to WiFi
```

---

## 🔧 L298N Motor Driver Configuration

### Each L298N Module Setup

```
1. REMOVE Jumpers:
   - Remove ENA jumper (if present)
   - Remove ENB jumper (if present)
   This enables PWM speed control from ESP32

2. Power Connections:
   - VCC → 12V from battery (motor power)
   - GND → Common ground
   - 5V pin → Leave disconnected OR use as 5V output

3. Logic Connections (from ESP32):
   - ENA → PWM pin (speed control)
   - IN1 → GPIO pin (direction bit 1)
   - IN2 → GPIO pin (direction bit 2)
   - ENB → PWM pin (speed control)
   - IN3 → GPIO pin (direction bit 1)
   - IN4 → GPIO pin (direction bit 2)

4. Motor Connections:
   - OUT1 → Motor A positive (+)
   - OUT2 → Motor A negative (−)
   - OUT3 → Motor B positive (+)
   - OUT4 → Motor B negative (−)

Motor Direction Control:
   IN1=HIGH, IN2=LOW  → Motor A Forward
   IN1=LOW,  IN2=HIGH → Motor A Reverse
   IN1=LOW,  IN2=LOW  → Motor A Stop/Brake
   IN1=HIGH, IN2=HIGH → Motor A Stop/Brake
```

---

## 🌐 WiFi Network Configuration

### Back ESP32 (Master) - Access Point

```cpp
// config.h
#define WIFI_SSID "ProjectNightfall"
#define WIFI_PASSWORD "rescue2025"  // Change this!
#define WIFI_CHANNEL 6
#define WEBSOCKET_PORT 8888

// Back ESP32 IP: 192.168.4.1 (default AP IP)
```

### Front & Camera ESP32 (Clients)

```cpp
// config.h
#define WIFI_SSID "ProjectNightfall"
#define WIFI_PASSWORD "rescue2025"  // Must match master!

// Auto-assigned IPs:
// Front ESP32:  192.168.4.2
// Camera ESP32: 192.168.4.3
// Dashboard:    192.168.4.4+ (any device)
```

---

## 📊 Complete Pin Reference Tables

### Back ESP32 Pin Summary

```
GPIO  | Function            | Direction | Notes
------|---------------------|-----------|------------------
13    | L298N ENA (PWM)     | Output    | Rear Left Speed
23    | L298N IN1           | Output    | Rear Left Dir A
22    | L298N IN2           | Output    | Rear Left Dir B
25    | L298N ENB (PWM)     | Output    | Rear Right Speed
26    | L298N IN3           | Output    | Rear Right Dir A
27    | L298N IN4           | Output    | Rear Right Dir B
15    | US Front Trig       | Output    | 10µs pulse
18    | US Front Echo       | Input     | With divider!
19    | US Rear Trig        | Output    | 10µs pulse
21    | US Rear Echo        | Input     | With divider!
32    | MQ-2 Analog         | Input     | ADC1 (0-4095)
33    | MQ-2 Digital        | Input     | Optional
4     | Buzzer              | Output    | PWM or digital
```

### Front ESP32 Pin Summary

```
GPIO  | Function            | Direction | Notes
------|---------------------|-----------|------------------
13    | L298N #1 ENA        | Output    | Motor 1 Speed
23    | L298N #1 IN1        | Output    | Motor 1 Dir A
22    | L298N #1 IN2        | Output    | Motor 1 Dir B
25    | L298N #1 ENB        | Output    | Motor 2 Speed
26    | L298N #1 IN3        | Output    | Motor 2 Dir A
27    | L298N #1 IN4        | Output    | Motor 2 Dir B
14    | L298N #2 ENA        | Output    | Motor 3 Speed
32    | L298N #2 IN1        | Output    | Motor 3 Dir A
33    | L298N #2 IN2        | Output    | Motor 3 Dir B
15    | L298N #2 ENB        | Output    | Motor 4 Speed
19    | L298N #2 IN3        | Output    | Motor 4 Dir A
21    | L298N #2 IN4        | Output    | Motor 4 Dir B
```

---

## 🧪 Testing Procedure

### Step 1: Power Supply Verification (Motors Disconnected)

```
1. Set multimeter to DC voltage
2. Measure battery: Should be 11.1V - 12.6V
3. Measure L298N VCC pins: Should be 11.1V - 12.6V
4. Measure buck converter output: Should be 4.9V - 5.1V
5. Measure ESP32 VIN pins: Should be 4.9V - 5.1V

✅ All voltages correct? Proceed to Step 2
❌ Any voltage wrong? Fix power before continuing!
```

### Step 2: ESP32 Individual Tests

**Test Back ESP32:**

```
1. Connect ONLY Back ESP32 to power
2. Open Serial Monitor (115200 baud)
3. Should see:
   "Back ESP32 initializing..."
   "WiFi AP started: ProjectNightfall"
   "WebSocket server started on port 8888"
4. On PC/Phone, check WiFi networks
5. Should see "ProjectNightfall" network

✅ WiFi AP visible? Proceed
❌ No WiFi AP? Check power, antenna, code
```

**Test Front ESP32:**

```
1. Keep Back ESP32 powered
2. Power on Front ESP32
3. Open Serial Monitor
4. Should see:
   "Front ESP32 initializing..."
   "Connecting to WiFi: ProjectNightfall"
   "WiFi connected! IP: 192.168.4.2"
   "WebSocket connected to ws://192.168.4.1:8888"

✅ Connected? Proceed
❌ Not connected? Check WiFi password, range
```

**Test Camera ESP32:**

```
1. Program ESP32-CAM (GPIO 0 to GND, upload, disconnect)
2. Power on Camera ESP32
3. Should connect to WiFi and WebSocket
4. Check Serial Monitor for confirmation

✅ Connected? Proceed
❌ Issues? Check programming procedure
```

### Step 3: Sensor Tests (Motors Still Disconnected)

**Test Ultrasonic Sensors:**

```
1. Place object 30cm in front of robot
2. Check dashboard or serial monitor
3. Should show: "Front Distance: ~30 cm"
4. Move object, distance should change

✅ Working? Test rear sensor same way
❌ Not working? Check:
   - Voltage divider on Echo pins
   - VCC = 5V, GND connected
   - Wiring to correct GPIOs
```

**Test Gas Sensor:**

```
1. Power on (sensor needs 30 sec warmup)
2. Read baseline value (should be 200-400)
3. Light match near sensor
4. Value should increase significantly

✅ Working? Proceed
❌ Not working? Check:
   - VCC = 5V
   - Analog pin = GPIO 32
   - Sensor not damaged
```

### Step 4: Motor Tests (One at a Time)

**Safety First:**

```
⚠️ WHEELS OFF GROUND OR USE BLOCKS
⚠️ BE READY TO DISCONNECT POWER
```

**Test Each Motor:**

```
1. Connect ONE motor to L298N OUT1/OUT2
2. In dashboard, send FORWARD command
3. Motor should spin
4. Send STOP command
5. Motor should stop
6. Send BACKWARD command
7. Motor should spin opposite direction

Repeat for all 6 motors

✅ All motors working? Full system ready!
❌ Motor not spinning? Check:
   - L298N VCC = 12V
   - Motor connected to correct OUT pins
   - L298N GND connected to ESP32 GND
   - ENA/ENB jumpers removed
```

### Step 5: Full System Integration

```
1. All motors connected
2. All sensors connected
3. Wheels on ground or blocks
4. Power on system
5. Connect to dashboard
6. Test autonomous mode
7. Test manual control
8. Test emergency stop

✅ Everything working? Congratulations! 🎉
```

---

## 🚨 Safety Checklist

**Before First Power-On:**

- [ ] All GNDs connected to common ground bus
- [ ] No shorts between VCC and GND (use multimeter)
- [ ] 20A fuse installed on motor battery positive
- [ ] 100µF capacitor across battery terminals
- [ ] Voltage dividers installed on Echo pins (GPIO 18, 21)
- [ ] L298N ENA/ENB jumpers removed
- [ ] Buck converter output verified at 5.0V ±0.1V
- [ ] ESP32-CAM GPIO 0 disconnected from GND (after programming)
- [ ] All wires secured with zip ties
- [ ] No loose connections
- [ ] Fire extinguisher nearby (LiPo battery safety)

**Before Each Operation:**

- [ ] Battery voltage > 11.5V
- [ ] WiFi AP "ProjectNightfall" active
- [ ] Dashboard shows "Connected"
- [ ] All sensor readings reasonable
- [ ] Emergency stop button accessible
- [ ] Operating area clear of people/obstacles
- [ ] Supervisor present

---

## 🔍 Troubleshooting

### WiFi Issues

**Problem:** Front/Camera ESP32 won't connect to WiFi

**Solutions:**

1. Check WiFi password matches exactly
2. Verify Back ESP32 AP is running (check serial)
3. Move ESP32s closer together (<5m for testing)
4. Change WiFi channel in config.h (try 1, 6, 11)
5. Check antenna connected on all ESP32s

### WebSocket Issues

**Problem:** Connected to WiFi but WebSocket fails

**Solutions:**

1. Verify WebSocket server started (check Back ESP32 serial)
2. Check firewall not blocking port 8888
3. Verify IP address is 192.168.4.1
4. Restart all ESP32s in order: Back → Front → Camera

### Motor Issues

**Problem:** Motors don't spin

**Solutions:**

1. Check L298N VCC = 12V (motor power)
2. Verify ESP32 GND connected to L298N GND
3. Confirm ENA/ENB jumpers removed
4. Test with multimeter: ENA/ENB should show ~2.5V when PWM=127
5. Swap motor wires (IN1 ↔ IN2) if direction wrong

### Sensor Issues

**Problem:** Ultrasonic shows 0 or max value

**Solutions:**

1. Check voltage divider installed correctly
2. Verify VCC = 5V, GND connected
3. Test with different object (not transparent/soft)
4. Reduce distance to <1 meter for testing

**Problem:** Gas sensor always shows high value

**Solutions:**

1. Let sensor warm up 1-2 minutes
2. Check in clean air (outdoor or ventilated area)
3. Calibrate threshold in code
4. Replace sensor if damaged

---

## 📐 Wiring Diagram

### Physical Layout Recommendation

```
        [Front View]

    M1  M2    (Front ESP32)
     │  │         ↓
    ┌────────────────┐
    │  [Front PCB]   │
    │  L298N  L298N  │
    │   #1     #2    │
    └────────┬───────┘
             │
    ┌────────▼───────┐
    │  [Back PCB]    │
    │  ESP32 Master  │
    │  Sensors       │
    │  L298N #1      │
    └────────┬───────┘
             │
    M5  M6   │
     │  │    │
    [Rear Motors]

    [Battery Pack]
    [Buck Converter]

    [ESP32-CAM]
    (Mounted high for camera view)
```

### Cable Color Code (Recommended)

```
Power:
  Red    = +12V (motor power)
  Orange = +5V (logic power)
  Black  = Ground (all GNDs)

Signals:
  Yellow = PWM signals (ENA/ENB)
  Blue   = Direction signals (IN1-4)
  Green  = Sensor signals (Trig/Echo)
  White  = Analog signals (Gas sensor)
  Purple = Buzzer/LED
```

---

## 📚 Additional Resources

### Pin Reference Links

- [ESP32 DevKit Pinout](https://randomnerdtutorials.com/esp32-pinout-reference-gpios/)
- [ESP32-CAM Pinout](https://randomnerdtutorials.com/esp32-cam-ai-thinker-pinout/)
- [L298N Datasheet](https://www.st.com/resource/en/datasheet/l298.pdf)
- [HC-SR04 Datasheet](https://cdn.sparkfun.com/datasheets/Sensors/Proximity/HCSR04.pdf)

### Code Examples

- WebSocket Server: `src/main_rear.cpp`
- WebSocket Client: `src/main_front.cpp`
- Motor Control: `lib/Motors/L298N.cpp`
- Sensor Drivers: `lib/Sensors/`

---

**Document Version:** 4.0 (Pure WiFi Architecture - Corrected Pins)  
**Last Updated:** December 2025  
**Author:** Project Nightfall Team

**🎉 Ready to build! All pin conflicts resolved!**
