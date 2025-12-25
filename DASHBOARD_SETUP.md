# React Dashboard Setup

## Quick Start

### 1. Install Dependencies

```bash
npm create vite@latest robot-dashboard -- --template react
cd robot-dashboard
npm install lucide-react
```

### 2. Replace App.jsx

Copy the contents of `dashboard.jsx` to `robot-dashboard/src/App.jsx`

### 3. Update main.jsx

```jsx
import React from "react";
import ReactDOM from "react-dom/client";
import App from "./App.jsx";
import "./index.css";

ReactDOM.createRoot(document.getElementById("root")).render(
  <React.StrictMode>
    <App />
  </React.StrictMode>
);
```

### 4. Update index.css

Add Tailwind CSS or keep the existing styles.

### 5. Connect to Robot WiFi

- Connect your PC to WiFi: **ProjectNightfall** (password: `rescue2025`)
- The dashboard will auto-connect to `192.168.4.1`

### 6. Run Dashboard

```bash
npm run dev
```

Open browser to `http://localhost:5173`

---

## Features Connected

✅ **Live Camera Feed**: ESP32-CAM stream at `:81/stream`
✅ **Real-time Telemetry**: WebSocket at `/ws`
✅ **Motor Control**: Sends commands via `/api/command`
✅ **Sensor Data**: Front/Rear distance, Gas sensor
✅ **Autonomous Mode Toggle**: Switch between manual/auto
✅ **Emergency Stop**: Immediate halt

---

## Troubleshooting

**Camera not showing?**

- Ensure ESP32-CAM is powered and streaming
- Check `http://192.168.4.1:81/stream` directly

**WebSocket not connecting?**

- Verify front ESP32 uploaded with CORS enabled
- Check browser console for errors

**Commands not working?**

- Re-upload front ESP32 firmware
- Test API: `curl -X POST http://192.168.4.1/api/command -d '{"cmd":"stop"}'`
