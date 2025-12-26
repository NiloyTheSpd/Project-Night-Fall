# Project Nightfall: Autonomous Rescue Robot 🤖🚨

**Status:** ✅ **ACTIVE DEVELOPMENT** - Firmware refactored, ESP-NOW + WiFi hybrid architecture, React dashboard operational

**Project Nightfall** is a specialized autonomous robotic platform designed for search and rescue operations in hazardous environments.

Unlike standard rovers, Nightfall utilizes a distributed computing architecture with three ESP32 controllers communicating via **ESP-NOW** for low-latency telemetry, while an **ESP32-CAM** serves as a WiFi telemetry bridge with a real-time React dashboard for monitoring.

---

## 🌟 Key Features

- **Distributed ESP-NOW Architecture**  
  Three-board system: Back ESP32 (Master/Sensors), Front ESP32 (Motor Slave), Camera ESP32 (Telemetry Bridge + WebSocket Server).

- **Real-Time Dashboard**  
  React-based web interface for live telemetry visualization, motor status, sensor readings, and system health metrics.

- **Auto-Climb Mode**  
  Dynamic torque distribution. When the back senses a climbable obstacle, it boosts front torque while the rear pushes, leveraging friction to climb debris.

- **Hazard Detection**  
  Integrated MQ-2 Gas/Smoke sensor. If toxic levels are detected, the robot triggers alerts and halts all motion immediately.

- **Zero-Latency Control**  
  Uses ESP-NOW (peer-to-peer WiFi) for `<10ms` latency between boards. Telemetry broadcasted every 500ms.

- **WebSocket Telemetry Bridge**  
  Camera ESP32 receives ESP-NOW packets and forwards them to web clients via WebSocket (port 8888/ws).

---

## 🛠️ Hardware Architecture

### Component List

- **Controllers:** 2× ESP32 DevKit V1 (Back Master, Front Slave), 1× ESP32-CAM (AI-Thinker Telemetry Bridge)
- **Drivers:** 2× L298N Dual H-Bridge Motor Drivers
- **Motors:** 4× (or 6×) 12V High-Torque DC Geared Motors (300–500 RPM)
- **Sensors:** 2× HC-SR04 Ultrasonic, 1× MQ-2 Gas/Smoke Sensor
- **Power:** 3S LiPo (11.1V) or 12V Lead Acid + 2× LM2596 Buck Converters (5V logic)
- **Communication:** ESP-NOW for telemetry broadcast (500ms), WiFi for dashboard (WebSocket on port 8888)

### Three-Board System Roles

**Back ESP32 (Master/Brain):**

- Sensor readings (ultrasonic front/rear, gas level)
- Autonomous navigation logic
- Rear motor control (2 motors via L298N)
- Broadcasts telemetry via ESP-NOW every 500ms

**Front ESP32 (Motor Slave):**

- Receives motor commands via ESP-NOW
- Controls 4 front motors (2× L298N drivers)
- Executes movement (forward, turn, climb)

**Camera ESP32 (Telemetry Bridge):**

- Receives ESP-NOW telemetry from Back ESP32
- Connects to WiFi AP "ProjectNightfall"
- Runs WebSocket server (port 8888, path `/ws`)
- Forwards telemetry JSON to dashboard clients
- Logs telemetry to Serial for debugging

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

_WebSocket Server + ESP-NOW Receiver_

| Component       | Pin       | Function                           |
| --------------- | --------- | ---------------------------------- |
| Built-in Camera | Internal  | Not currently used                 |
| WiFi            | Built-in  | Connects to "ProjectNightfall" AP  |
| ESP-NOW         | Built-in  | Receives telemetry from Back ESP32 |
| WebSocket       | Port 8888 | `/ws` endpoint for dashboard       |

---

## 📂 Project Structure

```text
Project-Nightfall/
├── src/
│   ├── main_rear.cpp       # Back ESP32: Master/Sensors/Navigation
│   ├── main_front.cpp      # Front ESP32: Motor Slave
│   └── main_camera.cpp     # Camera ESP32: Telemetry Bridge + WebSocket
├── include/
│   ├── config.h            # Global configuration (WiFi, thresholds, intervals)
│   └── pins.h              # Pin definitions for all three boards
├── lib/
│   ├── Communication/      # Message protocol and WiFi utilities
│   ├── Motors/             # Motor control abstraction
│   ├── Sensors/            # Ultrasonic and gas sensor drivers
│   ├── Navigation/         # Autonomous navigation logic
│   └── Safety/             # Safety monitor and hazard detection
├── robot-dashboard/        # React dashboard for real-time monitoring
│   ├── src/
│   │   ├── DashboardEnhanced.jsx  # Main dashboard component
│   │   └── ...
│   ├── .env.example        # Configuration template
│   ├── package.json
│   └── README.md
├── docs/                   # Architecture diagrams and documentation
├── platformio.ini          # PlatformIO project configuration
└── README.md
```

---

## 🚀 Getting Started

### Prerequisites

- VS Code with PlatformIO Extension
- Node.js 18+ and npm (for React dashboard)
- ESP32 USB drivers (CP2102/CH340)

### Installation & Flashing

#### 1. Clone the Repo

```bash
git clone https://github.com/NiloyTheSpd/Night-Fall.git
cd Night-Fall
```

#### 2. Flash ESP32 Firmware

**Back ESP32 (Master):**

```bash
python -m platformio run -e back_esp32 --target upload
```

**Front ESP32 (Motor Slave):**

```bash
python -m platformio run -e front_esp32 --target upload
```

**Camera ESP32 (Telemetry Bridge):**

```bash
python -m platformio run -e camera_esp32 --target upload
```

_Note: Put ESP32-CAM in boot mode (GPIO0 to GND, press RESET) before uploading._

#### 3. Setup Dashboard

```bash
cd robot-dashboard
npm install
cp .env.example .env
# Edit .env to set VITE_TELEMETRY_WS=ws://192.168.4.1:8888/ws
npm run dev
```

Access dashboard at `http://localhost:5173`

---

### 🔑 Network Configuration

**WiFi Access Point:**

- SSID: `ProjectNightfall`
- Password: `rescue2025`
- Camera ESP32 IP: `192.168.4.1` (WebSocket server)

**ESP-NOW Communication:**

- Back ESP32 broadcasts telemetry every 500ms
- Front ESP32 receives motor commands
- Camera ESP32 receives telemetry and forwards to WebSocket

_MAC addresses are auto-configured in firmware. No manual MAC setup required._

---

### ⚡ Verify Operation

**Serial Monitor (Back ESP32):**

```bash
python -m platformio device monitor -p COM4 -b 115200
```

You should see sensor readings and telemetry broadcasts.

**Serial Monitor (Camera ESP32):**

```bash
python -m platformio device monitor -p COM7 -b 115200
```

You should see:

- WiFi connection confirmation
- ESP-NOW initialization
- Incoming JSON telemetry packets
- WebSocket client connections

**Dashboard:**

- Open `http://localhost:5173`
- Check header shows `WS: CONNECTED`
- Verify Back/Front/Camera panels update with live data
- Monitor system events log at bottom

---

### 🧪 Calibration

- **Gas Sensor:**
  The MQ-2 sensor requires a warm-up period. Adjust `GAS_THRESHOLD` in `main_rear.cpp` based on your environment's "clean air" value.

- **Ultrasonic:**
  Verify distances in the Serial Monitor. Adjust `MIN_SAFE_DIST_CM` if the robot stops too early or too late.

---

## 🧠 System Logic Flow

1. **Initialization**

   - All three ESP32s boot
   - Camera ESP32 connects to WiFi AP "ProjectNightfall"
   - Camera ESP32 starts WebSocket server on port 8888
   - Back ESP32 initializes ESP-NOW in broadcast mode
   - Front ESP32 initializes ESP-NOW receiver

2. **Sensor Scan (Back ESP32)**

   - Reads ultrasonic front/rear distances every 100ms
   - Reads gas sensor analog value
   - Processes navigation state (idle, moving, scanning)

3. **Decision & Command**

   - **Path Clear:**
     Broadcast motor command via ESP-NOW. Both Back (rear motors) and Front (4 motors) drive at `NORMAL_SPEED`.
   - **Obstacle Close (<20 cm):**
     Trigger `CLIMB_MODE`. Front motors boost torque, rear motors push hard.
   - **Obstacle Very Close (<10 cm):**
     Stop → Reverse → Turn (avoid maneuver).

4. **Telemetry Broadcast**

   - Back ESP32 sends telemetry JSON via ESP-NOW every 500ms
   - Camera ESP32 receives and forwards to WebSocket clients
   - Dashboard renders live metrics

5. **Hazard Interrupt**
   - If gas level exceeds threshold → trigger buzzer
   - Broadcast hazard alert via ESP-NOW
   - Front ESP32 executes **Emergency Stop**
   - Dashboard shows alert in event log

---

## 📊 Dashboard Features

- **Three-Board Monitoring**: Real-time status of Back, Front, Camera ESP32s
- **Live Metrics**: Battery, temperature, CPU load, signal strength
- **Sensor Visualization**: Distance sensors, gas level with alert threshold
- **Motor PWM Bars**: Visual representation of motor speeds (0-255)
- **Event Logging**: System events with timestamps
- **WebSocket Auto-Reconnect**: Resilient connection with 3-second backoff
- **Debug Mode**: View raw JSON payloads (set `VITE_DEBUG_MODE=true` in `.env`)

---

## 🤝 Contributing

1. Fork the Project
2. Create your Feature Branch

   ```bash
   git checkout -b feature/AmazingFeature
   ```

3. Commit your Changes

   ```bash
   git commit -m "Add some AmazingFeature"
   ```

4. Push to the Branch

   ```bash
   git push origin feature/AmazingFeature
   ```

5. Open a Pull Request

---

## 📄 License

Distributed under the **MIT License**. See `LICENSE` for more information.

---

Built with ❤️ for safety and rescue innovation.

```

```
