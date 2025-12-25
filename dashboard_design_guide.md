# One-Page Dashboard Design Guide
## Essential Elements for Robot Control & Monitoring

---

## 🎯 Dashboard Priority Levels

### 🔴 CRITICAL (Must Have - Always Visible)
1. **Live Camera Feed** - Your eyes on the robot
2. **Emergency Stop Button** - Safety first
3. **Connection Status** - Know if you're in control
4. **Battery Level** - Don't get stranded
5. **Movement Controls** - Basic navigation

### 🟡 HIGH PRIORITY (Should Have - Primary View)
6. **Distance Sensors** - Collision avoidance
7. **Robot State** (Idle/Autonomous/Emergency)
8. **Gas/Hazard Sensors** - Mission-critical for rescue
9. **Current Direction/Speed** - Know what it's doing
10. **Mode Selector** (Manual/Autonomous)

### 🟢 MEDIUM PRIORITY (Nice to Have - Secondary View)
11. **ML Object Detection** - What it sees
12. **Mission/Waypoint Progress** - Where it's going
13. **System Alerts** - Recent warnings/errors
14. **Mini Map** - Position awareness
15. **Motor Status** - Left/right speed

### 🔵 LOW PRIORITY (Advanced - Collapsible/Toggleable)
16. **Telemetry Graphs** - Historical data
17. **Performance Stats** - FPS, inference time
18. **Temperature** - System health
19. **Uptime** - Session info
20. **Network Stats** - Connection quality

---

## 📐 Recommended Layout (Desktop - 1920x1080)

```
┌─────────────────────────────────────────────────────────────┐
│ TOP BAR (60px) - Critical Status                            │
│ [Logo] [Connection] [State] [Battery] [Uptime] [E-STOP]     │
└─────────────────────────────────────────────────────────────┘
┌────────────────┬──────────────────┬──────────────────────────┐
│  LEFT (40%)    │   CENTER (35%)   │      RIGHT (25%)         │
│                │                  │                          │
│  📹 CAMERA     │  📊 SENSORS      │  🎯 MISSION              │
│  (Large)       │                  │                          │
│  640x480       │  - Distance F/R  │  - Status                │
│                │  - Gas Level     │  - Progress Bar          │
│  [ML Overlay]  │  - Temperature   │  - Waypoints             │
│                │                  │                          │
│  ─────────────  │  📈 TELEMETRY   │  🗺️ MAP                 │
│                │  - Battery Graph │  (Mini)                  │
│  🎮 CONTROLS   │  - Distance      │                          │
│  Arrow Pad     │  - Gas Level     │  ⚠️ ALERTS              │
│  [Mode Toggle] │                  │  - Recent 5              │
│  [Speed]       │  🤖 ML DETECT    │                          │
│  [Motors L/R]  │  - Type: Person  │  ⚡ PERFORMANCE         │
│                │  - Confidence    │  - FPS, Inference        │
└────────────────┴──────────────────┴──────────────────────────┘
```

---

## 📱 Mobile Layout (Portrait)

```
┌──────────────────────┐
│ TOP BAR              │
│ Battery | Connection │
├──────────────────────┤
│   📹 CAMERA          │
│   (Full Width)       │
│                      │
├──────────────────────┤
│ 🎮 CONTROLS          │
│ [Arrow Pad] [E-STOP] │
├──────────────────────┤
│ 📊 SENSORS (Compact) │
│ Dist | Gas | Battery │
├──────────────────────┤
│ ⚠️ ALERTS (Latest)   │
└──────────────────────┘
[Tap for more info ▼]
```

---

## 🎨 Essential Components Breakdown

### 1. **Live Camera Feed** 🔴
**Why:** Most critical for situational awareness
**Size:** 40-50% of screen
**Features:**
- Live MJPEG stream from ESP32-CAM
- ML detection overlay (bounding boxes)
- Crosshair/grid overlay
- Recording indicator
- FPS counter
- Timestamp

**Code Priority:**
```jsx
<video src="http://192.168.4.1:81/stream" />
<div className="overlay">
  {mlDetection && <BoundingBox />}
  <Crosshair />
  <RecordingIndicator />
</div>
```

---

### 2. **Emergency Stop Button** 🔴
**Why:** Safety - must stop immediately
**Size:** Large, always visible
**Features:**
- Bright red color
- Large clickable area
- Keyboard shortcut (ESC or SPACE)
- Confirmation optional (too slow in emergency)

**Design:**
```jsx
<button 
  className="bg-red-600 text-white p-4 fixed top-4 right-4"
  onClick={emergencyStop}
>
  <Power /> E-STOP
</button>
```

---

### 3. **Connection Status** 🔴
**Why:** Must know if robot is responding
**Features:**
- Green dot = Connected
- Red dot = Disconnected
- Yellow dot = Weak signal
- Last heartbeat timestamp
- Auto-reconnect indicator

**Display:**
```
🟢 Connected | 192.168.4.1 | 23ms ping
```

---

### 4. **Battery Level** 🔴
**Why:** Critical for mission planning
**Features:**
- Voltage display (14.2V)
- Percentage (87%)
- Color coding (green/yellow/red)
- Time remaining estimate
- Low battery alert

**Visualization:**
```jsx
<div className={getBatteryColor()}>
  <Battery /> 14.2V (87%)
  <ProgressBar value={87} />
  <span>~45 min remaining</span>
</div>
```

---

### 5. **Movement Controls** 🔴
**Why:** Core robot operation
**Layout:**
```
        ▲
    ◀   ■   ▶
        ▼     🔄
```

**Features:**
- Arrow keys
- Stop button (center)
- 360° rotate
- Disable when in autonomous mode
- Visual feedback on press
- Keyboard support (WASD)

---

### 6. **Distance Sensors** 🟡
**Why:** Collision avoidance awareness
**Display:**
```
Front: ████████░░░ 45 cm
Rear:  ██████████░ 67 cm

Safe zone marker at 20cm
```

**Color Coding:**
- Green: > 50cm (safe)
- Yellow: 20-50cm (caution)
- Red: < 20cm (danger)

---

### 7. **Robot State** 🟡
**Why:** Know what the robot is doing
**States:**
- 🟢 IDLE - Waiting for command
- 🔵 MANUAL - User control
- 🟣 AUTONOMOUS - Self-navigating
- 🟡 AVOIDING - Obstacle maneuver
- 🔴 EMERGENCY - Stopped

**Display:**
```jsx
<div className="state-indicator">
  <Activity /> AUTONOMOUS
</div>
```

---

### 8. **Gas/Hazard Sensors** 🟡
**Why:** Mission-critical for rescue operations
**Display:**
```
💨 Gas Level: 150 (NORMAL)
🔥 Temperature: 35°C (OK)
```

**Alerts:**
- > 400: 🚨 CRITICAL - Red flash
- 300-400: ⚠️ ELEVATED - Yellow
- < 300: ✅ NORMAL - Green

---

### 9. **Mode Selector** 🟡
**Why:** Switch between manual and autonomous
**Design:**
```jsx
<button onClick={toggleMode}>
  {mode === 'autonomous' 
    ? '🤖 Autonomous Mode' 
    : '🎮 Manual Mode'}
</button>
```

**Behavior:**
- Autonomous: Disable manual controls
- Manual: Enable arrow buttons
- Smooth transition
- Confirmation for safety-critical changes

---

### 10. **ML Object Detection** 🟢
**Why:** Know what obstacles are detected
**Display:**
```
🤖 ML Detection
Detected: Person
Confidence: 87%
Distance: 2.5m
Inference: 180ms
```

**Overlay on Camera:**
- Bounding box around detected object
- Label with confidence
- Distance marker

---

### 11. **Mission Progress** 🟢
**Why:** Track autonomous mission status
**Display:**
```
🎯 Mission: Active
Waypoint 3 of 8 (37%)
Progress: ████░░░░░░
Distance: 245m remaining
ETA: 4m 32s
```

---

### 12. **System Alerts** 🟢
**Why:** Be aware of warnings/errors
**Display:**
```
⚠️ Alerts (2)
━━━━━━━━━━━━━━━━━━━
⚠️ Gas level elevated     10:24:32
ℹ️ Mission started        10:24:15
✅ Waypoint 2 reached     10:23:45
```

**Priority:**
- Critical (red) - Top
- Warning (yellow) - Middle
- Info (blue) - Bottom

---

### 13. **Mini Map** 🟢
**Why:** Spatial awareness
**Size:** 200x200px
**Features:**
- Grid overlay
- Robot position (red dot)
- Waypoints (blue dots)
- Completed waypoints (green)
- Path trail
- Obstacles (if mapped)

---

### 14. **Motor Status** 🟢
**Why:** Understand robot movement
**Display:**
```
Motors
Left:  180 ████████░░
Right: 180 ████████░░
```

**Use Cases:**
- Debug uneven movement
- Monitor motor health
- Detect stuck wheels

---

### 15. **Telemetry Graphs** 🔵
**Why:** Historical trends
**Graphs:**
```
Battery (Last 60s)
▂▃▄▅▆▇█▇▆▅▄▃▂

Distance (Last 60s)
█▇▆▅▄▃▂▃▄▅▆▇█

Gas Level (Last 60s)
▂▂▂▃▄▅▆▅▄▃▂▂▂
```

**Toggle:** Click to expand/collapse

---

### 16. **Performance Stats** 🔵
**Why:** System health monitoring
**Display:**
```
⚡ Performance
Camera: 12 FPS
ML: 180ms inference
Memory: 245KB / 520KB
CPU: 67%
Network: 23ms latency
```

---

## 🎯 Information Density Guide

### Desktop (1920x1080)
- **Camera:** 800x600px (40% width)
- **Controls:** 600x400px
- **Sensors:** 4-6 cards
- **Graphs:** 3-4 mini charts

### Tablet (1024x768)
- **Camera:** 640x480px (60% width)
- **Controls:** Below camera
- **Sensors:** 2-3 cards visible
- **Collapsible:** Graphs, stats

### Mobile (375x667)
- **Camera:** Full width, 16:9
- **Controls:** Full width, compact
- **Sensors:** Horizontal scroll
- **Everything else:** Accordion/tabs

---

## 🎨 Color Coding Standard

### Status Colors
```css
--success: #10b981  /* Green - Good */
--warning: #f59e0b  /* Yellow - Caution */
--danger: #ef4444   /* Red - Critical */
--info: #3b82f6     /* Blue - Information */
--neutral: #6b7280  /* Gray - Inactive */
```

### Sensor Ranges
```
Distance:
  > 50cm:  Green
  20-50cm: Yellow
  < 20cm:  Red

Battery:
  > 13.5V: Green
  12.5-13.5V: Yellow
  < 12.5V: Red

Gas:
  < 300:   Green
  300-400: Yellow
  > 400:   Red (Flash)
```

---

## ⌨️ Keyboard Shortcuts

Essential shortcuts for power users:

```
Movement:
  W / ↑  : Forward
  S / ↓  : Backward
  A / ←  : Turn Left
  D / →  : Turn Right
  Space  : Stop
  R      : Rotate 360°

Control:
  M      : Toggle Mode (Manual/Auto)
  ESC    : Emergency Stop
  P      : Pause Mission
  Tab    : Cycle focused element

View:
  1-9    : Switch camera views
  F      : Fullscreen camera
  G      : Toggle graphs
  H      : Hide/show UI elements
```

---

## 📊 Data Update Frequencies

Optimize for performance:

```javascript
Camera Stream:    10-15 FPS (continuous)
Sensor Data:      10 Hz (every 100ms)
ML Detection:     5 Hz (every 200ms)
Battery:          1 Hz (every 1000ms)
Mission Status:   2 Hz (every 500ms)
Alerts:           Event-driven (immediate)
Telemetry Graphs: 1 Hz (every 1000ms)
Performance:      0.5 Hz (every 2000ms)
```

---

## 🎭 Responsive Breakpoints

```css
/* Mobile */
@media (max-width: 768px) {
  - Stack vertically
  - Full-width camera
  - Collapsible sensors
  - Minimal graphs
}

/* Tablet */
@media (min-width: 769px) and (max-width: 1024px) {
  - 2-column layout
  - Medium camera
  - Visible sensors
  - Some graphs
}

/* Desktop */
@media (min-width: 1025px) {
  - 3-column layout
  - Large camera
  - All sensors visible
  - All graphs visible
}

/* Large Desktop */
@media (min-width: 1920px) {
  - 4-column layout
  - Extra-large camera
  - Multiple views
  - Extended telemetry
}
```

---

## 🚀 Loading Priority

Load critical elements first:

1. **Immediate (< 100ms):**
   - Dashboard skeleton
   - Emergency stop button
   - Connection status

2. **Fast (< 500ms):**
   - Camera stream
   - Movement controls
   - Battery level

3. **Normal (< 1s):**
   - Sensor readings
   - Robot state
   - Alerts

4. **Lazy (> 1s):**
   - Telemetry graphs
   - Performance stats
   - Historical data

---

## 💡 Pro Tips

1. **Use WebSocket** for real-time data (not polling)
2. **Cache sensor data** (don't request every frame)
3. **Debounce** movement commands (avoid spam)
4. **Progressive enhancement** (work without JS)
5. **Graceful degradation** (handle connection loss)
6. **Accessible** (keyboard navigation, screen readers)
7. **Dark mode default** (easier on eyes, saves power)
8. **Haptic feedback** (mobile vibration on actions)
9. **Sound alerts** (optional beeps for warnings)
10. **Save preferences** (layout, thresholds, etc.)

---

## 🎯 Dashboard Checklist

Before deployment, ensure:

- [ ] Emergency stop works from anywhere
- [ ] Camera stream loads reliably
- [ ] All controls respond < 100ms
- [ ] Battery warning at 20%
- [ ] Connection loss handled gracefully
- [ ] Keyboard shortcuts work
- [ ] Mobile responsive
- [ ] Color-blind friendly
- [ ] High contrast mode
- [ ] Touch-friendly (44px minimum)
- [ ] Works offline (cached)
- [ ] Auto-reconnect on disconnect
- [ ] Confirmation for destructive actions
- [ ] Loading states for all async
- [ ] Error messages are clear

---

**🎉 Your dashboard is now a mission control center!**