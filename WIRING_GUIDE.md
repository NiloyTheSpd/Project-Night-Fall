# Project Nightfall - Complete Wiring Guide

**Comprehensive pin-by-pin wiring instructions for all three ESP32 boards**

---

## 🔌 Power Supply Architecture

### Power Requirements

- **ESP32 Boards (3×):** 5V/500mA each (via USB or VIN pin)
- **L298N Drivers (3×):** 5-35V motor supply, draws from onboard 5V regulator
- **Motors (6×):** 12V @ 2-3A total (under load)
- **Sensors:** 5V/100mA total

### Recommended Power Setup

```
Battery (12V 5Ah LiPo or Lead-Acid)
    ├─→ [Fuse 10A] ─→ L298N Motor Supply (×3 drivers)
    │                   ├─→ L298N #1: VCC → 12V, GND → Battery GND
    │                   ├─→ L298N #2: VCC → 12V, GND → Battery GND
    │                   └─→ L298N #3: VCC → 12V, GND → Battery GND
    │
    └─→ [LM2596 Buck 12V→5V] ─→ 5V Rail (shared)
                                 ├─→ Back ESP32 (VIN + GND)
                                 ├─→ Front ESP32 (VIN + GND)
                                 ├─→ Camera ESP32 (VIN + GND)
                                 ├─→ HC-SR04 sensors (VCC + GND)
                                 └─→ MQ-2 sensor (VCC + GND)
```

**Critical:**

- **Common Ground:** All GNDs must connect (battery, ESP32s, L298N drivers, sensors)
- **Decoupling:** Add 100μF capacitor across motor battery terminals
- **Protection:** Use fast-blow fuse (10A) on motor battery positive

---

## 📟 Back ESP32 (Master/Sensor Node)

### Role

- Navigation brain
- Sensor processing (ultrasonic × 2, gas sensor)
- Rear motor control (2 motors via L298N)
- ESP-NOW telemetry broadcast

### Pin Assignments

| Function                  | Pin  | GPIO | Wire To            | Notes              |
| ------------------------- | ---- | ---- | ------------------ | ------------------ |
| **L298N Rear Driver**     |      |      |                    |
| Left Motor Speed          | ENA  | 13   | L298N ENA (PWM)    | PWM 0-255          |
| Left Motor Dir A          | IN1  | 12   | L298N IN1          | HIGH/LOW           |
| Left Motor Dir B          | IN2  | 14   | L298N IN2          | HIGH/LOW           |
| Right Motor Speed         | ENB  | 25   | L298N ENB (PWM)    | PWM 0-255          |
| Right Motor Dir A         | IN3  | 26   | L298N IN3          | HIGH/LOW           |
| Right Motor Dir B         | IN4  | 27   | L298N IN4          | HIGH/LOW           |
| **Front Ultrasonic**      |      |      |                    |
| Trigger                   | TRIG | 5    | HC-SR04 Trig       | 10μs pulse         |
| Echo                      | ECHO | 18   | HC-SR04 Echo       | 5V → use divider   |
| **Rear Ultrasonic**       |      |      |                    |
| Trigger                   | TRIG | 19   | HC-SR04 Trig       | 10μs pulse         |
| Echo                      | ECHO | 21   | HC-SR04 Echo       | 5V → use divider   |
| **MQ-2 Gas Sensor**       |      |      |                    |
| Analog                    | AO   | 32   | MQ-2 A0            | ADC1 (0-4095)      |
| Digital                   | DO   | 33   | MQ-2 D0            | Optional threshold |
| **Buzzer**                |      |      |                    |
| Output                    | PWM  | 2    | Buzzer+ via 220Ω   | Active/passive     |
| **Status LED (Optional)** |      |      |                    |
| Output                    | PWM  | 23   | LED+ via 220Ω      | Heartbeat          |
| **Power**                 |      |      |                    |
| VIN                       | 5V   | VIN  | Buck converter +5V |                    |
| GND                       | GND  | GND  | Common ground      | **CRITICAL**       |

### Detailed Wiring: L298N Rear Motor Driver

**L298N Terminal Connections:**

```
L298N Rear Driver
├─ VCC (Motor Supply) ────→ Battery +12V
├─ GND (Motor Ground) ────→ Battery GND + ESP32 GND (common)
├─ +5V (Logic Supply) ────→ ESP32 VIN (or separate 5V)
├─ ENA ───────────────────→ ESP32 GPIO 13 (PWM left speed)
├─ IN1 ───────────────────→ ESP32 GPIO 12 (left direction A)
├─ IN2 ───────────────────→ ESP32 GPIO 14 (left direction B)
├─ OUT1/OUT2 ─────────────→ Rear Left Motor terminals
├─ ENB ───────────────────→ ESP32 GPIO 25 (PWM right speed)
├─ IN3 ───────────────────→ ESP32 GPIO 26 (right direction A)
├─ IN4 ───────────────────→ ESP32 GPIO 27 (right direction B)
└─ OUT3/OUT4 ─────────────→ Rear Right Motor terminals
```

**Motor Logic:**

- **Forward:** IN1=HIGH, IN2=LOW (left) | IN3=HIGH, IN4=LOW (right)
- **Reverse:** IN1=LOW, IN2=HIGH (left) | IN3=LOW, IN4=HIGH (right)
- **Brake:** IN1=HIGH, IN2=HIGH or IN1=LOW, IN2=LOW
- **Speed:** ENA/ENB PWM duty cycle (0-255)

**Jumpers on L298N:**

- Remove ENA/ENB jumpers to enable PWM speed control
- Keep +5V enable jumper if using onboard regulator

### Detailed Wiring: HC-SR04 Ultrasonic Sensors

**Per Sensor (×2: Front + Rear):**

```
HC-SR04
├─ VCC ───→ +5V rail
├─ GND ───→ Common ground
├─ TRIG ──→ ESP32 GPIO (5 for front, 19 for rear)
└─ ECHO ──→ [Voltage Divider] ──→ ESP32 GPIO (18 for front, 21 for rear)
```

**ECHO Voltage Divider (5V → 3.3V):**

```
HC-SR04 ECHO (5V)
    ↓
   [1kΩ resistor] ──→ ESP32 GPIO (18 or 21)
    ↓
   [2kΩ resistor]
    ↓
   GND

Output voltage: 5V × (2kΩ / 3kΩ) = 3.33V ✓
```

**Note:** Many ESP32 GPIOs tolerate 5V on input. Divider is optional but recommended for strict 3.3V compliance.

**Timing:**

- Send 10μs HIGH pulse on TRIG
- Measure ECHO HIGH duration: `distance_cm = duration_μs / 58`

### Detailed Wiring: MQ-2 Gas Sensor

```
MQ-2 Module
├─ VCC ──→ +5V rail
├─ GND ──→ Common ground
├─ A0 ───→ ESP32 GPIO 32 (ADC1 channel 4)
└─ D0 ───→ ESP32 GPIO 33 (optional digital threshold)
```

**Calibration:**

1. Power on, wait 30-60 seconds (warm-up heater)
2. Read baseline in clean air: `analogRead(32)` → ~400-600
3. Set threshold in `config.h`: `GAS_THRESHOLD_ALERT = baseline + 200`
4. Test with lighter gas (DO NOT IGNITE): reading should spike >1000

**ADC Reading:**

- Range: 0-4095 (12-bit ADC)
- Higher value = more gas concentration
- Use ADC1 (pins 32-39) to avoid WiFi interference

### Detailed Wiring: Buzzer

**Active Buzzer (simpler):**

```
Buzzer
├─ + ──→ [220Ω resistor] ──→ ESP32 GPIO 2
└─ - ──→ GND

Code: digitalWrite(BUZZER_PIN, HIGH); // On
      digitalWrite(BUZZER_PIN, LOW);  // Off
```

**Passive Buzzer (tone control):**

```
Same wiring, but use PWM:
ledcWriteTone(PWM_CHANNEL_BUZZER, 2000); // 2kHz tone
ledcWrite(PWM_CHANNEL_BUZZER, 128);      // 50% duty
```

---

## 🎮 Front ESP32 (Motor Slave Node)

### Role

- 4-motor traction control
- Receives motor commands via ESP-NOW
- No sensors (dedicated motor driver)

### Pin Assignments

| Function                  | Pin | GPIO | Wire To            | Notes              |
| ------------------------- | --- | ---- | ------------------ | ------------------ |
| **L298N Driver #1**       |     |      |                    |
| Motor 1 Speed             | ENA | 13   | L298N #1 ENA       | PWM 0-255          |
| Motor 1 Dir A             | IN1 | 12   | L298N #1 IN1       | HIGH/LOW           |
| Motor 1 Dir B             | IN2 | 14   | L298N #1 IN2       | HIGH/LOW           |
| Motor 2 Speed             | ENB | 25   | L298N #1 ENB       | PWM 0-255          |
| Motor 2 Dir A             | IN3 | 26   | L298N #1 IN3       | HIGH/LOW           |
| Motor 2 Dir B             | IN4 | 27   | L298N #1 IN4       | HIGH/LOW           |
| **L298N Driver #2**       |     |      |                    |
| Motor 3 Speed             | ENA | 4    | L298N #2 ENA       | PWM 0-255          |
| Motor 3 Dir A             | IN1 | 16   | L298N #2 IN1       | HIGH/LOW           |
| Motor 3 Dir B             | IN2 | 17   | L298N #2 IN2       | HIGH/LOW           |
| Motor 4 Speed             | ENB | 18   | L298N #2 ENB       | PWM 0-255          |
| Motor 4 Dir A             | IN3 | 19   | L298N #2 IN3       | HIGH/LOW           |
| Motor 4 Dir B             | IN4 | 21   | L298N #2 IN4       | HIGH/LOW           |
| **Status LED (Optional)** |     |      |                    |
| Output                    | PWM | 23   | LED+ via 220Ω      | Activity indicator |
| **Power**                 |     |      |                    |
| VIN                       | 5V  | VIN  | Buck converter +5V |                    |
| GND                       | GND | GND  | Common ground      | **CRITICAL**       |

### Detailed Wiring: L298N Driver #1 (Motors 1 & 2)

```
L298N Driver #1
├─ VCC ──────────→ Battery +12V
├─ GND ──────────→ Battery GND + ESP32 GND
├─ +5V ──────────→ ESP32 VIN (or separate 5V)
├─ ENA ──────────→ ESP32 GPIO 13 (Motor 1 speed)
├─ IN1 ──────────→ ESP32 GPIO 12 (Motor 1 dir A)
├─ IN2 ──────────→ ESP32 GPIO 14 (Motor 1 dir B)
├─ OUT1/OUT2 ────→ Front Left Motor 1
├─ ENB ──────────→ ESP32 GPIO 25 (Motor 2 speed)
├─ IN3 ──────────→ ESP32 GPIO 26 (Motor 2 dir A)
├─ IN4 ──────────→ ESP32 GPIO 27 (Motor 2 dir B)
└─ OUT3/OUT4 ────→ Front Right Motor 1
```

### Detailed Wiring: L298N Driver #2 (Motors 3 & 4)

```
L298N Driver #2
├─ VCC ──────────→ Battery +12V
├─ GND ──────────→ Battery GND + ESP32 GND
├─ +5V ──────────→ ESP32 VIN (or separate 5V)
├─ ENA ──────────→ ESP32 GPIO 4 (Motor 3 speed)
├─ IN1 ──────────→ ESP32 GPIO 16 (Motor 3 dir A)
├─ IN2 ──────────→ ESP32 GPIO 17 (Motor 3 dir B)
├─ OUT1/OUT2 ────→ Front Left Motor 2
├─ ENB ──────────→ ESP32 GPIO 18 (Motor 4 speed)
├─ IN3 ──────────→ ESP32 GPIO 19 (Motor 4 dir A)
├─ IN4 ──────────→ ESP32 GPIO 21 (Motor 4 dir B)
└─ OUT3/OUT4 ────→ Front Right Motor 2
```

**Motor Layout (Top View):**

```
    Front of Robot
    ┌─────────────┐
    │  M1     M2  │  ← Driver #1 (GPIO 13,12,14 | 25,26,27)
    │             │
    │  M3     M4  │  ← Driver #2 (GPIO 4,16,17 | 18,19,21)
    └─────────────┘
       Rear
```

**Synchronized Control:**

- All 4 motors forward: Same direction bits on all drivers
- Turn left: Left motors slow/reverse, right motors fast/forward
- Turn right: Right motors slow/reverse, left motors fast/forward

---

## 📷 ESP32-CAM (Telemetry Bridge)

### Role

- ESP-NOW telemetry receiver (from Back ESP32)
- WebSocket server (port 8888/ws)
- WiFi client (connects to "ProjectNightfall" AP)
- Optional: Camera streaming (future)

### Pin Assignments (AI-Thinker ESP32-CAM)

| Function                      | Pin     | GPIO                     | Notes                        |
| ----------------------------- | ------- | ------------------------ | ---------------------------- |
| **Built-in Camera (OV2640)**  |         |                          |
| All camera pins               | Various | 0,5,18,19,21-27,34-36,39 | **DO NOT USE**               |
| **Flash LED**                 |         |                          |                              |
| White LED                     | PWM     | 4                        | Built-in, PWM for brightness |
| **microSD Card (1-bit mode)** |         |                          |                              |
| CMD                           | SD_MMC  | 15                       | Reserved for SD card         |
| CLK                           | SD_MMC  | 14                       | Reserved for SD card         |
| DATA0                         | SD_MMC  | 2                        | Reserved for SD card         |
| **Status LED**                |         |                          |                              |
| External LED                  | PWM     | 33                       | LED+ via 220Ω → GND          |
| **Power**                     |         |                          |                              |
| VIN                           | 5V      | 5V                       | USB or 5V rail               |
| GND                           | GND     | GND                      | Common ground                |
| **Programming (FTDI)**        |         |                          |                              |
| TX0                           | Serial  | 1                        | FTDI RX                      |
| RX0                           | Serial  | 3                        | FTDI TX                      |
| GPIO 0                        | Boot    | 0                        | Hold LOW for upload          |
| Reset                         | RST     | EN                       | Pulse LOW to reset           |

### Detailed Wiring: Programming ESP32-CAM

**FTDI/USB-to-Serial Adapter:**

```
FTDI Adapter    ESP32-CAM
├─ RX ─────────→ GPIO 1 (TX0)
├─ TX ─────────→ GPIO 3 (RX0)
├─ GND ────────→ GND
├─ 5V ─────────→ 5V (VCC)
└─ (not used)

Boot Mode Jumper (for upload only):
├─ GPIO 0 ────→ GND (connect during upload)
└─ Remove after upload, press RESET
```

**Upload Procedure:**

1. Connect GPIO 0 to GND
2. Connect FTDI to ESP32-CAM
3. Press RESET button (or power cycle)
4. Run: `python -m platformio run -e camera_esp32 --target upload`
5. Wait for "Connecting..." then upload starts
6. After success: Disconnect GPIO 0 from GND
7. Press RESET to run firmware

### ESP32-CAM GPIO Restrictions

**Reserved by Camera (DO NOT USE):**

- GPIO 0: XCLK (camera clock)
- GPIO 5, 18, 19, 21: Data lines (Y2, Y3, Y4, Y5)
- GPIO 22, 23, 25, 26, 27: PCLK, HREF, VSYNC, SIOD, SIOC
- GPIO 34, 35, 36, 39: Data lines (Y6, Y7, Y8, Y9) + input only
- GPIO 32: PWDN (camera power down)

**Available GPIOs (if not using SD card):**

- GPIO 1, 3: Serial (TX/RX) - use for debugging
- GPIO 2: Onboard LED / SD_DATA0 (shared)
- GPIO 4: Flash LED (can repurpose if no flash needed)
- GPIO 12, 13, 14, 15: Available if not using SD_MMC
- GPIO 16, 17: UART2 (available)
- GPIO 33: ADC1 input (safe for general use)

### Network Configuration

**WiFi Settings (in firmware):**

```cpp
SSID: "ProjectNightfall"
Password: "rescue2025"
Mode: WIFI_STA (client mode)
IP: Assigned by router (typically 192.168.4.1 if AP)
```

**WebSocket Server:**

- Port: 8888
- Path: `/ws`
- URL: `ws://192.168.4.1:8888/ws`

---

## 🔋 Power Distribution Schematic

```
Battery (12V 5Ah LiPo)
    │
    ├─[10A Fuse]─┬─→ L298N #1 (Back) VCC
    │            ├─→ L298N #2 (Front #1) VCC
    │            └─→ L298N #3 (Front #2) VCC
    │
    └─[LM2596 Buck 12V→5V]───┬─→ Back ESP32 (VIN)
                             ├─→ Front ESP32 (VIN)
                             ├─→ Camera ESP32 (5V)
                             ├─→ HC-SR04 × 2 (VCC)
                             └─→ MQ-2 (VCC)

Common Ground Bus:
    Battery GND ─┬─→ L298N #1 GND
                 ├─→ L298N #2 GND
                 ├─→ L298N #3 GND
                 ├─→ Buck Converter GND
                 ├─→ Back ESP32 GND
                 ├─→ Front ESP32 GND
                 ├─→ Camera ESP32 GND
                 ├─→ All sensor GNDs
                 └─→ All motor GNDs
```

**Critical Safety:**

- ✅ **Common Ground:** All GNDs must connect at one point (star topology)
- ✅ **Fuse:** 10A fast-blow on motor battery positive
- ✅ **Capacitors:** 100μF electrolytic across motor battery (+ to +, - to -)
- ✅ **Diodes:** Optional 1N4007 across each motor (cathode to +, anode to -)
- ✅ **Wire Gauge:** 18AWG for motors, 22-24AWG for logic

---

## 🧰 Bill of Materials (Wiring Components)

| Component             | Quantity | Notes                                |
| --------------------- | -------- | ------------------------------------ |
| ESP32 DevKit V1       | 2        | Back + Front controllers             |
| ESP32-CAM AI-Thinker  | 1        | Telemetry bridge                     |
| L298N Motor Driver    | 3        | 1× Back, 2× Front                    |
| HC-SR04 Ultrasonic    | 2        | Front + Rear sensors                 |
| MQ-2 Gas Sensor       | 1        | Module with A0/D0 outputs            |
| 12V DC Motors         | 6        | 300-500 RPM geared                   |
| Active Buzzer (5V)    | 1        | Alarm output                         |
| LEDs (any color)      | 3        | Status indicators (optional)         |
| 220Ω Resistors        | 4        | LED current limiting + buzzer        |
| 1kΩ Resistors         | 2        | ECHO voltage divider                 |
| 2kΩ Resistors         | 2        | ECHO voltage divider                 |
| 100μF Capacitors      | 3        | Motor power decoupling               |
| 1N4007 Diodes         | 6        | Motor back-EMF protection (optional) |
| LM2596 Buck Converter | 1        | 12V → 5V (3A+ rated)                 |
| 12V Battery           | 1        | 5Ah LiPo or Lead-Acid                |
| 10A Fuse + Holder     | 1        | Motor battery protection             |
| Jumper Wires          | 50+      | 22-24AWG, various colors             |
| Heat Shrink Tubing    | 1m       | Wire insulation                      |
| USB-to-Serial (FTDI)  | 1        | ESP32-CAM programming                |
| Breadboard/Perfboard  | 1        | Prototyping or permanent build       |

---

## 🛡️ Safety Checklist

Before powering on:

- [ ] All GNDs connected to common ground bus
- [ ] No shorts between VCC and GND (use multimeter continuity test)
- [ ] Motor battery fused (10A fast-blow)
- [ ] Decoupling capacitors installed (100μF near motors)
- [ ] ECHO pins use voltage dividers (or confirmed 5V-tolerant)
- [ ] L298N jumpers removed for PWM control (ENA/ENB)
- [ ] ESP32-CAM GPIO 0 disconnected from GND (after upload)
- [ ] All wires secured (no loose connections)
- [ ] Motor polarity marked (+ and - terminals)
- [ ] Battery voltage confirmed (11-13V for 12V nominal)

---

## 🔍 Troubleshooting

### Motors don't spin

- Check L298N VCC has 12V from battery
- Verify ESP32 GND connected to L298N GND
- Confirm PWM signals with LED on ENA/ENB pins
- Test motor directly with 12V (bypass L298N)

### ESP32 resets randomly

- Insufficient power supply (use 2A+ regulator)
- Motor noise on power rail (add capacitors)
- WiFi causing brownouts (upgrade power supply)

### Sensors give erratic readings

- Check 5V rail stability (should not drop below 4.8V)
- MQ-2 needs 30-60s warm-up time
- ECHO voltage too high (add voltage divider)

### ESP32-CAM won't upload

- GPIO 0 must be LOW during boot (connect to GND)
- Verify TX/RX not swapped (ESP32 TX → FTDI RX)
- Try lower baud rate (115200 instead of 460800)

---

## 📸 Visual Wiring Diagrams

_(Diagrams would be inserted here with tools like Fritzing)_

**Recommended Tools:**

- Fritzing (circuit diagrams)
- KiCad (PCB layouts)
- Draw.io (block diagrams)

---

**Document Version:** 2.0  
**Last Updated:** December 27, 2025  
**Author:** Project Nightfall Team
