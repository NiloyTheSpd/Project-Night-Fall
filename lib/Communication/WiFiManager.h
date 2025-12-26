#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <ArduinoJson.h>
#include <map>

// Conditional Includes based on Role
#ifdef BACK_CONTROLLER
    #include <WiFi.h>
    #include <AsyncTCP.h>
    #include <ESPAsyncWebServer.h>
#else
    #include <WiFi.h>
    #include <WebSocketsClient.h>
#endif

// ==========================================
// CLIENT MANAGER (Front ESP32 & Camera)
// ==========================================
// Uses arduinoWebSockets (Links2004)
#ifndef BACK_CONTROLLER

class WSClient_Manager
{
public:
    WSClient_Manager(const char *ssid, const char *password, const char *serverIP, uint16_t serverPort, const char *role);

    void begin();
    void update(); // Must be called in loop()
    bool isConnected();
    bool isWiFiConnected();

    void sendMessage(const JsonDocument &doc);
    void setMessageHandler(std::function<void(const JsonDocument &)> handler);

private:
    const char *_ssid;
    const char *_password;
    const char *_serverIP;
    uint16_t _serverPort;
    const char *_role;
    
    bool _wsConnected;
    unsigned long _lastWiFiCheck;
    
    WebSocketsClient _webSocket;
    std::function<void(const JsonDocument &)> _messageHandler;

    static void onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length);
};

#endif

// ==========================================
// SERVER MANAGER (Back ESP32)
// ==========================================
// Uses ESPAsyncWebServer
#ifdef BACK_CONTROLLER

class WSServer_Manager
{
public:
    WSServer_Manager(uint16_t port = 8888);

    void begin();
    void update(); // Placeholder for consistency

    void broadcast(const JsonDocument &doc);
    void setMessageHandler(std::function<void(const JsonDocument &, AsyncWebSocketClient *)> handler);
    
    // Count connected clients
    uint8_t getClientCount();
    String getClientRole(uint32_t id);
    bool isRoleConnected(const char* role);

private:
    AsyncWebServer _server;
    AsyncWebSocket _ws;
    std::function<void(const JsonDocument &, AsyncWebSocketClient *)> _messageHandler;
    
    // Map client ID to Role string
    std::map<uint32_t, String> _clientRoles;

    void onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len);
    void handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client);
};

#endif

#endif // WIFI_MANAGER_H
