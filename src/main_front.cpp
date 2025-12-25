/**
 * Project Nightfall - Front Controller (ESP32 #1)
 * Exposes WiFi AP + HTTP/WS endpoints for Next.js/React dashboard.
 * Handles front motors, front ultrasonic, gas sensor, safety, and UART links.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>
#include <esp_task_wdt.h>

#include "config.h"
#include "pins.h"

#include "MotorControl.h"
#include "UltrasonicSensor.h"
#include "GasSensor.h"
#include "SafetyMonitor.h"
#include "AutonomousNav.h"
#include "UARTComm.h"

// Front controller objects
MotorControl frontMotors(
    MOTOR_FL_ENA, MOTOR_FL_IN1, MOTOR_FL_IN2,
    MOTOR_FR_ENB, MOTOR_FR_IN3, MOTOR_FR_IN4,
    PWM_CHANNEL_FL, PWM_CHANNEL_FR);

UltrasonicSensor frontSensor(US_FRONT_TRIG, US_FRONT_ECHO);
GasSensor gasSensor(GAS_ANALOG, GAS_DIGITAL);
SafetyMonitor safetyMonitor;
AutonomousNav autonomousNav;
UARTComm rearComm(Serial1, UART_BAUD_RATE);
UARTComm cameraComm(Serial2, UART_BAUD_RATE);

// State
RobotState currentState = STATE_INIT;
NavigationState navState = NAV_FORWARD;
float lastFrontDistance = 0.0f;
float lastRearDistance = 0.0f;
int gasValue = 0;
bool autonomousMode = false;
bool emergencyStopTriggered = false;
bool cameraConnected = false;
unsigned long lastCameraHeartbeat = 0;
int cameraFPS = 0;
bool rearConnected = false;
unsigned long lastRearHeartbeat = 0;

// Timing
unsigned long lastHeartbeat = 0;
unsigned long lastNavUpdate = 0;
unsigned long lastGasReading = 0;
unsigned long lastSafetyCheck = 0;
unsigned long lastTelemetryPush = 0;
unsigned long loopCounter = 0;
IPAddress apIP;

// Dashboard server
AsyncWebServer webServer(DASHBOARD_HTTP_PORT);
AsyncWebSocket ws(DASHBOARD_WS_PATH);

// Debug helpers
#ifdef SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

static void setupWatchdog()
{
    esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
    esp_task_wdt_add(NULL);
}

static void resetWatchdog()
{
    esp_task_wdt_reset();
}

static void setupWiFiAP()
{
    DEBUG_PRINTLN("  [WIFI] Starting Access Point for dashboard...");
    WiFi.mode(WIFI_AP);
    if (!WiFi.softAP(WIFI_SSID, WIFI_PASSWORD))
    {
        DEBUG_PRINTLN("  [WIFI] Failed to start AP");
        return;
    }
    apIP = WiFi.softAPIP();
    DEBUG_PRINT("  [WIFI] AP IP: ");
    DEBUG_PRINTLN(apIP);
    DEBUG_PRINT("  [WIFI] Dashboard URL: http://");
    DEBUG_PRINT(apIP);
    DEBUG_PRINT(":");
    DEBUG_PRINTLN(DASHBOARD_HTTP_PORT);
}

static void buildTelemetryPayload(JsonDocument &doc)
{
    doc["front_distance"] = lastFrontDistance;
    doc["rear_distance"] = lastRearDistance;
    doc["gas"] = gasValue;
    doc["state"] = (int)currentState;
    doc["autonomous"] = autonomousMode;
    doc["emergency"] = emergencyStopTriggered;
    doc["uptime_ms"] = millis();
    doc["loop_counter"] = loopCounter;
    doc["wifi_ip"] = apIP.toString();
    String streamUrl = String("http://") + apIP.toString() + ":" + CAMERA_STREAM_PORT + "/stream";
    doc["stream_url"] = streamUrl;
    doc["camera_connected"] = cameraConnected && (millis() - lastCameraHeartbeat < 3000);
    doc["camera_fps"] = cameraFPS;
    doc["rear_connected"] = rearConnected && (millis() - lastRearHeartbeat < 3000);
}

static void handleWebCommand(const char *cmd)
{
    if (!cmd || !*cmd)
        return;
    DEBUG_PRINT("[CMD] Dashboard command: ");
    DEBUG_PRINTLN(cmd);

    if (strcmp(cmd, "autonomous_on") == 0)
    {
        autonomousMode = true;
        currentState = STATE_AUTONOMOUS;
        return;
    }
    if (strcmp(cmd, "autonomous_off") == 0)
    {
        autonomousMode = false;
        frontMotors.stop();
        currentState = STATE_IDLE;
        return;
    }

    // Manual actions disable autonomous
    autonomousMode = false;
    if (strcmp(cmd, "forward") == 0)
    {
        currentState = STATE_MANUAL;
        frontMotors.forward(MOTOR_NORMAL_SPEED);
    }
    else if (strcmp(cmd, "back") == 0 || strcmp(cmd, "backward") == 0)
    {
        currentState = STATE_MANUAL;
        frontMotors.backward(MOTOR_NORMAL_SPEED);
    }
    else if (strcmp(cmd, "left") == 0)
    {
        currentState = STATE_TURNING;
        frontMotors.turnLeft(MOTOR_TURN_SPEED);
    }
    else if (strcmp(cmd, "right") == 0)
    {
        currentState = STATE_TURNING;
        frontMotors.turnRight(MOTOR_TURN_SPEED);
    }
    else if (strcmp(cmd, "rotate_360") == 0)
    {
        currentState = STATE_TURNING;
        frontMotors.rotate360(true);
    }
    else if (strcmp(cmd, "stop") == 0)
    {
        currentState = STATE_IDLE;
        frontMotors.stop();
    }
    else if (strcmp(cmd, "estop") == 0 || strcmp(cmd, "emergency_stop") == 0)
    {
        emergencyStopTriggered = true;
        safetyMonitor.triggerEmergencyStop();
        frontMotors.stop();
        currentState = STATE_EMERGENCY;
    }
}

static void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT)
    {
        DEBUG_PRINT("[WS] Client connected, id=");
        DEBUG_PRINTLN(client->id());
        return;
    }
    if (type == WS_EVT_DISCONNECT)
    {
        DEBUG_PRINT("[WS] Client disconnected, id=");
        DEBUG_PRINTLN(client->id());
        return;
    }
    if (type == WS_EVT_DATA)
    {
        AwsFrameInfo *info = (AwsFrameInfo *)arg;
        if (!(info->final && info->index == 0 && info->len == len))
            return;
        if (info->opcode == WS_TEXT)
        {
            String msg;
            msg.reserve(len + 1);
            for (size_t i = 0; i < len; i++)
                msg += (char)data[i];
            StaticJsonDocument<256> doc;
            if (deserializeJson(doc, msg))
                return;
            const char *cmd = doc["cmd"] | "";
            handleWebCommand(cmd);
        }
    }
}

static const char DASHBOARD_HTML[] PROGMEM = R"rawliteral(
<!DOCTYPE html>
<html>
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>Project Nightfall - Dashboard</title>
    <style>
        * { margin: 0; padding: 0; box-sizing: border-box; }
        body { font-family: Arial, sans-serif; background: #1a1a1a; color: #fff; padding: 20px; }
        .container { max-width: 900px; margin: 0 auto; }
        h1 { text-align: center; margin-bottom: 20px; color: #00ff00; }
        .status-grid { display: grid; grid-template-columns: 1fr 1fr; gap: 15px; margin-bottom: 20px; }
        .status-box { background: #2a2a2a; padding: 15px; border: 2px solid #00ff00; border-radius: 5px; }
        .status-label { font-size: 12px; color: #aaa; text-transform: uppercase; }
        .status-value { font-size: 24px; font-weight: bold; color: #00ff00; margin-top: 5px; }
        .controls { display: grid; grid-template-columns: repeat(3, 1fr); gap: 10px; margin-bottom: 20px; }
        button { padding: 15px; font-size: 16px; border: none; border-radius: 5px; cursor: pointer; font-weight: bold; transition: 0.2s; }
        button { background: #004400; color: #00ff00; border: 2px solid #00ff00; }
        button:hover { background: #00ff00; color: #000; }
        button:active { transform: scale(0.95); }
        .emergency { grid-column: 1 / -1; background: #660000; border-color: #ff0000; color: #ff0000; }
        .emergency:hover { background: #ff0000; color: #fff; }
        .auto-toggle { grid-column: 1 / -1; background: #000066; border-color: #0099ff; color: #0099ff; }
        .auto-toggle:hover { background: #0099ff; color: #000; }
        .auto-toggle.active { background: #0099ff; color: #000; }
        .video { text-align: center; margin: 20px 0; }
        img { max-width: 100%; border: 2px solid #00ff00; border-radius: 5px; }
    </style>
</head>
<body>
    <div class="container">
        <h1>🤖 PROJECT NIGHTFALL</h1>
        
        <div class="status-grid">
            <div class="status-box">
                <div class="status-label">Front Distance</div>
                <div class="status-value" id="frontDist">-- cm</div>
            </div>
            <div class="status-box">
                <div class="status-label">Gas Level</div>
                <div class="status-value" id="gasLevel">-- ppm</div>
            </div>
            <div class="status-box">
                <div class="status-label">State</div>
                <div class="status-value" id="state">INIT</div>
            </div>
            <div class="status-box">
                <div class="status-label">Uptime</div>
                <div class="status-value" id="uptime">0s</div>
            </div>
        </div>

        <div class="controls">
            <button onclick="cmd('left')">⬅ LEFT</button>
            <button onclick="cmd('forward')">⬆ FORWARD</button>
            <button onclick="cmd('right')">➡ RIGHT</button>
            
            <button onclick="cmd('stop')" style="grid-column: 1 / -1;">⏹ STOP</button>
            
            <button onclick="cmd('backward')">⬇ BACKWARD</button>
            <button onclick="cmd('rotate_360')">🔄 ROTATE</button>
            <button onclick="cmd('left')" style="grid-column: 1 / -1;">Back</button>
            
            <button class="auto-toggle" id="autoBtn" onclick="toggleAuto()">🔴 AUTONOMOUS: OFF</button>
            <button class="emergency" onclick="cmd('estop')">🛑 EMERGENCY STOP</button>
        </div>

        <div class="video">
            <h3 style="margin-bottom: 10px; color: #00ff00;">📷 Camera Feed</h3>
            <img src="http://192.168.4.1:81/stream" alt="Camera Stream" style="width: 100%; max-width: 500px;">
        </div>
    </div>

    <script>
        let ws = null;
        let autoMode = false;

        function initWS() {
            ws = new WebSocket('ws://192.168.4.1/ws');
            ws.onmessage = (e) => {
                try {
                    const data = JSON.parse(e.data);
                    updateUI(data);
                } catch (err) {
                    console.error('Parse error:', err);
                }
            };
            ws.onerror = () => { setTimeout(initWS, 2000); };
            ws.onclose = () => { setTimeout(initWS, 2000); };
        }

        function updateUI(data) {
            document.getElementById('frontDist').textContent = data.front_distance ? data.front_distance.toFixed(1) + ' cm' : '-- cm';
            document.getElementById('gasLevel').textContent = data.gas || '0';
            const states = ['INIT', 'IDLE', 'AUTO', 'MANUAL', 'EMG', 'CLIMB', 'TURN', 'AVOID'];
            document.getElementById('state').textContent = states[data.state] || 'UNKNOWN';
            document.getElementById('uptime').textContent = (data.uptime_ms / 1000).toFixed(0) + 's';
            
            autoMode = data.autonomous;
            const btn = document.getElementById('autoBtn');
            if (autoMode) {
                btn.classList.add('active');
                btn.textContent = '🟢 AUTONOMOUS: ON';
            } else {
                btn.classList.remove('active');
                btn.textContent = '🔴 AUTONOMOUS: OFF';
            }
        }

        function cmd(action) {
            fetch('http://192.168.4.1/api/command', {
                method: 'POST',
                headers: { 'Content-Type': 'application/json' },
                body: JSON.stringify({ cmd: action })
            }).catch(e => console.error(e));
        }

        function toggleAuto() {
            cmd(autoMode ? 'autonomous_off' : 'autonomous_on');
        }

        initWS();
    </script>
</body>
</html>
)rawliteral";

static void setupWebServer()
{
    // Enable CORS for React dashboard
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, OPTIONS");
    DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);

    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
                 { request->send(200, "text/html", DASHBOARD_HTML); });

    webServer.on("/api/telemetry", HTTP_GET, [](AsyncWebServerRequest *request)
                 {
		StaticJsonDocument<512> doc;
		buildTelemetryPayload(doc);
		String out; serializeJson(doc, out);
		request->send(200, "application/json", out); });

    webServer.on("/api/command", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
                 {
								 if (index != 0) return;
								 StaticJsonDocument<256> doc;
								 if (deserializeJson(doc, data, len))
								 {
									 request->send(400, "application/json", "{\"error\":\"invalid_json\"}");
									 return;
								 }
								 const char *cmd = doc["cmd"] | "";
								 handleWebCommand(cmd);
								 request->send(200, "application/json", "{\"status\":\"ok\"}"); });

    webServer.begin();
    DEBUG_PRINTLN("  [HTTP] Dashboard server started");
}

static void broadcastTelemetry()
{
    unsigned long now = millis();
    if (now - lastTelemetryPush < DASHBOARD_UPDATE_INTERVAL)
        return;
    lastTelemetryPush = now;
    StaticJsonDocument<512> doc;
    buildTelemetryPayload(doc);
    String out;
    serializeJson(doc, out);
    ws.textAll(out);
}

static void handleFrontSensors()
{
    lastFrontDistance = frontSensor.getAverageDistance(3);
    safetyMonitor.checkCollisionRisk(lastFrontDistance, lastRearDistance);

    StaticJsonDocument<256> sensorMsg;
    sensorMsg["type"] = "sensor_update";
    sensorMsg["source"] = "front";
    sensorMsg["timestamp"] = millis();
    sensorMsg["data"]["front_distance"] = lastFrontDistance;
    sensorMsg["data"]["gas_level"] = gasValue;
    sensorMsg["data"]["robot_state"] = (int)currentState;
    rearComm.sendMessage(sensorMsg);
}

static void updateAutonomousNavigation()
{
    if (emergencyStopTriggered)
        return;
    autonomousNav.updateSensorData(lastFrontDistance, lastRearDistance);
    navState = autonomousNav.getCurrentState();

    switch (navState)
    {
    case NAV_FORWARD:
        currentState = STATE_AUTONOMOUS;
        frontMotors.forward(MOTOR_NORMAL_SPEED);
        break;
    case NAV_TURNING_LEFT:
        currentState = STATE_TURNING;
        frontMotors.turnLeft(MOTOR_TURN_SPEED);
        break;
    case NAV_TURNING_RIGHT:
        currentState = STATE_TURNING;
        frontMotors.turnRight(MOTOR_TURN_SPEED);
        break;
    case NAV_CLIMBING:
        currentState = STATE_CLIMBING;
        frontMotors.boostFront(CLIMB_BOOST_DURATION);
        break;
    case NAV_BACKING_UP:
        frontMotors.backward(MOTOR_NORMAL_SPEED);
        break;
    case NAV_STUCK:
        frontMotors.rotate360(true);
        break;
    case NAV_AVOIDING:
    case NAV_SCANNING:
        frontMotors.stop();
        break;
    }

    StaticJsonDocument<256> cmdMsg;
    cmdMsg["type"] = "motor_command";
    cmdMsg["source"] = "front";
    cmdMsg["timestamp"] = millis();
    // Send a simple mirror command string for rear sync
    switch (navState)
    {
    case NAV_FORWARD:
        cmdMsg["cmd"] = "forward";
        break;
    case NAV_TURNING_LEFT:
        cmdMsg["cmd"] = "left";
        break;
    case NAV_TURNING_RIGHT:
        cmdMsg["cmd"] = "right";
        break;
    case NAV_BACKING_UP:
        cmdMsg["cmd"] = "backward";
        break;
    case NAV_STUCK:
        cmdMsg["cmd"] = "rotate_360";
        break;
    default:
        cmdMsg["cmd"] = "stop";
        break;
    }
    rearComm.sendMessage(cmdMsg);
}

static void receiveMasterCommands()
{
    // Stub: If you plan user commands over UART, parse here
}

static void processCameraFeedback()
{
    StaticJsonDocument<512> doc = cameraComm.receiveMessage();
    if (doc.size() == 0)
        return;

    const char *type = doc["type"] | "";
    const char *source = doc["source"] | "";

    if (strcmp(source, "camera") != 0)
        return;

    if (strcmp(type, "status") == 0)
    {
        JsonVariant data = doc["data"];
        cameraFPS = data["fps"] | cameraFPS;
        cameraConnected = data["wifi_connected"] | cameraConnected;
        lastCameraHeartbeat = millis();
        return;
    }
    if (strcmp(type, "heartbeat_ack") == 0)
    {
        JsonVariant data = doc["data"];
        cameraFPS = data["fps"] | cameraFPS;
        cameraConnected = true;
        lastCameraHeartbeat = millis();
        return;
    }
}

static void processRearFeedback()
{
    // Receive rear sensor updates over UART and update cached values
    StaticJsonDocument<512> doc = rearComm.receiveMessage();
    if (doc.size() == 0)
        return;

    const char *type = doc["type"] | "";
    const char *source = doc["source"] | "";
    if (strcmp(type, "sensor_update") == 0 && strcmp(source, "rear") == 0)
    {
        // Expected format: { type: "sensor_update", source: "rear", data: { rear_distance: <float>, robot_state: <int> } }
        JsonVariant data = doc["data"];
        if (!data.isNull())
        {
            lastRearDistance = data["rear_distance"] | lastRearDistance;
            rearConnected = true;
            lastRearHeartbeat = millis();
        }
    }
}

static void sendHeartbeatToRear()
{
    StaticJsonDocument<128> hb;
    hb["type"] = "heartbeat";
    hb["source"] = "front";
    hb["ts"] = millis();
    rearComm.sendMessage(hb);
}

static void sendHeartbeatToCamera()
{
    StaticJsonDocument<128> hb;
    hb["type"] = "heartbeat";
    hb["source"] = "front";
    hb["ts"] = millis();
    cameraComm.sendMessage(hb);
}

static void handleEmergencyStop()
{
    safetyMonitor.triggerEmergencyStop();
    frontMotors.stop();
    currentState = STATE_EMERGENCY;
}

void setup()
{
    Serial.begin(115200);
    delay(800);

    setupWatchdog();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    DEBUG_PRINTLN("\nPROJECT NIGHTFALL - FRONT CONTROLLER STARTUP\n");

    frontMotors.begin();
    frontSensor.begin();
    gasSensor.begin();
    safetyMonitor.begin();
    autonomousNav.begin();
    // Initialize hardware UARTs for rear and camera links
    Serial1.begin(UART_BAUD_RATE, SERIAL_8N1, UART_REAR_RX, UART_REAR_TX);
    Serial2.begin(UART_BAUD_RATE, SERIAL_8N1, UART_CAM_RX, UART_CAM_TX);
    rearComm.begin();
    cameraComm.begin();

    currentState = STATE_IDLE;
    autonomousMode = false;
    emergencyStopTriggered = false;

    setupWiFiAP();
    setupWebServer();

    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
    resetWatchdog();
    unsigned long currentTime = millis();

    // Sensors @ 10Hz
    if (currentTime - lastNavUpdate >= 100)
    {
        lastNavUpdate = currentTime;
        handleFrontSensors();
    }

    // Safety @ 5Hz
    if (currentTime - lastSafetyCheck >= 200)
    {
        lastSafetyCheck = currentTime;
        safetyMonitor.update();
        if (!safetyMonitor.isSafe() || safetyMonitor.isEmergency())
        {
            if (!emergencyStopTriggered)
            {
                emergencyStopTriggered = true;
                handleEmergencyStop();
            }
        }
        else if (emergencyStopTriggered && safetyMonitor.isSafe())
        {
            emergencyStopTriggered = false;
            safetyMonitor.resetEmergencyStop();
            currentState = STATE_IDLE;
        }
    }

    // Autonomous nav @ 2Hz
    if (autonomousMode && currentState == STATE_AUTONOMOUS)
    {
        if (currentTime - lastNavUpdate >= 500)
        {
            updateAutonomousNavigation();
            lastNavUpdate = currentTime;
        }
    }

    // Gas sensor
    if (currentTime - lastGasReading >= GAS_SAMPLE_INTERVAL)
    {
        lastGasReading = currentTime;
        gasSensor.update();
        gasValue = gasSensor.getAnalogValue();
        if (gasSensor.isDetected())
        {
            safetyMonitor.raiseAlert(ALERT_GAS_DETECTED, ALERT_CRITICAL, "Hazardous gas detected");
        }
    }

    // Comm
    receiveMasterCommands();
    processRearFeedback();
    processCameraFeedback();

    // Heartbeat @ 1Hz
    if (currentTime - lastHeartbeat >= 1000)
    {
        lastHeartbeat = currentTime;
        sendHeartbeatToRear();
        sendHeartbeatToCamera();
    }

    // Dashboard
    ws.cleanupClients();
    broadcastTelemetry();

    loopCounter++;
}
