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

static void setupWebServer()
{
    ws.onEvent(onWsEvent);
    webServer.addHandler(&ws);

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
    // Stub: If camera sends status over UART, update lastRearDistance or FPS
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
