#ifndef WIFI_MANAGER_H
#define WIFI_MANAGER_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ArduinoJson.h>

// Only include ESPAsyncWebServer if building for back_esp32 (server)
#ifdef BACK_CONTROLLER
#include <ESPAsyncWebServer.h>
#endif

/**
 * WiFi Client abstraction for Front ESP32 and ESP32-CAM
 * Maintains connection to Back ESP32 server and provides message handlers
 */
class WiFiClient_Manager
{
public:
    WiFiClient_Manager(const char *ssid, const char *password, const char *serverIP, uint16_t serverPort);

    void begin();
    void update();
    bool isConnected();

    /**
     * Send JSON message to server
     */
    void sendMessage(const JsonDocument &doc);

    /**
     * Register message handler callback
     */
    void setMessageHandler(std::function<void(const JsonDocument &)> handler)
    {
        _messageHandler = handler;
    }

    /**
     * Get connection status
     */
    const char *getStatus() { return _isConnected ? "connected" : "disconnected"; }

private:
    const char *_ssid;
    const char *_password;
    const char *_serverIP;
    uint16_t _serverPort;
    bool _isConnected;
    AsyncClient *_client;
    String _rxBuffer;
    unsigned long _lastReconnectAttempt;

    std::function<void(const JsonDocument &)> _messageHandler;

    void connect();
    void onData(void *data, size_t len);
    void onError(uint8_t err);
    void onDisconnect();

    static constexpr unsigned long RECONNECT_INTERVAL_MS = 5000UL;
};

/**
 * WiFi Server abstraction for Back ESP32
 * Hosts TCP server for clients to connect
 */
class WiFiServer_Manager
{
public:
    WiFiServer_Manager(uint16_t port = 8888);

    void begin();
    void update();

    /**
     * Register message handler callback
     */
    void setMessageHandler(std::function<void(const JsonDocument &, AsyncClient *)> handler)
    {
        _messageHandler = handler;
    }

    /**
     * Broadcast message to all connected clients
     */
    void broadcast(const JsonDocument &doc);

    /**
     * Send message to specific client
     */
    void sendTo(AsyncClient *client, const JsonDocument &doc);

    /**
     * Get number of connected clients
     */
    uint8_t getClientCount() { return _clientCount; }

private:
    AsyncServer *_server;
    uint16_t _port;
    uint8_t _clientCount;

    std::function<void(const JsonDocument &, AsyncClient *)> _messageHandler;

    // Static callback handlers for AsyncServer
    static void onConnect(void *arg, AsyncClient *client);
    static void onData(void *arg, AsyncClient *client, void *data, size_t len);
    static void onDisconnect(void *arg, AsyncClient *client);
};

#endif // WIFI_MANAGER_H
