# Project Nightfall - Architecture & Pinout Reference

## System Architecture Diagram

```
┌─────────────────────────────────────────────────────────────┐
│                    PROJECT NIGHTFALL SYSTEM                 │
└─────────────────────────────────────────────────────────────┘

                        ┌──────────────────┐
                        │   ESP32-CAM      │
                        │  (Vision Module) │
                        └────────┬─────────┘
                                 │
                    ┌────────────┴────────────┐
                    │                         │
              WiFi AP Stream           UART2 (Tx/Rx)
              192.168.4.1:81              115200 baud
                    │                         │
                    ▼                         │
            ┌────────────────┐              │
            │ WiFi Clients   │              │
            │ (Browsers)     │              │
            └────────────────┘              │
                                            │
                        ┌───────────────────┘
                        │
                        ▼
        ┌───────────────────────────────┐
        │   ESP32 #1 (Front Master)     │
        │                               │
        │  ├─ Motor Control (L298N)    │
        │  ├─ Front Ultrasonic Sensor  │
        │  ├─ Gas Sensor (MQ-2)        │
        │  ├─ Safety Monitor           │
        │  ├─ Autonomous Navigation    │
        │  └─ UART Master              │
        └───┬────────┬──────────────────┘
            │        │
       UART1│        │UART2
      115200│ baud   │115200 baud
            │        │
            ▼        ▼
        ┌─────────┐
        │ESP32 #2 │  ◄─────────────────────┐
        │(Rear    │                         │
        │Slave)   │                         │
        │         │                         │
        │├─Motor  │     [Manual Control]    │
        │├─Sensor │     Serial Console      │
        │└─Safety │     (forward, back,     │
        │         │      left, right, etc)  │
        └─────────┘

═══════════════════════════════════════════════════════════════

    COMMUNICATION FLOW:
    ─────────────────

    Front (Master) → Rear (Slave)
    ├─ Heartbeat (1Hz) - "I'm alive"
    ├─ Sensor Updates (10Hz) - "Front distance: X cm"
    ├─ Motor Commands (2Hz) - "Turn left now"
    └─ Emergency (Immediate) - "STOP ALL!"

    Rear (Slave) → Front (Master)
    ├─ Sensor Feedback (10Hz) - "Rear distance: X cm"
    └─ Status Messages - "Motor active, safe"

    Front (Master) → Camera
    ├─ Heartbeat (1Hz) - "System alive"
    └─ Commands - "Flash on", "Capture"

    Camera → Front (Master)
    ├─ Status (5Hz) - "FPS: 30, Streaming"
    └─ Heartbeat ACK - "Camera alive"
```

---

## Front Controller (ESP32 #1) Pinout

```
┌─────────────────────────────────────────────────────┐
│          FRONT CONTROLLER (ESP32 #1)                │
│                                                      │
│  MOTOR CONTROL (L298N Driver)                      │
│  ┌──────────────────────────────────────────────┐  │
│  │ Left Motor:           Right Motor:           │  │
│  │ ENA (PWM) → GPIO18   ENB (PWM) → GPIO22    │  │
│  │ IN1       → GPIO19   IN3       → GPIO23    │  │
│  │ IN2       → GPIO21   IN4       → GPIO25    │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  ULTRASONIC SENSOR (Front)                         │
│  ┌──────────────────────────────────────────────┐  │
│  │ Trigger (TRIG) → GPIO26                      │  │
│  │ Echo (ECHO)    → GPIO27                      │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  GAS SENSOR (MQ-2)                                 │
│  ┌──────────────────────────────────────────────┐  │
│  │ Analog Output → GPIO32 (ADC)                │  │
│  │ Digital Output → GPIO33 (GPIO)              │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  UART COMMUNICATION                                 │
│  ┌──────────────────────────────────────────────┐  │
│  │ To Rear Controller:                          │  │
│  │   TX (Serial1) → GPIO16                     │  │
│  │   RX (Serial1) ← GPIO17                     │  │
│  │ To Camera Module:                            │  │
│  │   TX (Serial2) → GPIO14                     │  │
│  │   RX (Serial2) ← GPIO12                     │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  OTHER                                              │
│  ┌──────────────────────────────────────────────┐  │
│  │ Buzzer → GPIO13                             │  │
│  │ LED (Builtin) → GPIO2                       │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  POWER REQUIREMENTS:                               │
│  ├─ Logic: 5V @ 200mA                            │
│  ├─ Motors: 12V @ 2A (via L298N)               │
│  └─ Total: Separate power supplies recommended   │
└─────────────────────────────────────────────────────┘

TOTAL GPIO USED: 18 pins
PWM CHANNELS: 2 (motors) + 1 (buzzer) = 3
ADC CHANNELS: 2 (gas sensor)
UART PORTS: 2 (Serial1, Serial2)
```

---

## Rear Controller (ESP32 #2) Pinout

```
┌─────────────────────────────────────────────────────┐
│          REAR CONTROLLER (ESP32 #2)                 │
│                                                      │
│  MOTOR CONTROL (L298N Driver)                      │
│  ┌──────────────────────────────────────────────┐  │
│  │ Left Motor:           Right Motor:           │  │
│  │ ENA (PWM) → GPIO18   ENB (PWM) → GPIO22    │  │
│  │ IN1       → GPIO19   IN3       → GPIO23    │  │
│  │ IN2       → GPIO21   IN4       → GPIO25    │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  ULTRASONIC SENSOR (Rear)                          │
│  ┌──────────────────────────────────────────────┐  │
│  │ Trigger (TRIG) → GPIO26                      │  │
│  │ Echo (ECHO)    → GPIO27                      │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  UART COMMUNICATION                                 │
│  ┌──────────────────────────────────────────────┐  │
│  │ To Front Controller:                         │  │
│  │   TX (Serial1) → GPIO16                     │  │
│  │   RX (Serial1) ← GPIO17                     │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  OTHER                                              │
│  ┌──────────────────────────────────────────────┐  │
│  │ LED (Builtin) → GPIO2                       │  │
│  └──────────────────────────────────────────────┘  │
│                                                      │
│  POWER REQUIREMENTS:                               │
│  ├─ Logic: 5V @ 200mA                            │
│  ├─ Motors: 12V @ 2A (via L298N)               │
│  └─ Total: Separate power supplies recommended   │
└─────────────────────────────────────────────────────┘

TOTAL GPIO USED: 8 pins
PWM CHANNELS: 2 (motors)
UART PORTS: 1 (Serial1)
```

---

## ESP32-CAM Module Pinout

```
┌──────────────────────────────────────────────────┐
│         ESP32-CAM MODULE                         │
│                                                   │
│  CAMERA SENSOR (OV2640)                         │
│  ┌────────────────────────────────────────────┐ │
│  │ Power Down    → GPIO32 (PWDN)             │ │
│  │ Reset         → GPIO-1 (RESET)            │ │
│  │ Clock (XCLK)  → GPIO0 (20MHz)            │ │
│  │ I2C Data      → GPIO26 (SIOD)            │ │
│  │ I2C Clock     → GPIO27 (SIOC)            │ │
│  │                                            │ │
│  │ Data Pins (8-bit):                        │ │
│  │ D0 → GPIO5    D1 → GPIO18                │ │
│  │ D2 → GPIO19   D3 → GPIO21                │ │
│  │ D4 → GPIO36   D5 → GPIO39                │ │
│  │ D6 → GPIO34   D7 → GPIO35                │ │
│  │                                            │ │
│  │ Sync Pins:                                │ │
│  │ VSYNC (VSYNC) → GPIO25                   │ │
│  │ HREF (HREF)   → GPIO23                   │ │
│  │ PCLK (PCLK)   → GPIO22                   │ │
│  └────────────────────────────────────────────┘ │
│                                                   │
│  UART COMMUNICATION                             │
│  ┌────────────────────────────────────────────┐ │
│  │ To Front Controller:                       │ │
│  │   TX (Serial1) → GPIO14 (U1TXD)          │ │
│  │   RX (Serial1) ← GPIO15 (U1RXD)          │ │
│  └────────────────────────────────────────────┘ │
│                                                   │
│  FLASH LED                                       │
│  ┌────────────────────────────────────────────┐ │
│  │ Flash LED → GPIO4 (Active High)           │ │
│  └────────────────────────────────────────────┘ │
│                                                   │
│  OTHER                                           │
│  ┌────────────────────────────────────────────┐ │
│  │ Red LED (Builtin) → GPIO33                │ │
│  │ White LED (Flash) → GPIO4                 │ │
│  └────────────────────────────────────────────┘ │
│                                                   │
│  POWER REQUIREMENTS:                            │
│  ├─ USB Power: 5V @ 500mA (from USB)         │ │
│  └─ Or: 5V external, GND to GND              │ │
└──────────────────────────────────────────────────┘

TOTAL GPIO USED: 18 pins
I2C: 2 pins (for camera sensor)
UART: 2 pins (Serial1)
SPI: 4 pins (builtin, not exposed)
```

---

## Motor Driver Wiring (L298N Module)

```
┌─────────────────────────────────────────────────┐
│           L298N DUAL MOTOR DRIVER               │
│                                                  │
│  POWER CONNECTIONS:                            │
│  ┌──────────────────────────────────────────┐  │
│  │ +12V → Motor Power Input                │  │
│  │ GND  → Motor Power GND (common with ESP) │  │
│  │ +5V  → Logic Power Input                │  │
│  │ GND  → Logic Power GND (common)          │  │
│  └──────────────────────────────────────────┘  │
│                                                  │
│  MOTOR A (Left Motor) Connections:             │
│  ┌──────────────────────────────────────────┐  │
│  │ OUT1 → Motor A Positive Terminal         │  │
│  │ OUT2 → Motor A Negative Terminal         │  │
│  │ IN1  ← GPIO (Direction A)               │  │
│  │ IN2  ← GPIO (Direction B)               │  │
│  │ ENA  ← GPIO PWM (Speed Control)         │  │
│  └──────────────────────────────────────────┘  │
│                                                  │
│  MOTOR B (Right Motor) Connections:            │
│  ┌──────────────────────────────────────────┐  │
│  │ OUT3 → Motor B Positive Terminal         │  │
│  │ OUT4 → Motor B Negative Terminal         │  │
│  │ IN3  ← GPIO (Direction A)               │  │
│  │ IN4  ← GPIO (Direction B)               │  │
│  │ ENB  ← GPIO PWM (Speed Control)         │  │
│  └──────────────────────────────────────────┘  │
│                                                  │
│  MOTOR CONTROL LOGIC:                          │
│  ┌──────────────────────────────────────────┐  │
│  │ IN1=HIGH, IN2=LOW   → Forward            │  │
│  │ IN1=LOW,  IN2=HIGH  → Backward           │  │
│  │ IN1=HIGH, IN2=HIGH  → Brake              │  │
│  │ IN1=LOW,  IN2=LOW   → Coast              │  │
│  │ ENA/ENB PWM (0-255) → Speed Control     │  │
│  └──────────────────────────────────────────┘  │
└─────────────────────────────────────────────────┘
```

---

## Ultrasonic Sensor Wiring (HC-SR04)

```
┌──────────────────────────────────────┐
│   HC-SR04 ULTRASONIC SENSOR          │
│                                       │
│  Pin Connections:                    │
│  ┌──────────────────────────────────┐ │
│  │ VCC   → 5V Power                │ │
│  │ GND   → Ground                  │ │
│  │ TRIG  → GPIO (Trigger Pulse)   │ │
│  │ ECHO  → GPIO (Echo Pulse)      │ │
│  └──────────────────────────────────┘ │
│                                       │
│  Operation:                           │
│  1. Send 10µs pulse to TRIG          │
│  2. Measure pulse width on ECHO      │
│  3. Calculate: Distance = Time*340/2 │
│                                       │
│  Timing:                              │
│  ├─ Trigger pulse: 10µs              │
│  ├─ Echo pulse: 150-25000µs          │
│  ├─ Measurement range: 2cm to 400cm  │
│  ├─ Update rate: Max 50Hz            │
│  └─ Accuracy: ±3cm typical           │
└──────────────────────────────────────┘
```

---

## Gas Sensor Wiring (MQ-2)

```
┌──────────────────────────────────────┐
│      MQ-2 GAS SENSOR                 │
│                                       │
│  Pin Connections:                    │
│  ┌──────────────────────────────────┐ │
│  │ VCC     → 5V Power              │ │
│  │ GND     → Ground                │ │
│  │ A0/AO  → GPIO ADC (Analog)     │ │
│  │ D0/DO  → GPIO Digital          │ │
│  └──────────────────────────────────┘ │
│                                       │
│  Modes:                              │
│  ├─ Analog: Continuous PPM reading  │ │
│  ├─ Digital: Threshold alarm        │ │
│  └─ Best: Use analog + threshold   │ │
│                                       │
│  Calibration:                        │
│  ├─ Warm-up: 24 hours               │ │
│  ├─ Baseline: Fresh air = 400µS     │ │
│  ├─ Sensitivity: Set via pot        │ │
│  └─ Response time: 10-30 seconds    │ │
│                                       │
│  Detection:                          │
│  ├─ Propane: 200-10000 ppm         │ │
│  ├─ Butane: 200-10000 ppm          │ │
│  ├─ LPG: 200-10000 ppm             │ │
│  └─ Smoke/CO: Varies               │ │
└──────────────────────────────────────┘
```

---

## Power Distribution Diagram

```
┌────────────────────────────────────────────────────┐
│           POWER DISTRIBUTION SYSTEM                │
└────────────────────────────────────────────────────┘

                     ┌──────────────────────┐
                     │  Battery Pack        │
                     │  (12V DC Supply)     │
                     └──────────┬───────────┘
                                │
                    ┌───────────┴────────────┐
                    │                         │
            ┌───────▼────────┐    ┌──────────▼────────┐
            │  Motor Power   │    │  Logic Power      │
            │  12V Direct    │    │  (via Voltage Reg)│
            │  ├─L298N Input │    │  5V Output        │
            │  ├─Front Motors│    └──────────┬────────┘
            │  └─Rear Motors │               │
            └────────────────┘        ┌──────┴──────┐
                                      │             │
                          ┌───────────▼──┐  ┌──────▼────────┐
                          │ Front ESP32   │  │ Rear ESP32    │
                          │ + Sensors     │  │ + Sensors     │
                          └───────────────┘  └───────────────┘

    Front → Rear (UART) → Camera → WiFi Clients

RECOMMENDED POWER BUDGET:
├─ Motors: 12V @ 2A = 24W
├─ Front ESP32: 5V @ 0.3A = 1.5W
├─ Rear ESP32: 5V @ 0.3A = 1.5W
├─ ESP32-CAM: 5V @ 0.5A = 2.5W
└─ Total: ~30W (before peaks)

BATTERY SIZING:
├─ Motor-dominant system
├─ Heavy load during climbing
├─ Recommendation: 12V 2Ah Li-Po (minimum)
└─ Or: 12V 5000mAh (recommended for longer runtime)
```

---

## Communication Timing Diagram

```
Time →

UART1 (Front ↔ Rear):
├─ T+0ms:    Heartbeat sent (Front)
├─ T+10ms:   Received (Rear)
├─ T+100ms:  Sensor Update sent (Front)
├─ T+105ms:  Sensor Feedback sent (Rear)
├─ T+200ms:  Safety Check (both)
├─ T+500ms:  Navigation Decision (Front)
├─ T+510ms:  Motor Command sent (Front)
└─ T+520ms:  Command executed (Rear)

UART2 (Front ↔ Camera):
├─ T+0ms:    Heartbeat sent (Front)
├─ T+10ms:   Heartbeat ACK (Camera)
├─ T+200ms:  Status requested
├─ T+210ms:  Status received (FPS, clients, etc)
└─ On demand: Flash, Capture commands

WiFi (Camera → Clients):
├─ T+0ms:    Client connects
├─ T+20ms:   HTTP header sent
├─ T+30ms:   MJPEG stream starts
├─ T+35ms:   Frame 1 (33ms later: Frame 2 @30FPS)
├─ T+68ms:   Frame 2
└─ Continuous until disconnect

LOOP TIMING (Front Controller):
│ 0ms      100ms    200ms    300ms    400ms    500ms
├┼─────────┼────────┼────────┼────────┼────────┼
│ Sensor ━━ Sensor ━━ Sensor ━━ Sensor ━━ Sensor
│  Safety ━━ Safety ━━ Safety ━━ Safety ━━ Safety
│      Nav ════════════════════════════ Nav
│         Heartbeat ━━━━━━━━━━━━━━━━━━ HB
│      UART Communication (continuous)
```

---

## Emergency Stop Propagation

```
Emergency Trigger (any source):
│
├─ Front Safety Monitor detects violation
│  └─ motorControl.emergencyStop() (10ms)
│     └─ Set all motor pins to BRAKE
│        └─ Rear has 3sec timeout backup
│
├─ Emergency broadcast via UART (5ms)
│  │ {"type": "emergency", "data": {"emergency_stop": true}}
│  │
│  ├─ → Rear ESP32 receives (10ms)
│  │     └─ motorControl.emergencyStop() (10ms)
│  │        └─ All rear motors stop
│  │
│  └─ → Camera Module receives (10ms)
│      └─ Send status update: "emergency": true
│
├─ Buzzer Alert (dual beep, 100ms each)
│  └─ digitalWrite(BUZZER, HIGH) for 100ms
│     └─ delay 100ms
│        └─ digitalWrite(BUZZER, HIGH) for 100ms
│
└─ STATE = STATE_EMERGENCY
   └─ All control suspended
      └─ Manual reset required

TOTAL PROPAGATION TIME: <50ms (entire system stopped)
```

---

## Debug Connection Diagram

```
┌──────────────────────────────────────────────────┐
│         SERIAL DEBUG SETUP                       │
│                                                   │
│  USB → UART → ESP32              Monitor         │
│                                  115200 baud     │
│   Front ESP32 (main):                            │
│   ├─ System status every 5000 loops              │
│   ├─ Motor commands as executed                  │
│   ├─ Sensor readings (10Hz)                      │
│   └─ Errors and warnings                         │
│                                                   │
│   Rear ESP32 (secondary):                        │
│   ├─ Heartbeat reception                         │
│   ├─ Command reception and execution            │
│   ├─ Sensor readings (10Hz)                      │
│   └─ Status and errors                          │
│                                                   │
│   ESP32-CAM (tertiary):                         │
│   ├─ Initialization status                      │
│   ├─ WiFi AP status                             │
│   ├─ FPS and frame counts                       │
│   └─ Camera errors                              │
│                                                   │
│  LOGIC ANALYZER:                                 │
│  ├─ Monitor UART1 (Front↔Rear) @ 115200 baud  │
│  ├─ Monitor UART2 (Front↔Camera) @ 115200 baud│
│  ├─ Verify JSON message structure               │
│  ├─ Check heartbeat timing (should be 1Hz)      │
│  └─ Monitor motor PWM on GPIO18/22              │
└──────────────────────────────────────────────────┘
```

---

_Project Nightfall - Complete Architecture Reference_
_All pinouts, timing, and communication protocols documented_
_Ready for hardware assembly and integration testing_
