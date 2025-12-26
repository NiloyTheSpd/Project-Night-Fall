#include "WiFiManager.h"
#include "MessageProtocol.h"

// ==========================================
// CLIENT MANAGER (Front ESP32 & Camera)
// ==========================================
#ifndef BACK_CONTROLLER

WSClient_Manager *g_wsClientInstance = nullptr;

WSClient_Manager::WSClient_Manager(const char *ssid, const char *password, const char *serverIP, uint16_t serverPort, const char *role)
    : _ssid(ssid), _password(password), _serverIP(serverIP), _serverPort(serverPort), _role(role), _wsConnected(false), _lastWiFiCheck(0)
{
    g_wsClientInstance = this;
}

void WSClient_Manager::begin()
{
    WiFi.mode(WIFI_STA);
    WiFi.begin(_ssid, _password);
    Serial.printf("[WSClient] Connecting to WiFi AP: %s\n", _ssid);

    _webSocket.begin(_serverIP, _serverPort, "/ws");
    
    // Register event handler
    _webSocket.onEvent(onWebSocketEvent);
    
    // Auto reconnect every 5s if lost
    _webSocket.setReconnectInterval(5000); 
}

void WSClient_Manager::update()
{
    unsigned long now = millis();

    // WiFi Reconnect Logic
    if (now - _lastWiFiCheck >= 5000) {
        _lastWiFiCheck = now;
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println("[WSClient] WiFi lost. Reconnecting...");
            WiFi.disconnect(); 
            WiFi.begin(_ssid, _password);
        }
    }

    _webSocket.loop();
}

bool WSClient_Manager::isConnected()
{
    return _wsConnected;
}

bool WSClient_Manager::isWiFiConnected()
{
    return (WiFi.status() == WL_CONNECTED);
}

void WSClient_Manager::sendMessage(const JsonDocument &doc)
{
    String msg;
    serializeJson(doc, msg);
    _webSocket.sendTXT(msg);
}

void WSClient_Manager::setMessageHandler(std::function<void(const JsonDocument &)> handler)
{
    _messageHandler = handler;
}

void WSClient_Manager::onWebSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{
    if (!g_wsClientInstance) return;

    switch(type) {
        case WStype_DISCONNECTED:
            Serial.printf("[WSClient] Disconnected!\n");
            g_wsClientInstance->_wsConnected = false;
            break;
            
        case WStype_CONNECTED:
            Serial.printf("[WSClient] Connected to url: %s\n", payload);
            g_wsClientInstance->_wsConnected = true;
            
            // Send Role Handshake
            {
                StaticJsonDocument<256> doc;
                doc["type"] = "status";
                doc["role"] = g_wsClientInstance->_role;
                doc["status"] = "connected";
                g_wsClientInstance->sendMessage(doc);
            }
            break;
            
        case WStype_TEXT:
            {
                StaticJsonDocument<512> doc;
                DeserializationError error = deserializeJson(doc, payload, length);
                if (!error && g_wsClientInstance->_messageHandler) {
                    g_wsClientInstance->_messageHandler(doc);
                } else if (error) {
                    Serial.print("[WSClient] JSON Error: ");
                    Serial.println(error.c_str());
                }
            }
            break;
            
        case WStype_BIN:
        case WStype_ERROR:
        case WStype_FRAGMENT_TEXT_START:
        case WStype_FRAGMENT_BIN_START:
        case WStype_FRAGMENT:
        case WStype_FRAGMENT_FIN:
            break;
    }
}

#endif

// ==========================================
// SERVER MANAGER (Back ESP32)
// ==========================================
#ifdef BACK_CONTROLLER

WSServer_Manager::WSServer_Manager(uint16_t port)
    : _server(port), _ws("/ws")
{
}

void WSServer_Manager::begin()
{
    // Start AP
    WiFi.mode(WIFI_AP);
    // Config values from config.h should trigger compile error if mismatch, 
    // but here we hardcode or use macros. Assuming config.h has them.
    WiFi.softAP("ProjectNightfall", "rescue2025");
    Serial.println("[WSServer] AP Started: ProjectNightfall");
    Serial.print("[WSServer] IP Address: ");
    Serial.println(WiFi.softAPIP());

    // Setup WebSocket
    _ws.onEvent([this](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
        onEvent(server, client, type, arg, data, len);
    });

    _server.addHandler(&_ws);
    _server.begin();
    Serial.println("[WSServer] TCP/WS Server Started");
}

void WSServer_Manager::update()
{
    _ws.cleanupClients();
}

void WSServer_Manager::broadcast(const JsonDocument &doc)
{
    String msg;
    serializeJson(doc, msg);
    _ws.textAll(msg);
}

void WSServer_Manager::onEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
    if (type == WS_EVT_CONNECT) {
        Serial.printf("[WSServer] Client #%u connected from %s\n", client->id(), client->remoteIP().toString().c_str());
        _clientRoles[client->id()] = "unknown"; // Default
    } 
    else if (type == WS_EVT_DISCONNECT) {
        Serial.printf("[WSServer] Client #%u disconnected\n", client->id());
        _clientRoles.erase(client->id());
    } 
    else if (type == WS_EVT_DATA) {
        handleWebSocketMessage(arg, data, len, client);
    }
}

void WSServer_Manager::handleWebSocketMessage(void *arg, uint8_t *data, size_t len, AsyncWebSocketClient *client)
{
    AwsFrameInfo *info = (AwsFrameInfo*)arg;
    if (info->final && info->index == 0 && info->len == len && info->opcode == WS_TEXT) {
        
        StaticJsonDocument<512> doc;
        DeserializationError error = deserializeJson(doc, data, len);
        
        if (!error) {
            // Check for status/role message
            const char* type = doc["type"] | "";
            if (strcmp(type, "status") == 0) {
               const char* role = doc["role"] | "";
               if (strlen(role) > 0) {
                   _clientRoles[client->id()] = String(role);
                   Serial.printf("[WSServer] Client #%u registered as %s\n", client->id(), role);
               }
            }

            if (_messageHandler) {
                _messageHandler(doc, client);
            }
        } else if (error) {
            Serial.print("[WSServer] JSON Error: ");
            Serial.println(error.c_str());
        }
    }
}

void WSServer_Manager::setMessageHandler(std::function<void(const JsonDocument &, AsyncWebSocketClient *)> handler)
{
    _messageHandler = handler;
}

uint8_t WSServer_Manager::getClientCount()
{
    return _ws.count();
}

String WSServer_Manager::getClientRole(uint32_t id) {
    if (_clientRoles.count(id)) return _clientRoles[id];
    return "unknown";
}

bool WSServer_Manager::isRoleConnected(const char* role) {
    for (auto const& [id, r] : _clientRoles) {
        if (r.equalsIgnoreCase(role)) return true;
    }
    return false;
}

#endif
