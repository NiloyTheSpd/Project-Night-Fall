/**
 * Project Nightfall - Back ESP32 (Master Brain)
 *
 * Responsibilities:
 * - Sensor acquisition (2x ultrasonic, gas sensor)
 * - Safety monitoring & hazard detection
 * - Obstacle avoidance & auto-climb logic
 * - Autonomous navigation
 * - Rear motor control (L298N direct)
 * - Front motor command distribution
 * - Telemetry broadcast
 * - WiFi server (coordinator)
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <esp_task_wdt.h>

#define BACK_CONTROLLER
#include "config.h"
#include "pins.h"

#include "L298N.h"
#include "UltrasonicSensor.h"
#include "MQ2Sensor.h"
#include "WiFiManager.h"
#include "MessageProtocol.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

// Motors
L298N rearMotors(
    MOTOR_REAR_LEFT_ENA, MOTOR_REAR_LEFT_IN1, MOTOR_REAR_LEFT_IN2,
    MOTOR_REAR_RIGHT_ENB, MOTOR_REAR_RIGHT_IN3, MOTOR_REAR_RIGHT_IN4,
    PWM_CHANNEL_REAR_LEFT, PWM_CHANNEL_REAR_RIGHT);

// Sensors
UltrasonicSensor frontSensor(ULTRASONIC_FRONT_TRIG, ULTRASONIC_FRONT_ECHO);
UltrasonicSensor rearSensor(ULTRASONIC_REAR_TRIG, ULTRASONIC_REAR_ECHO);
MQ2Sensor gasSensor(GAS_SENSOR_ANALOG, GAS_SENSOR_DIGITAL);

// WiFi Server
WiFiServer_Manager wifiServer(WIFI_SERVER_PORT);

// ============================================
// STATE VARIABLES
// ============================================

RobotState currentState = STATE_INIT;
NavigationState navState = NAV_FORWARD;
bool autonomousMode = false;
bool emergencyStopActive = false;

// Sensor readings
float frontDistance = 0;
float rearDistance = 0;
int gasLevel = 0;

// Motor speeds
int rearLeftSpeed = 0;
int rearRightSpeed = 0;
int frontLeftSpeed = 0;
int frontRightSpeed = 0;

// Timing
unsigned long lastSensorUpdate = 0;
unsigned long lastNavUpdate = 0;
unsigned long lastTelemetryBroadcast = 0;
unsigned long lastLoopTime = 0;

// ============================================
// FUNCTION DECLARATIONS
// ============================================

void initSensors();
void initMotors();
void initWiFi();
void updateSensors();
void updateAutonomousNav();
void broadcastTelemetry();
void sendMotorCommandToFront(int leftSpeed, int rightSpeed);
void handleEmergencyStop();
void handleWiFiMessage(const JsonDocument &doc, AsyncClient *client);
void logStatus();

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN("\n\n=== PROJECT NIGHTFALL - BACK ESP32 (MASTER) ===");
    DEBUG_PRINTLN("Initializing...");

    // Initialize components
    initMotors();
    initSensors();
    initWiFi();

    currentState = STATE_IDLE;
    DEBUG_PRINTLN("INIT COMPLETE - Ready for commands");

    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
    unsigned long loopStart = millis();
    esp_task_wdt_reset();

    // WiFi updates (event-driven)
    wifiServer.update();

    // Update sensors at fixed interval
    if (loopStart - lastSensorUpdate >= SENSOR_UPDATE_INTERVAL_MS)
    {
        lastSensorUpdate = loopStart;
        updateSensors();
    }

    // Update navigation at fixed interval
    if (autonomousMode && (loopStart - lastNavUpdate >= NAVIGATION_UPDATE_INTERVAL_MS))
    {
        lastNavUpdate = loopStart;
        updateAutonomousNav();
    }

    // Broadcast telemetry
    if (loopStart - lastTelemetryBroadcast >= TELEMETRY_INTERVAL_MS)
    {
        lastTelemetryBroadcast = loopStart;
        broadcastTelemetry();
    }

    // Check for gas hazard - highest priority
    if (gasSensor.isGasDetected(GAS_THRESHOLD_EMERGENCY))
    {
        handleEmergencyStop();
    }

    // Frame rate limiting
    unsigned long loopDuration = millis() - loopStart;
    if (loopDuration < MAIN_LOOP_RATE_MS)
    {
        delay(MAIN_LOOP_RATE_MS - loopDuration);
    }
}

// ============================================
// INITIALIZATION FUNCTIONS
// ============================================

void initMotors()
{
    DEBUG_PRINTLN("[Motors] Initializing rear L298N driver...");
    rearMotors.begin();
    rearMotors.stopMotors();
    DEBUG_PRINTLN("[Motors] Rear motors ready");
}

void initSensors()
{
    DEBUG_PRINTLN("[Sensors] Initializing ultrasonic sensors...");
    frontSensor.begin();
    rearSensor.begin();
    DEBUG_PRINTLN("[Sensors] Initializing gas sensor...");
    gasSensor.begin();
    DEBUG_PRINTLN("[Sensors] All sensors ready");
}

void initWiFi()
{
    DEBUG_PRINTF("[WiFi] Starting AP: %s\n", WIFI_SSID);
    DEBUG_PRINTF("[WiFi] Server on port %d\n", WIFI_SERVER_PORT);

    wifiServer.begin();

    wifiServer.setMessageHandler([](const JsonDocument &doc, AsyncClient *client)
                                 { handleWiFiMessage(doc, client); });

    DEBUG_PRINTLN("[WiFi] Ready for connections");
}

// ============================================
// SENSOR UPDATES
// ============================================

void updateSensors()
{
    // Update sensor readings
    frontSensor.update();
    rearSensor.update();
    gasSensor.update();

    frontDistance = frontSensor.getDistance();
    rearDistance = rearSensor.getDistance();
    gasLevel = gasSensor.getSmoothedReading();

    DEBUG_PRINTF("[Sensors] F:%.1fcm R:%.1fcm Gas:%d\n",
                 frontDistance, rearDistance, gasLevel);
}

// ============================================
// AUTONOMOUS NAVIGATION
// ============================================

void updateAutonomousNav()
{
    // Simple obstacle avoidance state machine

    if (emergencyStopActive)
    {
        navState = NAV_IDLE;
        rearMotors.stopMotors();
        return;
    }

    // Check for obstacles
    bool frontObstacle = frontSensor.isObstacleDetected(ULTRASONIC_THRESHOLD_OBSTACLE);
    bool rearObstacle = rearSensor.isObstacleDetected(ULTRASONIC_THRESHOLD_OBSTACLE);

    switch (navState)
    {
    case NAV_FORWARD:
        if (frontObstacle)
        {
            // Obstacle detected - switch to avoidance
            navState = NAV_OBSTACLE_DETECTED;
            DEBUG_PRINTLN("[Nav] Obstacle detected - initiating avoidance");
        }
        else
        {
            // Continue forward
            rearMotors.setMotorsForward(MOTOR_NORMAL_SPEED);
            rearLeftSpeed = MOTOR_NORMAL_SPEED;
            rearRightSpeed = MOTOR_NORMAL_SPEED;
        }
        break;

    case NAV_OBSTACLE_DETECTED:
        // Stop and assess
        rearMotors.stopMotors();
        navState = NAV_AVOID_LEFT;
        break;

    case NAV_AVOID_LEFT:
        // Try turning left
        rearMotors.turnLeft(MOTOR_TURN_SPEED);
        navState = NAV_FORWARD;
        DEBUG_PRINTLN("[Nav] Turning left to avoid obstacle");
        break;

    case NAV_AVOID_RIGHT:
        // Try turning right
        rearMotors.turnRight(MOTOR_TURN_SPEED);
        navState = NAV_FORWARD;
        DEBUG_PRINTLN("[Nav] Turning right to avoid obstacle");
        break;

    case NAV_CLIMBING:
        // Boost rear motors for climbing
        if (frontDistance < ULTRASONIC_THRESHOLD_CLIFF)
        {
            rearMotors.setMotorsForward(MOTOR_CLIMB_SPEED);
            rearLeftSpeed = MOTOR_CLIMB_SPEED;
            rearRightSpeed = MOTOR_CLIMB_SPEED;
        }
        else
        {
            navState = NAV_FORWARD;
        }
        break;

    case NAV_IDLE:
        rearMotors.stopMotors();
        break;

    default:
        navState = NAV_FORWARD;
        break;
    }

    // Send command to Front ESP32
    sendMotorCommandToFront(rearLeftSpeed, rearRightSpeed);
}

// ============================================
// COMMUNICATION
// ============================================

void sendMotorCommandToFront(int leftSpeed, int rightSpeed)
{
    StaticJsonDocument<256> cmd;
    Msg::buildMotorCmd(cmd, leftSpeed, rightSpeed, "front");

    wifiServer.broadcast(cmd);
}

void broadcastTelemetry()
{
    StaticJsonDocument<512> telemetry;
    Msg::buildTelemetry(telemetry,
                        frontDistance, rearDistance, gasLevel,
                        frontLeftSpeed, frontRightSpeed,
                        rearLeftSpeed, rearRightSpeed,
                        autonomousMode,
                        navState == NAV_FORWARD ? "forward" : "obstacle_avoidance");

    wifiServer.broadcast(telemetry);
}

void handleWiFiMessage(const JsonDocument &doc, AsyncClient *client)
{
    const char *msgType = doc["type"] | "";

    DEBUG_PRINTF("[WiFi] Received: %s\n", msgType);

    if (strcmp(msgType, Msg::TYPE_UI_CMD) == 0)
    {
        const char *cmd = doc["cmd"] | "";

        if (strcmp(cmd, "auto_on") == 0)
        {
            autonomousMode = true;
            navState = NAV_FORWARD;
            currentState = STATE_AUTONOMOUS;
            DEBUG_PRINTLN("[Nav] Autonomous mode ON");
        }
        else if (strcmp(cmd, "auto_off") == 0)
        {
            autonomousMode = false;
            currentState = STATE_IDLE;
            rearMotors.stopMotors();
            DEBUG_PRINTLN("[Nav] Autonomous mode OFF");
        }
        else if (strcmp(cmd, "forward") == 0)
        {
            autonomousMode = false;
            currentState = STATE_MANUAL;
            rearMotors.setMotorsForward(MOTOR_NORMAL_SPEED);
        }
        else if (strcmp(cmd, "backward") == 0)
        {
            autonomousMode = false;
            currentState = STATE_MANUAL;
            rearMotors.setMotorsBackward(MOTOR_NORMAL_SPEED);
        }
        else if (strcmp(cmd, "left") == 0)
        {
            autonomousMode = false;
            currentState = STATE_MANUAL;
            rearMotors.turnLeft(MOTOR_TURN_SPEED);
        }
        else if (strcmp(cmd, "right") == 0)
        {
            autonomousMode = false;
            currentState = STATE_MANUAL;
            rearMotors.turnRight(MOTOR_TURN_SPEED);
        }
        else if (strcmp(cmd, "stop") == 0)
        {
            autonomousMode = false;
            currentState = STATE_IDLE;
            rearMotors.stopMotors();
        }
        else if (strcmp(cmd, "estop") == 0)
        {
            handleEmergencyStop();
        }
    }

    // Send acknowledgment
    StaticJsonDocument<128> ack;
    Msg::buildAck(ack, Msg::ROLE_BACK, msgType, true);
    wifiServer.sendTo(client, ack);
}

void handleEmergencyStop()
{
    if (emergencyStopActive)
        return;

    emergencyStopActive = true;
    currentState = STATE_EMERGENCY;
    autonomousMode = false;
    navState = NAV_IDLE;

    rearMotors.stopMotors();
    rearLeftSpeed = 0;
    rearRightSpeed = 0;

    // Buzz buzzer
    pinMode(BUZZER_PIN, OUTPUT);
    digitalWrite(BUZZER_PIN, HIGH);
    delay(BUZZER_ALERT_DURATION_MS);
    digitalWrite(BUZZER_PIN, LOW);

    // Broadcast alert
    StaticJsonDocument<256> alert;
    Msg::buildHazardAlert(alert, Msg::HAZARD_GAS,
                          "GAS DETECTED - EMERGENCY STOP ACTIVATED", true);
    wifiServer.broadcast(alert);

    DEBUG_PRINTLN("[EMERGENCY] STOP TRIGGERED - GAS DETECTED!");
}

void logStatus()
{
    DEBUG_PRINTF("[Status] State:%d NavState:%d Auto:%d\n",
                 currentState, navState, autonomousMode);
}
