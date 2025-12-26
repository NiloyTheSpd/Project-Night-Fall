# Project Nightfall - React Dashboard

## Overview

Real-time monitoring dashboard for Project Nightfall's three-board ESP32 system. Displays sensor data, motor status, network connectivity, and system health metrics in a modern, responsive interface.

## Features

### Three Board Dashboard

**Back ESP32 (Master)**

- Sensor readings (ultrasonic distance, gas level)
- Motor status (rear wheels - PWM%)
- Navigation state (idle, moving, scanning)
- CPU load and memory usage
- Temperature monitoring
- Battery status

**Front ESP32 (Motor Slave)**

- Real-time motor PWM values (4 motors)
- Connection status and signal strength
- Temperature and CPU load
- Battery voltage monitoring
- Memory usage tracking

**Camera ESP32 (Telemetry Bridge)**

- Camera initialization status
- Last telemetry reception time
- ESP-NOW packet monitoring
- Memory and power status
- Network connectivity

### Real-Time Monitoring

- **Live Sensor Data**: Distance sensors, gas levels with alert thresholds
- **Motor Visualization**: PWM bars showing current motor speed (0-255)
- **System Metrics**: CPU load, memory usage, temperature
- **Status Indicators**: Connection status, battery levels, signal strength
- **Event Logs**: Real-time system events and status changes

### Dark Theme UI

- High-contrast dark theme optimized for monitoring
- Color-coded status indicators (green=OK, yellow=warning, red=critical)
- Responsive grid layout
- Smooth animations and transitions
- Lucide icons for quick visual reference

## Quick Start

### Setup

```bash
# Navigate to dashboard directory
cd robot-dashboard

# Install dependencies
npm install

# Start development server
npm run dev
```

Dashboard available at `http://localhost:5173`

### Production Build

```bash
npm run build
npm run preview
```

## Architecture

### Main Components

- **DashboardEnhanced.jsx**: Main dashboard component with all three ESP32 board displays
- **StatusBadge**: Connection status indicator (connected/warning/disconnected)
- **MetricBox**: Reusable metric display widget with icon and value
- **MotorControl**: Motor PWM visualization with progress bars

### Real-Time Data Flow

```
ESP32 Devices (WiFi/Serial)
        ↓
Telemetry API (port 8888)
        ↓
React State Management
        ↓
Live Dashboard Visualization
```

## Integration

### Connect to ESP32 via WebSocket

```javascript
const ws = new WebSocket(
  import.meta.env.VITE_TELEMETRY_WS || "ws://192.168.4.1:8888"
);

ws.onmessage = (event) => {
  const data = JSON.parse(event.data);
  setEsp32Data(data);
};

ws.onerror = (error) => {
  console.error("WebSocket error:", error);
};
```

### Expected Data Format

```json
{
  "back": {
    "status": "connected",
    "battery": 85,
    "temperature": 42.5,
    "signal": 85,
    "uptime": 3600,
    "sensors": {
      "frontDist": 45.2,
      "rearDist": 38.1,
      "gasLevel": 250,
      "gasAlertLevel": false,
      "navigationState": "moving"
    },
    "motors": {
      "rearLeft": 180,
      "rearRight": 180,
      "isMoving": true
    },
    "memoryUsage": 45,
    "cpuLoad": 52
  },
  "front": {
    "status": "connected",
    "battery": 92,
    "temperature": 38.2,
    "signal": 90,
    "motors": {
      "motor1": 180,
      "motor2": 180,
      "motor3": 0,
      "motor4": 0
    },
    "cpuLoad": 28,
    "memoryUsage": 38
  },
  "camera": {
    "status": "connected",
    "battery": 88,
    "signal": 75,
    "cameraReady": true,
    "lastTelemetry": "12:45:32",
    "memoryUsage": 52
  }
}
```

## Customization

### Configure WebSocket Endpoint

Create a `.env` file in `robot-dashboard`:

```
VITE_TELEMETRY_WS=ws://192.168.4.1:8888/ws
```

Restart the dev server after changes.

### Modify Color Scheme

Update Tailwind color classes:

- Status colors: `text-blue-400`, `text-green-400`, `text-red-400`
- Background: `bg-gray-900`, `bg-gray-800`
- Accents: `text-yellow-400`, `text-purple-400`

### Add New Metrics

```jsx
<MetricBox
  label="New Metric"
  value={data.newValue}
  unit="unit"
  color="text-blue-400"
  icon={IconComponent}
/>
```

## Browser Support

- Chrome/Edge 90+
- Firefox 88+
- Safari 14+
- Mobile browsers

## Dependencies

- **React 18.2+** - UI framework
- **Tailwind CSS** - Styling
- **Lucide React** - Icons
- **Vite** - Build tool

## Troubleshooting

**Dashboard not connecting**

- Verify ESP32 is powered and broadcasting
- Check WebSocket endpoint is correct
- Ensure network connectivity between devices
- Check browser console for errors
- Close any Serial Monitor that may lock the COM port during upload

**Data not updating**

- Verify ESP32 is sending telemetry every 500ms
- Check JSON message format matches expected structure
- Inspect network tab in DevTools
- Ensure the Camera Bridge is receiving ESP-NOW and forwarding via WebSocket

**Performance issues**

- Increase polling interval (higher milliseconds)
- Reduce log entry display count
- Check CPU usage in browser DevTools

## Future Enhancements

- [ ] Real-time WebSocket integration
- [x] Dashboard WebSocket client with reconnect/backoff
- [ ] Autonomous navigation map visualization
- [ ] Historical data charts (temperature, battery trends)
- [ ] Motor control interface
- [ ] Alert notifications system
- [ ] Data export (CSV/JSON)
- [ ] Fleet monitoring (multiple robots)
- [ ] 3D pose visualization

## Version

**1.0** - December 27, 2025
ESP-NOW + WiFi Hybrid Architecture
