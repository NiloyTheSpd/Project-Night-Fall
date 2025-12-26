/**
 * Project Nightfall - Front ESP32 (Motor Slave)
 *
 * Responsibilities:
 * - Control 4 DC motors 
 * - Receive motor commands from Back ESP32 via WebSocket
 */

#include <Arduino.h>
#include <ArduinoJson.h>
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

// Motor Banks
L298N frontMotorsBank1(
    MOTOR_FRONT_LEFT1_ENA, MOTOR_FRONT_LEFT1_IN1, MOTOR_FRONT_LEFT1_IN2,
    MOTOR_FRONT_RIGHT1_ENB, MOTOR_FRONT_RIGHT1_IN3, MOTOR_FRONT_RIGHT1_IN4,
    PWM_CHANNEL_FRONT_LEFT1, PWM_CHANNEL_FRONT_RIGHT1);

L298N frontMotorsBank2(
    MOTOR_FRONT_LEFT2_ENA, MOTOR_FRONT_LEFT2_IN1, MOTOR_FRONT_LEFT2_IN2,
    MOTOR_FRONT_RIGHT2_ENB, MOTOR_FRONT_RIGHT2_IN3, MOTOR_FRONT_RIGHT2_IN4,
    PWM_CHANNEL_FRONT_LEFT2, PWM_CHANNEL_FRONT_RIGHT2);

// WS Client
// Connects to Back ESP32 (AP: ProjectNightfall, IP: 192.168.4.1)
WSClient_Manager wsClient(WIFI_SSID, WIFI_PASSWORD, "192.168.4.1", WIFI_SERVER_PORT, "front");

// ============================================
// STATE VARIABLES
// ============================================

bool isConnected = false;
unsigned long lastStatusReport = 0;

// ============================================
// FUNCTIONS
// ============================================

void initMotors();
void handleWebSocketMessage(const JsonDocument &doc);
void handleMotorCommand(int left, int right);
void reportStatus();

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN("\n\n=== PROJECT NIGHTFALL - FRONT ESP32 (SLAVE) ===");
    
    initMotors();

    // Start WebSocket Client
    wsClient.begin();
    wsClient.setMessageHandler(handleWebSocketMessage);

    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
    esp_task_wdt_reset();

    // Must call update() for arduinoWebSockets
    wsClient.update();

    unsigned long now = millis();

    // Report status periodically
    if (now - lastStatusReport >= TELEMETRY_INTERVAL_MS)
    {
        lastStatusReport = now;
        reportStatus();
    }
}

// ============================================
// LOGIC
// ============================================

void initMotors()
{
    DEBUG_PRINTLN("[Motors] Initializing...");
    frontMotorsBank1.begin();
    frontMotorsBank2.begin();
    frontMotorsBank1.stopMotors();
    frontMotorsBank2.stopMotors();
}

void handleWebSocketMessage(const JsonDocument &doc)
{
    Msg::MotorCmd cmd;
    
    // Try Parsing Motor Command
    if (Msg::parseMotorCmd(doc, cmd))
    {
        // Check Target
        if (cmd.target == "front" || cmd.target == "all")
        {
            handleMotorCommand(cmd.leftSpeed, cmd.rightSpeed);
        }
    }
}

void handleMotorCommand(int left, int right)
{
    // Apply to both banks (4 motors total)
    frontMotorsBank1.setMotors(left, right);
    frontMotorsBank2.setMotors(left, right);
    // DEBUG_PRINTF("[Motors] Cmd: %d %d\n", left, right);
}

void reportStatus()
{
    if (WiFi.status() != WL_CONNECTED) return;
    
    StaticJsonDocument<256> doc;
    Msg::buildStatus(doc, Msg::ROLE_FRONT, "active", "Motors OK");
    wsClient.sendMessage(doc);
}
