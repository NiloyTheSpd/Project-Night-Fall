/**
 * Project Nightfall - Front ESP32 (Motor Slave)
 *
 * Responsibilities:
 * - Control 4 DC motors via 2x L298N drivers
 * - Receive motor commands from Back ESP32 over WiFi
 * - Execute commands non-blockingly
 * - Report status/acknowledge commands
 *
 * Motor Layout:
 * - L298N Driver 1: Front Left Motor 1, Front Right Motor 1
 * - L298N Driver 2: Front Left Motor 2, Front Right Motor 2
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <esp_task_wdt.h>

#define FRONT_CONTROLLER
#include "config.h"
#include "pins.h"

#include "L298N.h"
#include "WiFiManager.h"
#include "MessageProtocol.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

// Motor Controllers
L298N frontMotorsBank1(
    MOTOR_FRONT_LEFT1_ENA, MOTOR_FRONT_LEFT1_IN1, MOTOR_FRONT_LEFT1_IN2,
    MOTOR_FRONT_RIGHT1_ENB, MOTOR_FRONT_RIGHT1_IN3, MOTOR_FRONT_RIGHT1_IN4,
    PWM_CHANNEL_FRONT_LEFT1, PWM_CHANNEL_FRONT_RIGHT1);

L298N frontMotorsBank2(
    MOTOR_FRONT_LEFT2_ENA, MOTOR_FRONT_LEFT2_IN1, MOTOR_FRONT_LEFT2_IN2,
    MOTOR_FRONT_RIGHT2_ENB, MOTOR_FRONT_RIGHT2_IN3, MOTOR_FRONT_RIGHT2_IN4,
    PWM_CHANNEL_FRONT_LEFT2, PWM_CHANNEL_FRONT_RIGHT2);

// WiFi Client
WiFiClient_Manager wifiClient(WIFI_SSID, WIFI_PASSWORD,
                              "192.168.4.1", WIFI_SERVER_PORT);

// ============================================
// STATE VARIABLES
// ============================================

RobotState currentState = STATE_INIT;
bool isConnected = false;

// Current motor speeds
int motorLeft1Speed = 0;
int motorRight1Speed = 0;
int motorLeft2Speed = 0;
int motorRight2Speed = 0;

// Timing
unsigned long lastStatusReport = 0;
unsigned long lastConnectionCheck = 0;

// ============================================
// FUNCTION DECLARATIONS
// ============================================

void initMotors();
void initWiFi();
void handleMotorCommand(int leftSpeed, int rightSpeed);
void handleWiFiMessage(const JsonDocument &doc);
void reportStatus();
void setupMotorCommand(int leftSpeed, int rightSpeed);

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN("\n\n=== PROJECT NIGHTFALL - FRONT ESP32 (MOTOR SLAVE) ===");
    DEBUG_PRINTLN("Initializing...");

    initMotors();
    initWiFi();

    currentState = STATE_IDLE;
    DEBUG_PRINTLN("INIT COMPLETE - Waiting for Back ESP32...");

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

    // WiFi update (check connection, receive messages)
    wifiClient.update();

    // Check connection status
    if (loopStart - lastConnectionCheck >= 1000)
    {
        lastConnectionCheck = loopStart;
        bool connected = wifiClient.isConnected();

        if (connected != isConnected)
        {
            isConnected = connected;
            if (isConnected)
            {
                DEBUG_PRINTLN("[WiFi] Connected to Back ESP32");
                currentState = STATE_IDLE;
                reportStatus();
            }
            else
            {
                DEBUG_PRINTLN("[WiFi] Disconnected from Back ESP32");
                currentState = STATE_ERROR;
                // Stop all motors on disconnect
                frontMotorsBank1.stopMotors();
                frontMotorsBank2.stopMotors();
            }
        }
    }

    // Report status periodically
    if (loopStart - lastStatusReport >= TELEMETRY_INTERVAL_MS)
    {
        lastStatusReport = loopStart;
        reportStatus();
    }

    // Frame rate limiting
    unsigned long loopDuration = millis() - loopStart;
    if (loopDuration < MAIN_LOOP_RATE_MS)
    {
        delay(MAIN_LOOP_RATE_MS - loopDuration);
    }
}

// ============================================
// INITIALIZATION
// ============================================

void initMotors()
{
    DEBUG_PRINTLN("[Motors] Initializing motor banks...");

    frontMotorsBank1.begin();
    frontMotorsBank2.begin();

    frontMotorsBank1.stopMotors();
    frontMotorsBank2.stopMotors();

    DEBUG_PRINTLN("[Motors] 4-motor system ready");
}

void initWiFi()
{
    DEBUG_PRINTF("[WiFi] Connecting to AP: %s\n", WIFI_SSID);
    DEBUG_PRINTF("[WiFi] Server: 192.168.4.1:%d\n", WIFI_SERVER_PORT);

    wifiClient.begin();

    // Register message handler
    wifiClient.setMessageHandler([](const JsonDocument &doc)
                                 { handleWiFiMessage(doc); });

    DEBUG_PRINTLN("[WiFi] Initialized (waiting for Back ESP32)");
}

// ============================================
// MOTOR CONTROL
// ============================================

void setupMotorCommand(int leftSpeed, int rightSpeed)
{
    // Distribute speed to both motor banks
    // Left bank gets left speed, right bank gets right speed

    motorLeft1Speed = leftSpeed;
    motorRight1Speed = rightSpeed;
    motorLeft2Speed = leftSpeed;
    motorRight2Speed = rightSpeed;

    // Apply to bank 1
    frontMotorsBank1.setMotors(motorLeft1Speed, motorRight1Speed);

    // Apply to bank 2
    frontMotorsBank2.setMotors(motorLeft2Speed, motorRight2Speed);

    DEBUG_PRINTF("[Motors] Cmd: L:%d R:%d (applied to both banks)\n",
                 leftSpeed, rightSpeed);
}

void handleMotorCommand(int leftSpeed, int rightSpeed)
{
    if (!isConnected)
    {
        DEBUG_PRINTLN("[Motors] Ignoring command - not connected");
        return;
    }

    currentState = STATE_MANUAL;
    setupMotorCommand(leftSpeed, rightSpeed);
}

// ============================================
// COMMUNICATION
// ============================================

void handleWiFiMessage(const JsonDocument &doc)
{
    const char *msgType = doc["type"] | "";

    DEBUG_PRINTF("[WiFi] Received: %s\n", msgType);

    if (strcmp(msgType, Msg::TYPE_MOTOR_CMD) == 0)
    {
        int leftSpeed = doc["left"] | 0;
        int rightSpeed = doc["right"] | 0;
        const char *target = doc["target"] | "";

        // Only execute if meant for us
        if (strcmp(target, "front") == 0 || strcmp(target, "all") == 0)
        {
            handleMotorCommand(leftSpeed, rightSpeed);

            // Send acknowledgment
            StaticJsonDocument<128> ack;
            Msg::buildAck(ack, Msg::ROLE_FRONT, Msg::TYPE_MOTOR_CMD, true);
            wifiClient.sendMessage(ack);
        }
    }
    else if (strcmp(msgType, Msg::TYPE_TELEMETRY) == 0)
    {
        // Just logging telemetry updates
        DEBUG_PRINTLN("[Data] Telemetry received from Back");
    }
}

void reportStatus()
{
    StaticJsonDocument<256> status;
    Msg::buildStatus(status, Msg::ROLE_FRONT,
                     isConnected ? "connected" : "disconnected",
                     "Front motor controller active");

    wifiClient.sendMessage(status);
}
