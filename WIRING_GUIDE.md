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

- **Master Brain**: Creating WiFi AP + WebSocket Server.
- **Navigation**: FSM & Autonomy logic.
- **Sensors**: Ultrasonic × 2, Gas Sensor.
- **Actuators**: Rear motor control (2 motors), Buzzer.

### Pin Assignments

| Function              | Pin  | GPIO | Wire To            | Notes              |
| --------------------- | ---- | ---- | ------------------ | ------------------ |
| **L298N Rear Driver** |      |      |                    |
| Left Motor Speed      | ENA  | 13   | L298N ENA (PWM)    | PWM 0-255          |
| Left Motor Dir A      | IN1  | 12   | L298N IN1          | HIGH/LOW           |
| Left Motor Dir B      | IN2  | 14   | L298N IN2          | HIGH/LOW           |
| Right Motor Speed     | ENB  | 25   | L298N ENB (PWM)    | PWM 0-255          |
| Right Motor Dir A     | IN3  | 26   | L298N IN3          | HIGH/LOW           |
| Right Motor Dir B     | IN4  | 27   | L298N IN4          | HIGH/LOW           |
| **Front Ultrasonic**  |      |      |                    |
| Trigger               | TRIG | 5    | HC-SR04 Trig       | 10μs pulse         |
| Echo                  | ECHO | 18   | HC-SR04 Echo       | 5V → use divider   |
| **Rear Ultrasonic**   |      |      |                    |
| Trigger               | TRIG | 19   | HC-SR04 Trig       | 10μs pulse         |
| Echo                  | ECHO | 21   | HC-SR04 Echo       | 5V → use divider   |
| **MQ-2 Gas Sensor**   |      |      |                    |
| Analog                | AO   | 32   | MQ-2 A0            | ADC1 (0-4095)      |
| Digital               | DO   | 33   | MQ-2 D0            | Optional threshold |
| **Buzzer**            |      |      |                    |
| Output                | PWM  | 2    | Buzzer+ via 220Ω   | Active/passive     |
| **Power**             |      |      |                    |
| VIN                   | 5V   | VIN  | Buck converter +5V |                    |
| GND                   | GND  | GND  | Common ground      | **CRITICAL**       |

---

## 🎮 Front ESP32 (Motor Slave Node)

### Role

- **Motor Slave**: Connects to Back ESP32 via WiFi/WebSocket.
- **Actuators**: Controls 4 front motors (via 2x L298N drivers).

### Pin Assignments

| Function            | Pin | GPIO | Wire To            | Notes        |
| ------------------- | --- | ---- | ------------------ | ------------ |
| **L298N Driver #1** |     |      |                    |
| Motor 1 Speed       | ENA | 13   | L298N #1 ENA       | PWM 0-255    |
| Motor 1 Dir A       | IN1 | 12   | L298N #1 IN1       | HIGH/LOW     |
| Motor 1 Dir B       | IN2 | 14   | L298N #1 IN2       | HIGH/LOW     |
| Motor 2 Speed       | ENB | 25   | L298N #1 ENB       | PWM 0-255    |
| Motor 2 Dir A       | IN3 | 26   | L298N #1 IN3       | HIGH/LOW     |
| Motor 2 Dir B       | IN4 | 27   | L298N #1 IN4       | HIGH/LOW     |
| **L298N Driver #2** |     |      |                    |
| Motor 3 Speed       | ENA | 4    | L298N #2 ENA       | PWM 0-255    |
| Motor 3 Dir A       | IN1 | 16   | L298N #2 IN1       | HIGH/LOW     |
| Motor 3 Dir B       | IN2 | 17   | L298N #2 IN2       | HIGH/LOW     |
| Motor 4 Speed       | ENB | 18   | L298N #2 ENB       | PWM 0-255    |
| Motor 4 Dir A       | IN3 | 19   | L298N #2 IN3       | HIGH/LOW     |
| Motor 4 Dir B       | IN4 | 21   | L298N #2 IN4       | HIGH/LOW     |
| **Power**           |     |      |                    |
| VIN                 | 5V  | VIN  | Buck converter +5V |              |
| GND                 | GND | GND  | Common ground      | **CRITICAL** |

---

## 📷 ESP32-CAM (Telemetry Bridge)

### Role

- **Telemetry Client**: Connects to Back ESP32 via WiFi/WebSocket.
- **Status Reporting**: Reports health status to Master.

### Pin Assignments (AI-Thinker ESP32-CAM)

| Function                     | Pin     | GPIO                     | Notes                        |
| ---------------------------- | ------- | ------------------------ | ---------------------------- |
| **Built-in Camera (OV2640)** |         |                          |                              |
| All camera pins              | Various | 0,5,18,19,21-27,34-36,39 | **DO NOT USE**               |
| **Flash LED**                |         |                          |                              |
| White LED                    | PWM     | 4                        | Built-in, PWM for brightness |
| **Status LED**               |         |                          |                              |
| External LED                 | PWM     | 33                       | LED+ via 220Ω → GND          |
| **Programming (FTDI)**       |         |                          |                              |
| TX0                          | Serial  | 1                        | FTDI RX                      |
| RX0                          | Serial  | 3                        | FTDI TX                      |
| GPIO 0                       | Boot    | 0                        | Hold LOW for upload          |

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

---

## 🔍 Troubleshooting

### Motors don't spin

- Check L298N VCC has 12V from battery
- Verify ESP32 GND connected to L298N GND
- Login to Dashboard and check "Motor" telemetry values

### Sensors give erratic readings

- Check 5V rail stability
- Ensure WebSocket connection is active (see Dashboard status)

---

**Document Version:** 3.0 (Pure WiFi Refactor)
**Last Updated:** December 2025
**Author:** Project Nightfall Team
