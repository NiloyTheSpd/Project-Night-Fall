# Project Nightfall: Autonomous Rescue Robot 🤖🚨

**Status:** ✅ **ACTIVE DEVELOPMENT** - Firmware refactored to Modular Pure WiFi Architecture

**Project Nightfall** is a specialized autonomous robotic platform designed for search and rescue operations in hazardous environments.

It utilizes a distributed computing architecture with three ESP32 controllers communicating via a **centralized WiFi + WebSocket** network for reliable, low-latency control and telemetry. A React-based dashboard provides real-time monitoring.

---

## 🌟 Key Features

- **Pure WiFi Architecture**
  Three-board system:

  - **Back ESP32**: Master Brain, WiFi AP, WebSocket Server.
  - **Front ESP32**: Motor Slave (WebSocket Client).
  - **Camera ESP32**: Telemetry Bridge (WebSocket Client).

- **Modular Codebase**
  Refactored into dedicated modules: `SafetyManager`, `SensorManager`, `StateMachine` (FSM), `Navigation`, and strict `MessageProtocol`.

- **Real-Time Dashboard**
  React-based web interface for live telemetry visualization, motor status, sensor readings, and system health metrics.

- **Safety First**
  Centralized `SafetyManager` monitors hazards (Gas/Smoke). Emergency Stop overrides all other states immediately.

- **Auto-Climb & Autonomy**
  Smart obstacle avoidance and dynamic torque distribution for climbing debris.

---

## 🛠️ Hardware Architecture

### Component List

- **Controllers:** 2× ESP32 DevKit V1 (Back Master, Front Slave), 1× ESP32-CAM (AI-Thinker Telemetry Bridge)
- **Drivers:** 2× L298N Dual H-Bridge Motor Drivers (Back: 1 driver, Front: 2 drivers)
- **Motors:** 6× 12V High-Torque DC Geared Motors (300–500 RPM)
- **Sensors:** 2× HC-SR04 Ultrasonic, 1× MQ-2 Gas/Smoke Sensor
- **Power:** 3S LiPo (11.1V) or 12V Lead Acid

### Three-Board System Roles

**1. Back ESP32 (Master/Brain)**

- **WiFi Mode:** Access Point (`ProjectNightfall`) + WebSocket Server (port 8888).
- **Responsibilities:**
  - Runs the Master FSM (Finite State Machine).
  - Manages all sensors (Ultrasonic, Gas).
  - Controls Rear Motors directly.
  - Broadcasts telemetry to all clients.
  - Sends motor commands to Front ESP32.

**2. Front ESP32 (Motor Slave)**

- **WiFi Mode:** Station (Client) -> Connects to Back ESP32.
- **Responsibilities:**
  - Receives motor commands via WebSocket.
  - Drives 4 Front Motors (2x L298N).

**3. Camera ESP32 (Telemetry Bridge)**

- **WiFi Mode:** Station (Client) -> Connects to Back ESP32.
- **Responsibilities:**
  - Connects as a client to report status.
  - Future: Video Streaming.

---

## 🔌 Wiring & Pinout

### 1. Back ESP32 (Master/Sensor Node)

_Responsible for Navigation Logic, Sensors & Rear Traction_

| Component       | Pin             | ESP32 GPIO | Function              |
| --------------- | --------------- | ---------- | --------------------- |
| L298N (Rear)    | ENA / ENB       | 13 / 25    | PWM Speed Control     |
|                 | IN1 / IN2       | 12 / 14    | Left Motor Direction  |
|                 | IN3 / IN4       | 26 / 27    | Right Motor Direction |
| Ultrasonic      | Front Trig/Echo | 5 / 18     | Front Distance Sensor |
|                 | Rear Trig/Echo  | 19 / 21    | Rear Distance Sensor  |
| MQ-2 Gas Sensor | Analog          | 32         | Gas Level Reading     |
| Buzzer          | PWM             | 2          | Alert Output          |

---

### 2. Front ESP32 (Motor Slave Node)

_Responsible for Front Traction (4 Motors via 2× L298N)_

| Component | Pin       | ESP32 GPIO | Function          |
| --------- | --------- | ---------- | ----------------- |
| L298N #1  | ENA / ENB | 13 / 25    | Motor 1/2 PWM     |
|           | IN1 / IN2 | 12 / 14    | Motor 1 Direction |
|           | IN3 / IN4 | 26 / 27    | Motor 2 Direction |
| L298N #2  | ENA / ENB | 4 / 18     | Motor 3/4 PWM     |
|           | IN1 / IN2 | 16 / 17    | Motor 3 Direction |
|           | IN3 / IN4 | 19 / 21    | Motor 4 Direction |

---

### 3. Camera ESP32 (Telemetry Bridge)

_WebSocket Client_

| Component | Pin      | Function                            |
| --------- | -------- | ----------------------------------- |
| WiFi      | Built-in | Connects to "ProjectNightfall" AP   |
| WebSocket | Built-in | Connects to `ws://192.168.4.1:8888` |

---

## 📂 Project Structure

```text
Project-Nightfall/
├── src/
│   ├── main_rear.cpp       # Master Brain: FSM, Sensors, AP/Server
│   ├── main_front.cpp      # Motor Slave: Client
│   └── main_camera.cpp     # Telemetry Client
├── include/
│   ├── config.h            # Global configuration
│   └── pins.h              # Pin definitions
├── lib/
│   ├── Communication/      # MessageProtocol, WiFiManager
│   ├── Control/            # StateMachine (FSM)
│   ├── Motors/             # L298N Driver
│   ├── Navigation/         # Autonomy Logic
│   ├── Safety/             # SafetyManager (Hazards)
│   └── Sensors/            # SensorManager, Drivers
├── robot-dashboard/        # React Dashboard
└── platformio.ini          # Build Configuration
```

---

## 🚀 Getting Started

1.  **Hardware Setup**: Follow `WIRING_GUIDE.md` to connect motors and sensors.
2.  **Flash Firmware**:
    - Connect Back ESP32 -> Build/Upload `back_esp32` env.
    - Connect Front ESP32 -> Build/Upload `front_esp32` env.
    - Connect Camera ESP32 -> Build/Upload `camera_esp32` env.
3.  **Boot Up**:
    - Power on Back ESP32 first. It creates the WiFi AP `ProjectNightfall`.
    - Power on Front/Camera. They will auto-connect.
4.  **Dashboard**:
    - Connect PC to WiFi `ProjectNightfall`.
    - Open Dashboard (Local React App).
