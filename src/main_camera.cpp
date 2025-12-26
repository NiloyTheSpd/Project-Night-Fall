/**
 * Project Nightfall - ESP32-CAM Telemetry Bridge (REFACTORED)
 *
 * Improvements:
 * - Non-blocking WiFi initialization with exponential backoff
 * - Removed dead code (tickHello function)
 * - Optimized WebSocket client cleanup (every 5 seconds vs every loop)
 * - Added error handling for WiFi disconnects and JSON parsing
 * - Added reconnection logic with timeout tracking
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <esp_now.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

#define CAMERA_CONTROLLER 1
#include "config.h"
#include <MessageProtocol.h>

// ============================================
// TIMING AND STATE
// ============================================

enum WiFiState
{
    WIFI_NOT_STARTED,
    WIFI_CONNECTING,
    WIFI_CONNECTED,
    WIFI_FAILED
};

// Timing constants
static constexpr unsigned long WIFI_CONNECT_TIMEOUT_MS = 10000UL;     // 10 sec max per attempt
static constexpr unsigned long WIFI_RECONNECT_DELAY_MIN_MS = 2000UL;  // Start with 2 sec backoff
static constexpr unsigned long WIFI_RECONNECT_DELAY_MAX_MS = 30000UL; // Cap at 30 sec backoff
static constexpr unsigned long WS_CLEANUP_INTERVAL_MS = 5000UL;       // Cleanup clients every 5 sec
static constexpr unsigned long ESPNOW_INIT_TIMEOUT_MS = 5000UL;       // Wait 5 sec for ESP-NOW

// WiFi state tracking
static WiFiState wifiState = WIFI_NOT_STARTED;
static unsigned long wifiConnectStartTime = 0;
static unsigned long wifiReconnectDelay = WIFI_RECONNECT_DELAY_MIN_MS;
static unsigned long lastWiFiAttempt = 0;
static unsigned long lastWsCleanup = 0;
static int wifiFailCount = 0;

// Server and WebSocket
AsyncWebServer server(WIFI_SERVER_PORT);
AsyncWebSocket ws("/ws");

// ============================================
// MESSAGE PROCESSING
// ============================================

/**
 * Process received telemetry message
 * Validates JSON and broadcasts to WebSocket clients
 */
static void processMessage(const JsonDocument &doc)
{
    const char *type = doc["type"] | "";

    // Validate message type
    if (type == nullptr || type[0] == '\0')
    {
        DEBUG_PRINTLN("[JSON] Invalid message: missing type");
        return;
    }

    // Forward telemetry and sensor updates to WebSocket
    if (strcmp(type, Msg::TYPE_TELEMETRY) == 0 ||
        strcmp(type, Msg::TYPE_SENSOR_UPDATE) == 0 ||
        strcmp(type, Msg::TYPE_HAZARD_ALERT) == 0)
    {

        // Log to serial (debug)
        serializeJson(doc, Serial);
        Serial.println();

        // Broadcast to WebSocket clients
        String out;
        serializeJson(doc, out);
        ws.textAll(out);
        DEBUG_PRINTF("[WS] Broadcast message: %s\n", type);
    }
}

/**
 * ESP-NOW receive callback
 * Called when telemetry received from Back ESP32
 */
static void onRadioMessage(const uint8_t *mac, const uint8_t *data, int len)
{
    StaticJsonDocument<512> doc;
    DeserializationError error = deserializeJson(doc, data, len);

    if (error)
    {
        DEBUG_PRINTF("[ESP-NOW] JSON parse error: %s\n", error.c_str());
        return;
    }

    processMessage(doc);
}

// ============================================
// WIFI INITIALIZATION (NON-BLOCKING)
// ============================================

/**
 * Start WiFi connection attempt (non-blocking)
 * Called once per retry period
 */
static void startWiFiConnect()
{
    unsigned long now = millis();

    // Skip if not enough delay since last attempt
    if (now - lastWiFiAttempt < wifiReconnectDelay)
    {
        return;
    }

    lastWiFiAttempt = now;
    wifiConnectStartTime = now;
    wifiState = WIFI_CONNECTING;

    DEBUG_PRINTF("[WiFi] Attempting connect (attempt %d, delay was %lums)\n",
                 wifiFailCount + 1, wifiReconnectDelay);

    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
}

/**
 * Update WiFi connection state machine (non-blocking)
 * Call every loop iteration
 */
static void updateWiFi()
{
    unsigned long now = millis();

    switch (wifiState)
    {
    case WIFI_NOT_STARTED:
        // Kick off first connection attempt
        startWiFiConnect();
        break;

    case WIFI_CONNECTING:
        // Check if connected
        if (WiFi.status() == WL_CONNECTED)
        {
            wifiState = WIFI_CONNECTED;
            wifiFailCount = 0;
            wifiReconnectDelay = WIFI_RECONNECT_DELAY_MIN_MS; // Reset delay
            DEBUG_PRINTF("[WiFi] Connected! IP: %s\n", WiFi.localIP().toString().c_str());
        }
        // Check if timeout reached
        else if (now - wifiConnectStartTime >= WIFI_CONNECT_TIMEOUT_MS)
        {
            wifiState = WIFI_FAILED;
            wifiFailCount++;

            // Increase backoff delay exponentially (up to max)
            wifiReconnectDelay = min(
                (unsigned long)(wifiReconnectDelay * 1.5),
                WIFI_RECONNECT_DELAY_MAX_MS);

            DEBUG_PRINTF("[WiFi] Connection timeout. Next retry in %lums\n", wifiReconnectDelay);
            WiFi.disconnect();
        }
        break;

    case WIFI_CONNECTED:
        // Check if still connected
        if (WiFi.status() != WL_CONNECTED)
        {
            wifiState = WIFI_FAILED;
            wifiFailCount++;
            wifiReconnectDelay = WIFI_RECONNECT_DELAY_MIN_MS; // Reset to minimum
            DEBUG_PRINTLN("[WiFi] Connection lost, will retry...");
            WiFi.disconnect();
        }
        break;

    case WIFI_FAILED:
        // Wait for backoff timer and retry
        if (now - lastWiFiAttempt >= wifiReconnectDelay)
        {
            startWiFiConnect();
        }
        break;
    }
}

/**
 * Initialize ESP-NOW (called once after WiFi ready)
 */
static void initESPNow()
{
    DEBUG_PRINTLN("[ESP-NOW] Initializing...");

    if (esp_now_init() != ESP_OK)
    {
        DEBUG_PRINTLN("[ESP-NOW] Init failed!");
        return;
    }

    DEBUG_PRINTLN("[ESP-NOW] Init OK");
    esp_now_register_recv_cb(onRadioMessage);
}

/**
 * Initialize WebSocket handler (called once)
 */
static void initWebSocket()
{
    DEBUG_PRINTLN("[WebSocket] Initializing...");

    ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                  AwsEventType type, void *arg, uint8_t *data, size_t len)
               {
        if (type == WS_EVT_CONNECT) {
            DEBUG_PRINTF("[WebSocket] Client %d connected\n", client->id());
            client->text("{\"type\":\"hello\",\"role\":\"camera\"}");
        }
        else if (type == WS_EVT_DISCONNECT) {
            DEBUG_PRINTF("[WebSocket] Client %d disconnected\n", client->id());
        } });

    server.addHandler(&ws);
    server.begin();

    DEBUG_PRINTLN("[WebSocket] Server started on port 8888");
}

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN("\n\n=== PROJECT NIGHTFALL - ESP32-CAM BRIDGE (REFACTORED) ===");
    DEBUG_PRINTLN("[Setup] Initializing...");

    // Initialize WebSocket first (listening mode)
    initWebSocket();

    // WiFi will be managed non-blocking in loop
    DEBUG_PRINTLN("[Setup] Complete - WiFi connecting in background");
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
    unsigned long now = millis();

    // Non-blocking WiFi state machine
    updateWiFi();

    // Initialize ESP-NOW once WiFi is connected
    static bool espNowInitialized = false;
    if (!espNowInitialized && wifiState == WIFI_CONNECTED)
    {
        initESPNow();
        espNowInitialized = true;
    }

    // Periodic WebSocket client cleanup (every 5 seconds, not every loop)
    if (now - lastWsCleanup >= WS_CLEANUP_INTERVAL_MS)
    {
        lastWsCleanup = now;
        ws.cleanupClients();
        DEBUG_PRINTLN("[WebSocket] Cleanup completed");
    }
}
