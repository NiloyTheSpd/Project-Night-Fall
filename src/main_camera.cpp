/**
 * Project Nightfall - ESP32-CAM (Telemetry Client)
 * 
 * Responsibilities:
 * - WebSocket Client -> connects to Back ESP32 (192.168.4.1)
 * - Receive updates (passive) or send camera-related status
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#define CAMERA_CONTROLLER
#include "config.h"
#include "WiFiManager.h"
#include "MessageProtocol.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

// WS Client connects to Rear ESP32
WSClient_Manager wsClient(WIFI_SSID, WIFI_PASSWORD, "192.168.4.1", WIFI_SERVER_PORT, "camera");

unsigned long lastStatusReport = 0;

// ============================================
// SETUP
// ============================================

void handleWebSocketMessage(const JsonDocument &doc)
{
    const char *type = doc["type"] | "";
    if (strcmp(type, Msg::TYPE_PING) == 0) {
        // Reply?
    }
}

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN("\n\n=== PROJECT NIGHTFALL - CAMERA ESP32 ===");

    // Start WS Client
    wsClient.begin();
    wsClient.setMessageHandler(handleWebSocketMessage);
    
    // Initialize WDT (Optional but recommended)
    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);
    
    DEBUG_PRINTLN("Setup Complete");
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
    esp_task_wdt_reset();
    
    wsClient.update();

    // Report status occasionally
    unsigned long now = millis();
    if (now - lastStatusReport >= 2000)
    {
        lastStatusReport = now;
        
        if (wsClient.isConnected()) {
            StaticJsonDocument<256> doc;
            Msg::buildStatus(doc, Msg::ROLE_CAMERA, "active", "Camera Ready");
            wsClient.sendMessage(doc);
        }
    }
}
