# Project Nightfall: Autonomous Rescue Robot 🤖🚨

**Project Nightfall** is a specialized autonomous robotic platform designed for search and rescue operations in hazardous environments.

Unlike standard rovers, Nightfall utilizes a distributed computing architecture with two ESP32 controllers (Front/Rear) communicating via **ESP-NOW** to manage traction and obstacle climbing logic independently, while a dedicated **ESP32-CAM** handles live video streaming.

---

## 🌟 Key Features

- **Distributed Processing**  
  Split-brain architecture (Front Master / Rear Slave) ensures motor timing doesn't block sensor reading.

- **Auto-Climb Mode**  
  Dynamic torque distribution. When the front senses a climbable obstacle, it boosts front torque while the rear pushes, leveraging friction to climb debris.

- **Hazard Detection**  
  Integrated MQ-2 Gas/Smoke sensor. If toxic levels are detected, the robot triggers a hardware interrupt, sounds an alarm, and halts all motion immediately.

- **Zero-Latency Control**  
  Uses ESP-NOW (peer-to-peer WiFi) for `<10ms` latency between front and rear traction systems.

- **Live Telemetry**  
  Dedicated video feed via ESP32-CAM for remote visual verification.

---

## 🛠️ Hardware Architecture

### Component List

- **Controllers:** 2× ESP32 DevKit V1, 1× ESP32-CAM (AI-Thinker)
- **Drivers:** 2× L298N Dual H-Bridge Motor Drivers
- **Motors:** 4× (or 6×) 12V High-Torque DC Geared Motors (300–500 RPM)
- **Sensors:** 2× HC-SR04 Ultrasonic, 1× MQ-2 Gas/Smoke Sensor
- **Power:** 3S LiPo (11.1V) or 12V Lead Acid + 2× LM2596 Buck Converters (5V logic)

---

## 🔌 Wiring & Pinout

### 1. Front ESP32 (Master Node)

_Responsible for Navigation Logic & Front Traction_

| Component     | Pin           | ESP32 GPIO | Function              |
| ------------- | ------------- | ---------- | --------------------- |
| L298N (Front) | ENA / ENB     | 13 / 25    | PWM Speed Control     |
|               | IN1 / IN2     | 12 / 14    | Left Motor Direction  |
|               | IN3 / IN4     | 27 / 26    | Right Motor Direction |
| Sensors       | Trig/Echo (L) | 5 / 18     | Ultrasonic Left       |
|               | Trig/Echo (R) | 19 / 21    | Ultrasonic Right      |

---

### 2. Rear ESP32 (Slave Node)

_Responsible for Rear Traction, Safety & Power Pushing_

| Component    | Pin       | ESP32 GPIO | Function              |
| ------------ | --------- | ---------- | --------------------- |
| L298N (Rear) | ENA / ENB | 13 / 25    | PWM Speed Control     |
|              | IN1 / IN2 | 12 / 14    | Left Motor Direction  |
|              | IN3 / IN4 | 27 / 26    | Right Motor Direction |
| Safety       | MQ-2 A0   | 34         | Analog Gas Reading    |
|              | Buzzer    | 4          | Alarm Output          |

---

## 📂 Project Structure

```text
Project-Nightfall/
├── src/
│   ├── main_front.cpp      # Code for the Master Node (Navigation Brain)
│   ├── main_rear.cpp       # Code for the Slave Node (Safety & Power)
│   └── main_camera.cpp     # Code for the Vision Node
├── include/                # Header files
├── docs/                   # Diagrams and Schematics
├── platformio.ini          # Project Configuration
└── README.md
```

---

## 🚀 Getting Started

### Prerequisites

- VS Code
- PlatformIO Extension

### Installation & Flashing

#### Clone the Repo

```bash
git clone https://github.com/NiloyTheSpd/Night-Fall.git
cd Night-Fall
```

---

### 🔑 Obtain MAC Addresses (Critical Step)

ESP-NOW requires the physical MAC address of the paired board.

1. Flash a simple **"Get MAC"** sketch to both ESP32s first.
2. Update `broadcastAddress[]` in `main_front.cpp` with the Rear ESP32's MAC.
3. Update `masterAddress[]` in `main_rear.cpp` with the Front ESP32's MAC.

---

### ⚡ Flash the Firmware

- Connect the **Front ESP32** and upload `main_front.cpp`.
- Connect the **Rear ESP32** and upload `main_rear.cpp`.
- Connect the **ESP32-CAM** (using an FTDI programmer) and upload `main_camera.cpp`.

---

### 🧪 Calibration

- **Gas Sensor:**
  The MQ-2 sensor requires a warm-up period. Adjust `GAS_THRESHOLD` in `main_rear.cpp` based on your environment's "clean air" value.

- **Ultrasonic:**
  Verify distances in the Serial Monitor. Adjust `MIN_SAFE_DIST_CM` if the robot stops too early or too late.

---

## 🧠 Logic Flow

1. **Initialization**
   Both ESPs boot. Front Node establishes ESP-NOW link with Rear Node.

2. **Scan**
   Front Node reads Ultrasonic L/R.

3. **Decision**

   - **Path Clear:**
     Send `"FORWARD"` command to Rear. Both drive at `NORMAL_SPEED`.
   - **Obstacle Close (<10 cm):**
     Trigger `CLIMB_MODE`. Front motors boost torque, Rear motors push hard.
   - **Obstacle Farther (<20 cm):**
     Stop → Reverse → Turn.

4. **Interrupt**
   If Rear Node detects `Smoke > Threshold` → sends `"HAZARD"` packet → Front Node executes **Emergency Stop**.

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
