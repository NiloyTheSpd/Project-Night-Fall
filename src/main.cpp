/**
 * Project Nightfall - Main Control Module
 * Dual ESP32 System: Front Controller (Master) + Rear Controller (Slave)
 *
 * Front Controller (ESP32 #1):
 * - Front wheel motor control (L298N driver)
 * - Front ultrasonic obstacle detection
 * - Gas/smoke sensor monitoring
 * - UART communication with rear controller and camera
 * - Autonomous navigation logic
 * - Master coordination and heartbeat
 *
 * Rear Controller (ESP32 #2):
 * - Rear wheel motor control (L298N driver)
 * - Rear ultrasonic sensor
 * - UART communication with front controller
 * - Safety monitoring
 * - Motor command reception and execution
 *
 * Compile with: -D FRONT_CONTROLLER or -D REAR_CONTROLLER
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_system.h>
#include <esp_task_wdt.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <AsyncJson.h>

// ============================================
// CONFIGURATION & PINS
// ============================================
#define SERIAL_DEBUG 1     // Enable serial debugging
#define FRONT_CONTROLLER 1 // Define board type (comment for REAR_CONTROLLER)
// #define REAR_CONTROLLER 1     // Define for rear board (uncomment and comment above)

#include "config.h"
#include "pins.h"

// ============================================
// LIBRARY INCLUDES
// ============================================
#include "MotorControl.h"
#include "UltrasonicSensor.h"
#include "GasSensor.h"
#include "SafetyMonitor.h"
#include "AutonomousNav.h"
#include "UARTComm.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

#ifdef FRONT_CONTROLLER
// Front Controller Instances
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

// Front controller state variables
RobotState currentState = STATE_INIT;
NavigationState navState = NAV_FORWARD;
bool isMaster = true;
float lastFrontDistance = 0.0f;
float lastRearDistance = 0.0f;
unsigned long lastRearDistanceTime = 0; // Phase 3.1: Track rear distance timestamp
int gasValue = 0;
int lastGasValue = 0; // Track previous value for trending
unsigned long lastHeartbeat = 0;
unsigned long lastNavUpdate = 0;
unsigned long lastGasReading = 0;
unsigned long lastSafetyCheck = 0;
unsigned long lastBuzzerAlert = 0; // Track buzzer timing for dynamic feedback
bool autonomousMode = false;
bool emergencyStopTriggered = false;
unsigned long lastTelemetryPush = 0;
IPAddress apIP;
AsyncWebServer webServer(DASHBOARD_HTTP_PORT);
AsyncWebSocket ws(DASHBOARD_WS_PATH);

#endif

#ifdef REAR_CONTROLLER
// Rear Controller Instances
MotorControl rearMotors(
    MOTOR_RL_ENA, MOTOR_RL_IN1, MOTOR_RL_IN2,
    MOTOR_RR_ENB, MOTOR_RR_IN3, MOTOR_RR_IN4,
    PWM_CHANNEL_RL, PWM_CHANNEL_RR);

UltrasonicSensor rearSensor(US_REAR_TRIG, US_REAR_ECHO);
SafetyMonitor safetyMonitor;
UARTComm masterComm(Serial1, UART_BAUD_RATE);

// Rear controller state variables
RobotState currentState = STATE_INIT;
bool isMaster = false;
float lastRearDistance = 0.0f;
unsigned long lastHeartbeatReceived = 0;
unsigned long lastSafetyCheck = 0;
MovementCommand lastCommand = CMD_STOP;
bool emergencyStopTriggered = false;

#endif

// Shared timing variables
unsigned long lastLoopTime = 0;
unsigned long loopCounter = 0;

// ============================================
// FUNCTION DECLARATIONS
// ============================================

#ifdef FRONT_CONTROLLER
void initializeFrontController();
void updateFrontController();
void handleFrontSensors();
void updateAutonomousNavigation();
void sendHeartbeatToRear();
void sendHeartbeatToCamera();
void broadcastSensorDataToCamera();
void receiveMasterCommands();
void processRearFeedback();
void receiveRearFeedback(); // Phase 3.1: Receive rear distance with timestamp
void processCameraFeedback();
void handleEmergencyStop();
void logSystemStatus();
void setupWiFiAP();
void setupWebServer();
void broadcastTelemetry();
void buildTelemetryPayload(JsonDocument &doc);
void handleWebCommand(const char *cmd);
#endif

#ifdef REAR_CONTROLLER
void initializeRearController();
void updateRearController();
void handleRearSensors();
void receiveMasterCommands();
void executeMotorCommand(MovementCommand cmd);
void sendRearStatusToMaster();
void handleEmergencyStop();
void logSystemStatus();
#endif

void setupWatchdog();
void resetWatchdog();
void buzzerAlert(int frequency = 1000, int duration = 100);

// ============================================
// SETUP
// ============================================

void setup()
{
  Serial.begin(115200);
  delay(1000); // Allow serial to initialize

  DEBUG_PRINTLN("\n=====================================");
  DEBUG_PRINTLN("PROJECT NIGHTFALL - SYSTEM STARTUP");
  DEBUG_PRINTLN("=====================================\n");

  // Initialize watchdog timer
  setupWatchdog();

  // LED indicators setup
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

#ifdef FRONT_CONTROLLER
  DEBUG_PRINTLN(">>> Initializing FRONT CONTROLLER (Master Node)");
  initializeFrontController();
  DEBUG_PRINTLN(">>> Front Controller Ready\n");

  // Bring up WiFi AP + dashboard endpoints
  setupWiFiAP();
  setupWebServer();
#endif

#ifdef REAR_CONTROLLER
  DEBUG_PRINTLN(">>> Initializing REAR CONTROLLER (Slave Node)");
  initializeRearController();
  DEBUG_PRINTLN(">>> Rear Controller Ready\n");
#endif

  // Signal ready with LED
  digitalWrite(LED_BUILTIN, HIGH);
  delay(500);
  digitalWrite(LED_BUILTIN, LOW);

  lastLoopTime = millis();
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
  resetWatchdog();

#ifdef FRONT_CONTROLLER
  updateFrontController();
#endif

#ifdef REAR_CONTROLLER
  updateRearController();
#endif

#ifdef FRONT_CONTROLLER
  ws.cleanupClients();
  broadcastTelemetry();
#endif

  loopCounter++;
}

// ============================================
// FRONT CONTROLLER IMPLEMENTATION
// ============================================

#ifdef FRONT_CONTROLLER

void initializeFrontController()
{
  DEBUG_PRINTLN("  [FRONT] Initializing motor drivers...");
  frontMotors.begin();

  DEBUG_PRINTLN("  [FRONT] Initializing front ultrasonic sensor...");
  frontSensor.begin();

  DEBUG_PRINTLN("  [FRONT] Initializing gas sensor...");
  gasSensor.begin();

  DEBUG_PRINTLN("  [FRONT] Initializing safety monitor...");
  safetyMonitor.begin();

  DEBUG_PRINTLN("  [FRONT] Initializing autonomous navigation...");
  autonomousNav.begin();

  DEBUG_PRINTLN("  [FRONT] Initializing UART to Rear Controller...");
  rearComm.begin();

  DEBUG_PRINTLN("  [FRONT] Initializing UART to Camera Module...");
  cameraComm.begin();

  currentState = STATE_IDLE;
  autonomousMode = false;
  emergencyStopTriggered = false;

  DEBUG_PRINTLN("  [FRONT] All systems initialized successfully");
}

void setupWiFiAP()
{
  DEBUG_PRINTLN("  [WIFI] Starting Access Point for dashboard...");

  WiFi.mode(WIFI_AP);
  bool started = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

  if (!started)
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
    {
      return; // Only handle single-frame full messages
    }

    if (info->opcode == WS_TEXT)
    {
      String msg;
      msg.reserve(len + 1);
      for (size_t i = 0; i < len; i++)
      {
        msg += (char)data[i];
      }

      StaticJsonDocument<256> doc;
      DeserializationError err = deserializeJson(doc, msg);
      if (err)
      {
        DEBUG_PRINTLN("[WS] JSON parse error");
        return;
      }

      const char *cmd = doc["cmd"] | "";
      handleWebCommand(cmd);
    }
  }
}

void setupWebServer()
{
  ws.onEvent(onWsEvent);
  webServer.addHandler(&ws);

  webServer.on("/api/telemetry", HTTP_GET, [](AsyncWebServerRequest *request)
               {
    StaticJsonDocument<512> doc;
    buildTelemetryPayload(doc);
    String out;
    serializeJson(doc, out);
    request->send(200, "application/json", out); });

  webServer.on("/api/command", HTTP_POST, [](AsyncWebServerRequest *request) {}, NULL, [](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total)
               {
                 if (index != 0)
                 {
                   return; // Only accept single-chunk bodies
                 }

                 StaticJsonDocument<256> doc;
                 DeserializationError err = deserializeJson(doc, data, len);
                 if (err)
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

void buildTelemetryPayload(JsonDocument &doc)
{
  // Phase 3.3: Calculate distance trending for dashboard
  static float lastTelemetryDistance = 0.0f;
  float distanceTrend = lastFrontDistance - lastTelemetryDistance;
  lastTelemetryDistance = lastFrontDistance;

  doc["front_distance"] = lastFrontDistance;
  doc["rear_distance"] = lastRearDistance;
  doc["front_distance_trend"] = distanceTrend;  // Rate of distance change
  doc["approaching"] = (distanceTrend < -0.5f); // True if rapidly approaching
  doc["gas_level"] = gasValue;
  doc["gas_detected"] = (gasValue > GAS_THRESHOLD_ANALOG);
  doc["gas_trend"] = (gasValue - lastGasValue); // Rate of smoke change
  doc["smoke_emergency"] = (gasValue > 3000);   // Emergency threshold
  doc["smoke_warning"] = (gasValue > 2000);     // Warning threshold
  doc["state"] = (int)currentState;
  doc["autonomous"] = autonomousMode;
  doc["emergency"] = emergencyStopTriggered;
  doc["uptime_ms"] = millis();
  doc["loop_counter"] = loopCounter;
  doc["wifi_ip"] = apIP.toString();
  String streamUrl = String("http://") + apIP.toString() + ":" + CAMERA_STREAM_PORT + "/stream";
  doc["stream_url"] = streamUrl;
}

void broadcastTelemetry()
{
  unsigned long now = millis();
  if (now - lastTelemetryPush < DASHBOARD_UPDATE_INTERVAL)
  {
    return;
  }

  lastTelemetryPush = now;

  StaticJsonDocument<512> doc;
  buildTelemetryPayload(doc);
  String out;
  serializeJson(doc, out);
  ws.textAll(out);
}

void handleWebCommand(const char *cmd)
{
  if (cmd == nullptr || cmd[0] == '\0')
  {
    return;
  }

  DEBUG_PRINT("[CMD] Dashboard command: ");
  DEBUG_PRINTLN(cmd);

  // Manual commands disable autonomous mode
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

void updateFrontController()
{
  unsigned long currentTime = millis();

  // ========== SENSOR UPDATES (10Hz) ==========
  if (currentTime - lastNavUpdate >= 100)
  {
    lastNavUpdate = currentTime;
    handleFrontSensors();
    broadcastSensorDataToCamera();
  }

  // ========== SAFETY CHECKS (5Hz) ==========
  if (currentTime - lastSafetyCheck >= 200)
  {
    lastSafetyCheck = currentTime;
    safetyMonitor.update();

    // Check for emergency conditions
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
      // Clear emergency stop if safe conditions restored
      emergencyStopTriggered = false;
      safetyMonitor.resetEmergencyStop();
      currentState = STATE_IDLE;
      DEBUG_PRINTLN("[FRONT] Emergency stop cleared - resuming operations");
    }
  }

  // ========== NAVIGATION UPDATES (2Hz in autonomous mode) ==========
  if (autonomousMode && currentState == STATE_AUTONOMOUS)
  {
    if (currentTime - lastNavUpdate >= 500)
    {
      updateAutonomousNavigation();
      lastNavUpdate = currentTime;
    }
  }

  // ========== GAS SENSOR UPDATES ==========
  if (currentTime - lastGasReading >= GAS_SAMPLE_INTERVAL)
  {
    lastGasReading = currentTime;
    gasSensor.update();
    lastGasValue = gasValue;
    gasValue = gasSensor.getAnalogValue();

    // Dynamic buzzer feedback based on smoke level
    // Map gas value to frequency: 0-500ppm = 800-1500Hz, 500-2000ppm = 1500-3000Hz
    if (gasValue > GAS_THRESHOLD_ANALOG)
    {
      // Proportional frequency based on gas concentration
      int baseFreq = 800;
      int maxFreq = 3500;
      int frequency = baseFreq + ((gasValue / 4095.0) * (maxFreq - baseFreq));

      // Alert categorization
      if (gasValue > 3000) // EMERGENCY: Heavy smoke
      {
        // Continuous rapid buzzing (50ms beep, 50ms silence)
        buzzerAlert(frequency, 50);
        safetyMonitor.raiseAlert(ALERT_GAS_DETECTED, ALERT_CRITICAL,
                                 "CRITICAL: Heavy smoke detected!");
        DEBUG_PRINT("[FRONT] 🚨 CRITICAL GAS LEVEL: ");
        DEBUG_PRINTLN(gasValue);
      }
      else if (gasValue > 2000) // WARNING: Significant smoke
      {
        // Pulsing pattern (100ms beep, 200ms silence)
        buzzerAlert(frequency, 100);
        safetyMonitor.raiseAlert(ALERT_GAS_DETECTED, ALERT_WARNING,
                                 "Warning: Moderate smoke detected");
        DEBUG_PRINT("[FRONT] ⚠️ WARNING GAS LEVEL: ");
        DEBUG_PRINTLN(gasValue);
      }
      else // NOTICE: Light smoke
      {
        // Light beeping (70ms beep, 300ms silence)
        buzzerAlert(frequency, 70);
        DEBUG_PRINT("[FRONT] ℹ️ SMOKE DETECTED: ");
        DEBUG_PRINTLN(gasValue);
      }
    }
    else if (gasSensor.isDetected())
    {
      // Digital sensor detected but below threshold
      buzzerAlert(1200, 150);
      DEBUG_PRINTLN("[FRONT] ℹ️ Smoke sensor triggered (low level)");
    }
  }

  // ========== COMMUNICATION ==========
  receiveMasterCommands();
  processCameraFeedback();
  receiveRearFeedback(); // Phase 3.1: Receive rear distance with timestamp

  // ========== HEARTBEAT (1Hz) ==========
  if (currentTime - lastHeartbeat >= 1000)
  {
    lastHeartbeat = currentTime;
    sendHeartbeatToRear();
    sendHeartbeatToCamera();
  }

  // ========== PERIODIC STATUS LOGGING ==========
  if (loopCounter % 5000 == 0)
  {
    logSystemStatus();
  }
}

void handleFrontSensors()
{
  // Update front ultrasonic distance using EMA filtering
  lastFrontDistance = frontSensor.getSmoothedDistance();

  // Safety monitoring - feed distance to safety monitor
  // (assumes rear distance available from previous slave update)
  safetyMonitor.checkCollisionRisk(lastFrontDistance, lastRearDistance);

  DEBUG_PRINT("[FRONT] Front Distance: ");
  DEBUG_PRINT(lastFrontDistance);
  DEBUG_PRINTLN(" cm");

  // Send sensor data to rear controller for coordination
  StaticJsonDocument<256> sensorMsg;
  sensorMsg["type"] = "sensor_update";
  sensorMsg["source"] = "front";
  sensorMsg["timestamp"] = millis();
  sensorMsg["data"]["front_distance"] = lastFrontDistance;
  sensorMsg["data"]["gas_level"] = gasValue;
  sensorMsg["data"]["robot_state"] = (int)currentState;

  rearComm.sendMessage(sensorMsg);
}

void updateAutonomousNavigation()
{
  if (emergencyStopTriggered)
  {
    return;
  }

  // Update navigation with current sensor data
  autonomousNav.updateSensorData(lastFrontDistance, lastRearDistance);

  // Get next movement command
  MovementCommand nextCmd = autonomousNav.getNextMove();
  navState = autonomousNav.getCurrentState();

  DEBUG_PRINT("[FRONT] Navigation State: ");
  switch (navState)
  {
  case NAV_FORWARD:
    DEBUG_PRINTLN("FORWARD");
    currentState = STATE_AUTONOMOUS;
    frontMotors.forward(MOTOR_NORMAL_SPEED);
    break;
  case NAV_AVOIDING:
    DEBUG_PRINTLN("AVOIDING");
    currentState = STATE_AVOIDING;
    break;
  case NAV_TURNING_LEFT:
    DEBUG_PRINTLN("TURNING LEFT");
    currentState = STATE_TURNING;
    frontMotors.turnLeft(MOTOR_TURN_SPEED);
    break;
  case NAV_TURNING_RIGHT:
    DEBUG_PRINTLN("TURNING RIGHT");
    currentState = STATE_TURNING;
    frontMotors.turnRight(MOTOR_TURN_SPEED);
    break;
  case NAV_CLIMBING:
    DEBUG_PRINTLN("CLIMBING");
    currentState = STATE_CLIMBING;
    frontMotors.boostFront(CLIMB_BOOST_DURATION);
    break;
  case NAV_BACKING_UP:
    DEBUG_PRINTLN("BACKING UP");
    frontMotors.backward(MOTOR_NORMAL_SPEED);
    break;
  case NAV_STUCK:
    DEBUG_PRINTLN("STUCK - ROTATING");
    frontMotors.rotate360(true);
    break;
  case NAV_SCANNING:
    DEBUG_PRINTLN("SCANNING");
    frontMotors.stop();
    break;
  }

  // Send command to rear controller for synchronization
  StaticJsonDocument<256> cmdMsg;
  cmdMsg["type"] = "motor_command";
  cmdMsg["source"] = "front";
  cmdMsg["timestamp"] = millis();
  cmdMsg["data"]["command"] = (int)nextCmd;
  cmdMsg["data"]["nav_state"] = (int)navState;

  rearComm.sendMessage(cmdMsg);
}

void sendHeartbeatToRear()
{
  StaticJsonDocument<300> heartbeat;
  heartbeat["type"] = "heartbeat";
  heartbeat["source"] = "front";
  heartbeat["timestamp"] = millis();
  heartbeat["data"]["uptime"] = millis() / 1000;
  heartbeat["data"]["loop_count"] = loopCounter;
  heartbeat["data"]["state"] = (int)currentState;
  heartbeat["data"]["safe"] = safetyMonitor.isSafe();
  heartbeat["data"]["emergency"] = emergencyStopTriggered;
  heartbeat["data"]["autonomous"] = autonomousMode;

  rearComm.sendMessage(heartbeat);
}

void sendHeartbeatToCamera()
{
  StaticJsonDocument<512> heartbeat;
  heartbeat["type"] = "heartbeat";
  heartbeat["source"] = "front";
  heartbeat["timestamp"] = millis();
  heartbeat["data"]["uptime"] = millis() / 1000;
  heartbeat["data"]["state"] = (int)currentState;
  heartbeat["data"]["safe"] = safetyMonitor.isSafe();
  heartbeat["data"]["emergency"] = emergencyStopTriggered;
  heartbeat["data"]["front_distance"] = lastFrontDistance;
  heartbeat["data"]["rear_distance"] = lastRearDistance;
  heartbeat["data"]["obstacle_threshold"] = OBSTACLE_THRESHOLD;
  heartbeat["data"]["emergency_distance"] = EMERGENCY_STOP_DISTANCE;
  heartbeat["data"]["gas_level"] = gasValue;

  cameraComm.sendMessage(heartbeat);
}

void broadcastSensorDataToCamera()
{
  static unsigned long lastSensorBroadcast = 0;
  const unsigned long SENSOR_BROADCAST_INTERVAL = 100; // 10Hz

  unsigned long now = millis();
  if (now - lastSensorBroadcast < SENSOR_BROADCAST_INTERVAL)
  {
    return;
  }

  lastSensorBroadcast = now;

  StaticJsonDocument<300> sensorMsg;
  sensorMsg["type"] = "sensor_data";
  sensorMsg["source"] = "front";
  sensorMsg["timestamp"] = now;

  JsonObject data = sensorMsg.createNestedObject("data");
  data["front_distance"] = lastFrontDistance;
  data["rear_distance"] = lastRearDistance;
  data["obstacle_detected"] = (lastFrontDistance > 0 && lastFrontDistance < OBSTACLE_THRESHOLD);
  data["emergency_triggered"] = (lastFrontDistance > 0 && lastFrontDistance < EMERGENCY_STOP_DISTANCE);
  data["gas_level"] = gasValue;

  cameraComm.sendMessage(sensorMsg);
}

void receiveMasterCommands()
{
  // Commands from external control system would be received here
  // For now, this is where you'd implement wireless remote control
  // or waypoint navigation commands

  if (Serial.available())
  {
    String input = Serial.readStringUntil('\n');
    input.trim();

    if (input == "forward")
    {
      autonomousMode = false;
      currentState = STATE_MANUAL;
      frontMotors.forward(MOTOR_NORMAL_SPEED);
      DEBUG_PRINTLN("[FRONT] MANUAL: Forward");
    }
    else if (input == "backward")
    {
      autonomousMode = false;
      currentState = STATE_MANUAL;
      frontMotors.backward(MOTOR_NORMAL_SPEED);
      DEBUG_PRINTLN("[FRONT] MANUAL: Backward");
    }
    else if (input == "left")
    {
      autonomousMode = false;
      currentState = STATE_MANUAL;
      frontMotors.turnLeft(MOTOR_TURN_SPEED);
      DEBUG_PRINTLN("[FRONT] MANUAL: Turn Left");
    }
    else if (input == "right")
    {
      autonomousMode = false;
      currentState = STATE_MANUAL;
      frontMotors.turnRight(MOTOR_TURN_SPEED);
      DEBUG_PRINTLN("[FRONT] MANUAL: Turn Right");
    }
    else if (input == "stop")
    {
      currentState = STATE_IDLE;
      frontMotors.stop();
      autonomousMode = false;
      DEBUG_PRINTLN("[FRONT] MANUAL: Stop");
    }
    else if (input == "auto")
    {
      autonomousMode = true;
      currentState = STATE_AUTONOMOUS;
      autonomousNav.reset();
      DEBUG_PRINTLN("[FRONT] AUTO: Autonomous navigation started");
    }
    else if (input == "estop")
    {
      handleEmergencyStop();
    }
    else if (input == "status")
    {
      logSystemStatus();
    }
  }
}

void processCameraFeedback()
{
  if (cameraComm.available())
  {
    StaticJsonDocument<512> cameraMsg = cameraComm.receiveMessage();

    if (cameraMsg["type"] == "status")
    {
      DEBUG_PRINT("[FRONT] Camera Status - Streaming: ");
      DEBUG_PRINTLN(cameraMsg["data"]["streaming"].as<bool>());
    }
  }
}

void receiveRearFeedback()
{
  // Phase 3.1: Receive rear sensor feedback with timestamp synchronization
  if (rearComm.available())
  {
    StaticJsonDocument<512> rearMsg = rearComm.receiveMessage();

    if (rearMsg["type"] == "sensor_feedback")
    {
      lastRearDistance = rearMsg["data"]["rear_distance"].as<float>();
      lastRearDistanceTime = millis(); // Timestamp when we received it

      DEBUG_PRINT("[FRONT] Rear Distance (synced): ");
      DEBUG_PRINT(lastRearDistance);
      DEBUG_PRINTLN(" cm");
    }
  }
}

void handleEmergencyStop()
{
  DEBUG_PRINTLN("\n⚠️⚠️⚠️ EMERGENCY STOP TRIGGERED ⚠️⚠️⚠️\n");

  emergencyStopTriggered = true;
  currentState = STATE_EMERGENCY;
  autonomousMode = false;

  // Stop front motors immediately
  frontMotors.emergencyStop();

  // Notify rear controller
  StaticJsonDocument<256> emergencyMsg;
  emergencyMsg["type"] = "emergency";
  emergencyMsg["source"] = "front";
  emergencyMsg["timestamp"] = millis();
  emergencyMsg["data"]["emergency_stop"] = true;
  emergencyMsg["data"]["reason"] = "Safety violation detected";

  rearComm.sendMessage(emergencyMsg);
  cameraComm.sendMessage(emergencyMsg);

  // Sound alarm
  buzzerAlert(2000, 500);
  delay(100);
  buzzerAlert(2000, 500);

  DEBUG_PRINTLN("Front motors stopped. Waiting for manual reset...");
}

void logSystemStatus()
{
  DEBUG_PRINTLN("\n========== FRONT CONTROLLER STATUS ==========");
  DEBUG_PRINT("Uptime: ");
  DEBUG_PRINT(millis() / 1000);
  DEBUG_PRINTLN(" seconds");
  DEBUG_PRINT("Loop Count: ");
  DEBUG_PRINTLN(loopCounter);
  DEBUG_PRINT("Current State: ");
  DEBUG_PRINTLN((int)currentState);
  DEBUG_PRINT("Autonomous Mode: ");
  DEBUG_PRINTLN(autonomousMode ? "ON" : "OFF");
  DEBUG_PRINT("Front Distance: ");
  DEBUG_PRINT(lastFrontDistance);
  DEBUG_PRINTLN(" cm");
  DEBUG_PRINT("Gas Level: ");
  DEBUG_PRINTLN(gasValue);
  DEBUG_PRINT("Safe: ");
  DEBUG_PRINTLN(safetyMonitor.isSafe() ? "YES" : "NO");
  DEBUG_PRINT("Emergency: ");
  DEBUG_PRINTLN(emergencyStopTriggered ? "YES" : "NO");

  // Sensor health diagnostics
  auto sensorHealth = frontSensor.getHealthStatus();
  DEBUG_PRINTLN("\n--- Front Sensor Health ---");
  DEBUG_PRINT("Total Readings: ");
  DEBUG_PRINTLN(sensorHealth.totalReadings);
  DEBUG_PRINT("Valid: ");
  DEBUG_PRINT(sensorHealth.validReadings);
  DEBUG_PRINT(" | Invalid: ");
  DEBUG_PRINTLN(sensorHealth.invalidReadings);
  DEBUG_PRINT("Availability: ");
  DEBUG_PRINT(sensorHealth.availabilityPercent);
  DEBUG_PRINTLN("%");
  DEBUG_PRINT("Status: ");
  DEBUG_PRINTLN(sensorHealth.isHealthy ? "HEALTHY" : "DEGRADED");

  DEBUG_PRINTLN("============================================\n");
}

#endif // FRONT_CONTROLLER

// ============================================
// REAR CONTROLLER IMPLEMENTATION
// ============================================

#ifdef REAR_CONTROLLER

void initializeRearController()
{
  DEBUG_PRINTLN("  [REAR] Initializing motor drivers...");
  rearMotors.begin();

  DEBUG_PRINTLN("  [REAR] Initializing rear ultrasonic sensor...");
  rearSensor.begin();

  DEBUG_PRINTLN("  [REAR] Initializing safety monitor...");
  safetyMonitor.begin();

  DEBUG_PRINTLN("  [REAR] Initializing UART to Master (Front Controller)...");
  masterComm.begin();

  currentState = STATE_IDLE;
  emergencyStopTriggered = false;
  lastHeartbeatReceived = millis();

  DEBUG_PRINTLN("  [REAR] All systems initialized successfully");
}

void updateRearController()
{
  unsigned long currentTime = millis();

  // ========== SENSOR UPDATES (10Hz) ==========
  if (currentTime - lastNavUpdate >= 100)
  {
    lastNavUpdate = currentTime;
    handleRearSensors();
  }

  // ========== SAFETY CHECKS (5Hz) ==========
  if (currentTime - lastSafetyCheck >= 200)
  {
    lastSafetyCheck = currentTime;
    safetyMonitor.update();

    // Check for emergency conditions
    if (!safetyMonitor.isSafe() || safetyMonitor.isEmergency())
    {
      if (!emergencyStopTriggered)
      {
        emergencyStopTriggered = true;
        handleEmergencyStop();
      }
    }
  }

  // ========== COMMUNICATION WITH MASTER ==========
  receiveMasterCommands();

  // ========== WATCHDOG: Check master heartbeat ==========
  if (currentTime - lastHeartbeatReceived > 3000)
  {
    if (!emergencyStopTriggered)
    {
      DEBUG_PRINTLN("[REAR] ⚠️ Master heartbeat timeout!");
      emergencyStopTriggered = true;
      rearMotors.emergencyStop();
      currentState = STATE_EMERGENCY;
    }
  }

  // ========== PERIODIC STATUS LOGGING ==========
  if (loopCounter % 5000 == 0)
  {
    logSystemStatus();
  }

  loopCounter++;
}

void handleRearSensors()
{
  // Update rear ultrasonic distance using EMA filtering
  lastRearDistance = rearSensor.getSmoothedDistance();

  DEBUG_PRINT("[REAR] Rear Distance: ");
  DEBUG_PRINT(lastRearDistance);
  DEBUG_PRINTLN(" cm");

  // Send sensor feedback to master
  StaticJsonDocument<256> sensorMsg;
  sensorMsg["type"] = "sensor_feedback";
  sensorMsg["source"] = "rear";
  sensorMsg["timestamp"] = millis();
  sensorMsg["data"]["rear_distance"] = lastRearDistance;
  sensorMsg["data"]["robot_state"] = (int)currentState;

  masterComm.sendMessage(sensorMsg);
}

void receiveMasterCommands()
{
  if (masterComm.available())
  {
    StaticJsonDocument<512> masterMsg = masterComm.receiveMessage();

    if (masterMsg["type"] == "heartbeat")
    {
      lastHeartbeatReceived = millis();
      currentState = (RobotState)masterMsg["data"]["state"].as<int>();
    }
    else if (masterMsg["type"] == "motor_command")
    {
      int cmd = masterMsg["data"]["command"].as<int>();
      executeMotorCommand((MovementCommand)cmd);
    }
    else if (masterMsg["type"] == "emergency")
    {
      if (!emergencyStopTriggered)
      {
        emergencyStopTriggered = true;
        handleEmergencyStop();
      }
    }
    else if (masterMsg["type"] == "sensor_update")
    {
      // Receive sensor data from front for safety coordination
      // Update shared safety parameters if needed
    }
  }
}

void executeMotorCommand(MovementCommand cmd)
{
  switch (cmd)
  {
  case CMD_FORWARD:
    rearMotors.forward(MOTOR_NORMAL_SPEED);
    DEBUG_PRINTLN("[REAR] Command: FORWARD");
    break;
  case CMD_BACKWARD:
    rearMotors.backward(MOTOR_NORMAL_SPEED);
    DEBUG_PRINTLN("[REAR] Command: BACKWARD");
    break;
  case CMD_TURN_LEFT:
    rearMotors.turnLeft(MOTOR_TURN_SPEED);
    DEBUG_PRINTLN("[REAR] Command: TURN LEFT");
    break;
  case CMD_TURN_RIGHT:
    rearMotors.turnRight(MOTOR_TURN_SPEED);
    DEBUG_PRINTLN("[REAR] Command: TURN RIGHT");
    break;
  case CMD_CLIMB_BOOST:
    rearMotors.boostFront(CLIMB_BOOST_DURATION);
    DEBUG_PRINTLN("[REAR] Command: CLIMB BOOST");
    break;
  case CMD_ROTATE_360:
    rearMotors.rotate360(true);
    DEBUG_PRINTLN("[REAR] Command: ROTATE 360");
    break;
  case CMD_STOP:
  default:
    rearMotors.stop();
    DEBUG_PRINTLN("[REAR] Command: STOP");
    break;
  }

  lastCommand = cmd;
}

void handleEmergencyStop()
{
  DEBUG_PRINTLN("\n⚠️⚠️⚠️ REAR EMERGENCY STOP ⚠️⚠️⚠️\n");

  emergencyStopTriggered = true;
  currentState = STATE_EMERGENCY;
  rearMotors.emergencyStop();

  DEBUG_PRINTLN("Rear motors stopped immediately.");
}

void logSystemStatus()
{
  DEBUG_PRINTLN("\n========== REAR CONTROLLER STATUS ==========");
  DEBUG_PRINT("Uptime: ");
  DEBUG_PRINT(millis() / 1000);
  DEBUG_PRINTLN(" seconds");
  DEBUG_PRINT("Loop Count: ");
  DEBUG_PRINTLN(loopCounter);
  DEBUG_PRINT("Current State: ");
  DEBUG_PRINTLN((int)currentState);
  DEBUG_PRINT("Rear Distance: ");
  DEBUG_PRINT(lastRearDistance);
  DEBUG_PRINTLN(" cm");
  DEBUG_PRINT("Safe: ");
  DEBUG_PRINTLN(safetyMonitor.isSafe() ? "YES" : "NO");
  DEBUG_PRINT("Emergency: ");
  DEBUG_PRINTLN(emergencyStopTriggered ? "YES" : "NO");

  // Sensor health diagnostics
  auto sensorHealth = rearSensor.getHealthStatus();
  DEBUG_PRINTLN("\n--- Rear Sensor Health ---");
  DEBUG_PRINT("Total Readings: ");
  DEBUG_PRINTLN(sensorHealth.totalReadings);
  DEBUG_PRINT("Valid: ");
  DEBUG_PRINT(sensorHealth.validReadings);
  DEBUG_PRINT(" | Invalid: ");
  DEBUG_PRINTLN(sensorHealth.invalidReadings);
  DEBUG_PRINT("Availability: ");
  DEBUG_PRINT(sensorHealth.availabilityPercent);
  DEBUG_PRINTLN("%");
  DEBUG_PRINT("Status: ");
  DEBUG_PRINTLN(sensorHealth.isHealthy ? "HEALTHY" : "DEGRADED");

  DEBUG_PRINTLN("==========================================\n");
}

#endif // REAR_CONTROLLER

// ============================================
// SHARED UTILITY FUNCTIONS
// ============================================

void setupWatchdog()
{
  // Initialize ESP32 watchdog timer
  // 5 second timeout
  esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
  esp_task_wdt_add(NULL);

  DEBUG_PRINTLN("  [SYSTEM] Watchdog timer initialized");
}

void resetWatchdog()
{
  // Feed the watchdog to prevent reset
  esp_task_wdt_reset();
}

void buzzerAlert(int frequency, int duration)
{
  // Dynamic buzzer control - frequency proportional to smoke level
  // Validates frequency to safe range (800Hz - 4000Hz for typical buzzers)
  if (frequency < 800)
    frequency = 800;
  if (frequency > 4000)
    frequency = 4000;

  pinMode(BUZZER_PIN, OUTPUT);

  unsigned long endTime = millis() + duration;
  unsigned long halfPeriod = 500000 / frequency; // Period in microseconds

  while (millis() < endTime)
  {
    digitalWrite(BUZZER_PIN, HIGH);
    delayMicroseconds(halfPeriod);
    digitalWrite(BUZZER_PIN, LOW);
    delayMicroseconds(halfPeriod);
  }

  digitalWrite(BUZZER_PIN, LOW);
}