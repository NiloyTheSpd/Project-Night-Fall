/**
* Project Nightfall - ESP32-CAM Telemetry Bridge
* Receives telemetry over ESP-NOW and forwards to WebSocket clients.
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

unsigned long lastHelloSent = 0;
AsyncWebServer server(WIFI_SERVER_PORT);
AsyncWebSocket ws("/ws");

static void processMessage(const JsonDocument &doc)
{
  const char *type = doc["type"] | "";
  if (strcmp(type, Msg::TYPE_TELEMETRY) == 0 || strcmp(type, Msg::TYPE_SENSOR_UPDATE) == 0)
  {
    // Log to serial
    serializeJson(doc, Serial);
    Serial.println();
    // Broadcast to WebSocket clients
    String out;
    serializeJson(doc, out);
    ws.textAll(out);
  }
}

static void onRadioMessage(const uint8_t *mac, const uint8_t *data, int len)
{
  StaticJsonDocument<512> doc;
  if (deserializeJson(doc, data, len) != DeserializationError::Ok)
    return;
  processMessage(doc);
}

static void tickHello(unsigned long now)
{
  // Optional: could send status via WebSocket periodically
  (void)now;
}

static void initRadio()
{
  WiFi.mode(WIFI_STA);
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  unsigned long t0 = millis();
  while (WiFi.status() != WL_CONNECTED && (millis() - t0) < 15000) {
    delay(250);
  }
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("WiFi connected. IP: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("WiFi connect timeout");
  }

  if (esp_now_init() != ESP_OK) {
    Serial.println("ESP-NOW init failed");
  } else {
    Serial.println("ESP-NOW init OK");
  }

  esp_now_register_recv_cb(onRadioMessage);

  // WebSocket setup
  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client,
                AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      client->text("{\"type\":\"hello\",\"role\":\"camera\"}");
    }
  });
  server.addHandler(&ws);
  server.begin();
}

void setup()
{
  Serial.begin(SERIAL_BAUD_RATE);
  initRadio();
}

void loop()
{
  unsigned long now = millis();
  tickHello(now);
  ws.cleanupClients();
}
