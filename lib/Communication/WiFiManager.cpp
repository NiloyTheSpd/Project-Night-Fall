#include "WiFiManager.h"

// ============== WiFiClient_Manager ==============

WiFiClient_Manager::WiFiClient_Manager(const char *ssid, const char *password, const char *serverIP, uint16_t serverPort)
    : _ssid(ssid), _password(password), _serverIP(serverIP), _serverPort(serverPort),
      _isConnected(false), _client(nullptr), _lastReconnectAttempt(0)
{
}

void WiFiClient_Manager::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    Serial.println("[WiFiClient] Connecting to WiFi...");
}

void WiFiClient_Manager::update()
{
    if (!_isConnected)
    {
        unsigned long now = millis();
        if (now - _lastReconnectAttempt >= RECONNECT_INTERVAL_MS)
        {
            _lastReconnectAttempt = now;
            connect();
        }
    }
}

bool WiFiClient_Manager::isConnected()
{
    return _isConnected && _client && _client->connected();
}

void WiFiClient_Manager::sendMessage(const JsonDocument &doc)
{
    if (!isConnected())
        return;

    String msg;
    serializeJson(doc, msg);
    msg += "\n";
    _client->write(msg.c_str(), msg.length());
}

void WiFiClient_Manager::connect()
{
    if (WiFi.status() == WL_CONNECTED)
    {
        _client = new AsyncClient();

        _client->onData([this](void *arg, AsyncClient *client, void *data, size_t len)
                        { onData(data, len); });

        _client->onError([this](void *arg, AsyncClient *client, int8_t error)
                         { onError(error); });

        _client->onDisconnect([this](void *arg, AsyncClient *client)
                              { onDisconnect(); });

        _client->connect(_serverIP, _serverPort);
        Serial.printf("[WiFiClient] Connecting to server %s:%d\n", _serverIP, _serverPort);
    }
}

void WiFiClient_Manager::onData(void *data, size_t len)
{
    const char *buffer = (const char *)data;
    for (size_t i = 0; i < len; i++)
    {
        _rxBuffer += buffer[i];

        if (buffer[i] == '\n')
        {
            // Parse JSON line
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, _rxBuffer.c_str());

            if (!error && _messageHandler)
            {
                _messageHandler(doc);
            }
            else if (error)
            {
                Serial.printf("[WiFiClient] JSON parse error: %s\n", error.c_str());
            }

            _rxBuffer.clear();
        }
    }
}

void WiFiClient_Manager::onError(uint8_t err)
{
    Serial.printf("[WiFiClient] Connection error: %d\n", err);
    _isConnected = false;
}

void WiFiClient_Manager::onDisconnect()
{
    Serial.println("[WiFiClient] Disconnected from server");
    _isConnected = false;
    if (_client)
    {
        delete _client;
        _client = nullptr;
    }
}

// ============== WiFiServer_Manager ==============

// Singleton instance for static callback access
static WiFiServer_Manager *g_serverInstance = nullptr;

WiFiServer_Manager::WiFiServer_Manager(uint16_t port)
    : _port(port), _clientCount(0), _server(nullptr)
{
    g_serverInstance = this; // Store for static callbacks
}

void WiFiServer_Manager::begin()
{
    WiFi.mode(WIFI_AP_STA);
    WiFi.softAP("ProjectNightfall", "rescue2025");
    Serial.printf("[WiFiServer] AP started: %s\n", "ProjectNightfall");

    // Create AsyncServer for TCP connections
    _server = new AsyncServer(_port);
    _server->onClient(WiFiServer_Manager::onConnect, nullptr);
    _server->begin();
    Serial.printf("[WiFiServer] TCP server listening on port %d\n", _port);
}

void WiFiServer_Manager::update()
{
    // Server is event-driven - this is a placeholder for future expansion
}

void WiFiServer_Manager::broadcast(const JsonDocument &doc)
{
    String msg;
    serializeJson(doc, msg);
    msg += "\n";

    // In a full implementation, maintain a client list
    // For now, this is a placeholder - broadcasting would be done
    // via AsyncWebSocket or custom client tracking
}

void WiFiServer_Manager::sendTo(AsyncClient *client, const JsonDocument &doc)
{
    if (!client || !client->connected())
        return;

    String msg;
    serializeJson(doc, msg);
    msg += "\n";
    client->write(msg.c_str(), msg.length());
}

// Static callbacks - access singleton instance
void WiFiServer_Manager::onConnect(void *arg, AsyncClient *client)
{
    if (!g_serverInstance)
        return;

    g_serverInstance->_clientCount++;
    Serial.printf("[WiFiServer] Client connected! Total: %d\n", g_serverInstance->_clientCount);

    // Register data and disconnect handlers
    client->onData([](void *arg, AsyncClient *c, void *data, size_t len)
                   { WiFiServer_Manager::onData(arg, c, data, len); });
    client->onDisconnect([](void *arg, AsyncClient *c)
                         { WiFiServer_Manager::onDisconnect(arg, c); });
}

void WiFiServer_Manager::onData(void *arg, AsyncClient *client, void *data, size_t len)
{
    if (!g_serverInstance)
        return;

    const char *buffer = (const char *)data;
    String rxBuffer;

    for (size_t i = 0; i < len; i++)
    {
        rxBuffer += buffer[i];

        if (buffer[i] == '\n')
        {
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, rxBuffer.c_str());

            if (!error && g_serverInstance->_messageHandler)
            {
                g_serverInstance->_messageHandler(doc, client);
            }
            else if (error)
            {
                Serial.printf("[WiFiServer] JSON parse error: %s\n", error.c_str());
            }

            rxBuffer.clear();
        }
    }
}

void WiFiServer_Manager::onDisconnect(void *arg, AsyncClient *client)
{
    if (!g_serverInstance)
        return;

    g_serverInstance->_clientCount--;
    Serial.printf("[WiFiServer] Client disconnected. Total: %d\n", g_serverInstance->_clientCount);
}
