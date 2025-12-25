/**
 * Project Nightfall - ESP32-CAM Vision Module
 *
 * Responsibilities:
 * - Camera initialization and configuration
 * - MJPEG streaming over WiFi
 * - UART communication with front master controller
 * - Heartbeat and status monitoring
 * - Command reception for flash control and frame capture
 * - Frame rate monitoring and FPS reporting
 *
 * WiFi Access Point Mode:
 * - SSID: ProjectNightfall
 * - Password: rescue2025
 * - Stream URL: http://192.168.4.1:81/stream
 *
 * Communication Protocol:
 * - JSON-based messages over UART with front controller
 * - Status updates, heartbeats, and commands
 */

#include <Arduino.h>
#include <WiFi.h>
#include <esp_camera.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>
#include <SD_MMC.h>

// ============================================
// CONFIGURATION
// ============================================
#define SERIAL_DEBUG 1  // Enable serial debugging
#define CAMERA_MODULE 1 // Camera module identifier

#include "config.h"
#include "pins.h"

// ============================================
// CAMERA CONFIGURATION
// ============================================

// Camera model configuration (OV2640 typical for ESP32-CAM)
#define CAMERA_MODEL_AITEST // Alternative: AI_THINKER, WROVER, WROVER_1MB, etc.

#ifdef CAMERA_MODEL_AITEST
// AI-THINKER configuration (most common)
#define PWDN_GPIO_NUM 32
#define RESET_GPIO_NUM -1
#define XCLK_GPIO_NUM 0
#define SIOD_GPIO_NUM 26
#define SIOC_GPIO_NUM 27

#define Y9_GPIO_NUM 35
#define Y8_GPIO_NUM 34
#define Y7_GPIO_NUM 39
#define Y6_GPIO_NUM 36
#define Y5_GPIO_NUM 21
#define Y4_GPIO_NUM 19
#define Y3_GPIO_NUM 18
#define Y2_GPIO_NUM 5
#define VSYNC_GPIO_NUM 25
#define HREF_GPIO_NUM 23
#define PCLK_GPIO_NUM 22
#endif

// ============================================
// GLOBAL STATE VARIABLES
// ============================================

// Camera state
bool cameraInitialized = false;
bool wifiConnected = false;
bool streaming = false;

// SD Card state
bool sdCardInitialized = false;
uint64_t sdCardUsedBytes = 0;
uint64_t sdCardMaxBytes = SD_MAX_SIZE_BYTES;

// Timing and monitoring
unsigned long lastHeartbeat = 0;
unsigned long lastFrameTime = 0;
unsigned long lastFPS = 0;
unsigned long cameraInitTime = 0;
unsigned long lastSDCheck = 0;
int frameCount = 0;
int fpsCounter = 0;

// Communication
HardwareSerial masterSerial(1);
WiFiServer streamServer(CAMERA_STREAM_PORT);
WiFiClient streamClient;

// Frame buffer and streaming
camera_fb_t *fb = nullptr;
static const size_t CHUNK_SIZE = 8192; // Streaming chunk size

// Status tracking
struct CameraStatus
{
    bool initialized;
    bool streaming;
    int connectedClients;
    int totalFramesStreamed;
    int droppedFrames;
    float avgFrameTime;
    unsigned long uptimeSeconds;
} cameraStatus;

// ============================================
// FUNCTION DECLARATIONS
// ============================================

void initializeCamera();
void initializeSDCard();
void checkSDCardSpace();
void initializeWiFi();
void handleCameraStream();
void sendStatusUpdate();
void receiveCommands();
void handleHeartbeat();
void processCameraCommand(const JsonDocument &cmd);
void setupWatchdog();
void resetWatchdog();
void logCameraStatus();

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(115200);
    delay(2000); // Extended delay for serial initialization

    DEBUG_PRINTLN("\n=====================================");
    DEBUG_PRINTLN("PROJECT NIGHTFALL - ESP32-CAM INIT");
    DEBUG_PRINTLN("=====================================\n");

    // Initialize watchdog
    setupWatchdog();

    // Initialize built-in LED
    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    // Initialize flash LED
    pinMode(FLASH_LED, OUTPUT);
    digitalWrite(FLASH_LED, LOW);

    // Initialize camera
    DEBUG_PRINTLN(">>> Initializing ESP32-CAM...");
    initializeCamera();

    if (!cameraInitialized)
    {
        DEBUG_PRINTLN("❌ Camera initialization failed!");
        while (true)
        {
            digitalWrite(LED_BUILTIN, HIGH);
            delay(200);
            digitalWrite(LED_BUILTIN, LOW);
            delay(200);
        }
    }

    // Initialize SD Card
    DEBUG_PRINTLN(">>> Initializing microSD Card...");
    initializeSDCard();

    // Initialize WiFi AP
    DEBUG_PRINTLN(">>> Initializing WiFi Access Point...");
    initializeWiFi();

    // Initialize UART to Master Controller
    DEBUG_PRINTLN(">>> Initializing UART to Master Controller...");
    masterSerial.begin(UART_BAUD_RATE, SERIAL_8N1, UART_MASTER_RX, UART_MASTER_TX);
    delay(100);

    // Start streaming server
    DEBUG_PRINTLN(">>> Starting streaming server...");
    streamServer.begin();
    streamServer.setNoDelay(true);

    // Initialize camera status
    cameraStatus.initialized = true;
    cameraStatus.streaming = false;
    cameraStatus.connectedClients = 0;
    cameraStatus.totalFramesStreamed = 0;
    cameraStatus.droppedFrames = 0;
    cameraStatus.avgFrameTime = 0;
    cameraStatus.uptimeSeconds = 0;

    // Signal ready
    digitalWrite(LED_BUILTIN, HIGH);
    delay(500);
    digitalWrite(LED_BUILTIN, LOW);

    cameraInitTime = millis();
    lastHeartbeat = millis();
    lastFrameTime = millis();

    DEBUG_PRINTLN("\n✓ ESP32-CAM Ready for streaming\n");
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
    resetWatchdog();

    // Handle camera streaming to connected clients
    if (cameraInitialized && wifiConnected)
    {
        handleCameraStream();
    }

    // Receive and process commands from master
    receiveCommands();

    // Check SD card space (every 5 seconds)
    unsigned long currentTime = millis();
    if (currentTime - lastSDCheck >= 5000)
    {
        lastSDCheck = currentTime;
        if (sdCardInitialized)
        {
            checkSDCardSpace();
        }
    }

    // Send status updates (5Hz)
    if (currentTime - lastHeartbeat >= 200)
    {
        lastHeartbeat = currentTime;
        sendStatusUpdate();
    }

    // Calculate and display FPS (1Hz)
    if (currentTime - lastFrameTime >= 1000)
    {
        lastFPS = frameCount;
        frameCount = 0;
        lastFrameTime = currentTime;

        cameraStatus.uptimeSeconds = (currentTime - cameraInitTime) / 1000;

        DEBUG_PRINT("📊 FPS: ");
        DEBUG_PRINT(lastFPS);
        DEBUG_PRINT(" | Clients: ");
        DEBUG_PRINT(cameraStatus.connectedClients);
        DEBUG_PRINT(" | Frames: ");
        DEBUG_PRINT(cameraStatus.totalFramesStreamed);
        DEBUG_PRINT(" | Uptime: ");
        DEBUG_PRINT(cameraStatus.uptimeSeconds);
        DEBUG_PRINTLN("s");
    }
}

// ============================================
// CAMERA INITIALIZATION
// ============================================

void initializeCamera()
{
    DEBUG_PRINTLN("  [CAMERA] Setting up camera configuration...");

    // Camera configuration structure
    camera_config_t config;

    // Pin mapping
    config.pin_pwdn = PWDN_GPIO_NUM;
    config.pin_reset = RESET_GPIO_NUM;
    config.pin_xclk = XCLK_GPIO_NUM;
    config.pin_sccb_sda = SIOD_GPIO_NUM;
    config.pin_sccb_scl = SIOC_GPIO_NUM;

    // Data pins
    config.pin_d7 = Y9_GPIO_NUM;
    config.pin_d6 = Y8_GPIO_NUM;
    config.pin_d5 = Y7_GPIO_NUM;
    config.pin_d4 = Y6_GPIO_NUM;
    config.pin_d3 = Y5_GPIO_NUM;
    config.pin_d2 = Y4_GPIO_NUM;
    config.pin_d1 = Y3_GPIO_NUM;
    config.pin_d0 = Y2_GPIO_NUM;

    // Sync pins
    config.pin_vsync = VSYNC_GPIO_NUM;
    config.pin_href = HREF_GPIO_NUM;
    config.pin_pclk = PCLK_GPIO_NUM;

    // Clock configuration
    config.xclk_freq_hz = 20000000; // 20MHz clock
    config.ledc_timer = LEDC_TIMER_0;
    config.ledc_channel = LEDC_CHANNEL_0;

    // Image format configuration
    config.pixel_format = PIXFORMAT_JPEG;
    config.frame_size = CAMERA_FRAME_SIZE;
    config.jpeg_quality = CAMERA_JPEG_QUALITY;
    config.fb_count = CAMERA_FB_COUNT;
    config.grab_mode = CAMERA_GRAB_LATEST;

    DEBUG_PRINTLN("  [CAMERA] Initializing ESP camera...");
    esp_err_t err = esp_camera_init(&config);

    if (err != ESP_OK)
    {
        DEBUG_PRINT("  ❌ Camera init failed with error code: 0x");
        // Use Serial directly to allow HEX formatting without macro limits
        Serial.println(err, HEX);
        cameraInitialized = false;
        return;
    }

    DEBUG_PRINTLN("  [CAMERA] Getting camera sensor...");
    sensor_t *s = esp_camera_sensor_get();

    if (s == NULL)
    {
        DEBUG_PRINTLN("  ❌ Camera sensor not found!");
        cameraInitialized = false;
        return;
    }

    // Configure sensor for optimal image quality
    DEBUG_PRINTLN("  [CAMERA] Configuring sensor parameters...");

    // Brightness and contrast (lift image for low-light)
    s->set_brightness(s, 1); // -2 to 2
    s->set_contrast(s, 1);   // -2 to 2
    s->set_saturation(s, 0); // -2 to 2

    // Effects and white balance
    s->set_special_effect(s, 0); // 0 = No Effect, 4 = B&W
    s->set_whitebal(s, 1);       // Auto white balance
    s->set_awb_gain(s, 1);       // Auto WB gain
    s->set_wb_mode(s, 0);        // 0-4 white balance modes

    // Exposure control
    s->set_exposure_ctrl(s, 1); // Auto exposure
    s->set_aec2(s, 1);          // Enable improved AEC for low-light
    s->set_ae_level(s, 1);      // -2 to 2 (slightly brighter)
    s->set_aec_value(s, 600);   // 0 to 1200 (target exposure)

    // Gain control
    s->set_gain_ctrl(s, 1);                 // Auto gain
    s->set_agc_gain(s, 0);                  // Auto controlled when gain_ctrl=1
    s->set_gainceiling(s, GAINCEILING_32X); // Allow higher gain for low-light

    // Image processing
    s->set_bpc(s, 0);     // Black pixel correction
    s->set_wpc(s, 1);     // White pixel correction
    s->set_raw_gma(s, 1); // RAW gamma correction
    s->set_lenc(s, 1);    // Lens distortion correction

    // Flip and mirror
    s->set_hmirror(s, 1); // Horizontal mirror (1 = flip to fix mirrored image)
    s->set_vflip(s, 0);   // Vertical flip

    // Quality
    s->set_dcw(s, 1);      // Downsize enable
    s->set_colorbar(s, 0); // Color bar pattern OFF

    cameraInitialized = true;
    DEBUG_PRINTLN("  ✓ Camera initialized successfully");
}

// ============================================
// WiFi INITIALIZATION
// ============================================

void initializeWiFi()
{
    DEBUG_PRINTLN("  [WIFI] Starting WiFi Access Point mode...");

    // Configure as WiFi Access Point (hotspot)
    WiFi.mode(WIFI_AP);

    bool apStarted = WiFi.softAP(WIFI_SSID, WIFI_PASSWORD);

    if (!apStarted)
    {
        DEBUG_PRINTLN("  ❌ Failed to start WiFi AP!");
        wifiConnected = false;
        return;
    }

    // Get and display AP IP address
    IPAddress apIP = WiFi.softAPIP();
    DEBUG_PRINT("  ✓ AP started - IP: ");
    DEBUG_PRINTLN(apIP);

    // Display connection details
    DEBUG_PRINT("\n  📱 WiFi Details:");
    DEBUG_PRINT("\n     SSID: ");
    DEBUG_PRINT(WIFI_SSID);
    DEBUG_PRINT("\n     Password: ");
    DEBUG_PRINT(WIFI_PASSWORD);
    DEBUG_PRINT("\n     Stream URL: http://");
    DEBUG_PRINT(apIP);
    DEBUG_PRINT(":");
    DEBUG_PRINT(CAMERA_STREAM_PORT);
    DEBUG_PRINTLN("/stream");
    DEBUG_PRINT("\n     Connected clients: ");
    DEBUG_PRINT(WiFi.softAPgetStationNum());
    DEBUG_PRINTLN("\n");

    wifiConnected = true;
}

// ============================================
// CAMERA STREAMING
// ============================================

void handleCameraStream()
{
    // Check for new clients connecting to streaming port
    if (!streamClient || !streamClient.connected())
    {
        streamClient = streamServer.available();

        if (streamClient)
        {
            cameraStatus.connectedClients++;
            DEBUG_PRINTLN("\n[STREAM] New client connected");
            streaming = true;

            // Send HTTP header for MJPEG streaming
            streamClient.println("HTTP/1.1 200 OK");
            streamClient.println("Content-Type: multipart/x-mixed-replace; boundary=frame");
            streamClient.println("Connection: keep-alive");
            streamClient.println();
        }
    }

    // Stream frames to connected client
    if (streamClient && streamClient.connected() && streaming)
    {
        // Capture frame from camera
        fb = esp_camera_fb_get();

        if (!fb)
        {
            cameraStatus.droppedFrames++;
            DEBUG_PRINTLN("[STREAM] Frame capture failed!");
            return;
        }

        // Prepare boundary
        streamClient.println("--frame");
        streamClient.println("Content-Type: image/jpeg");
        streamClient.print("Content-Length: ");
        streamClient.println(fb->len);
        streamClient.println();

        // Send frame data
        size_t sent = streamClient.write(fb->buf, fb->len);
        if (sent != fb->len)
        {
            cameraStatus.droppedFrames++;
            DEBUG_PRINT("[STREAM] Incomplete frame sent: ");
            DEBUG_PRINT(sent);
            DEBUG_PRINT("/");
            DEBUG_PRINTLN(fb->len);
        }

        // Frame boundary
        streamClient.println();

        // Release frame buffer
        esp_camera_fb_return(fb);
        fb = nullptr;

        // Update statistics
        frameCount++;
        cameraStatus.totalFramesStreamed++;
    }
    else if (streamClient && !streamClient.connected())
    {
        streaming = false;
        cameraStatus.connectedClients = max(0, cameraStatus.connectedClients - 1);
        DEBUG_PRINTLN("[STREAM] Client disconnected");
    }
}

// ============================================
// COMMUNICATION WITH MASTER CONTROLLER
// ============================================

void sendStatusUpdate()
{
    // Create status message with camera information
    StaticJsonDocument<512> statusMsg;

    statusMsg["type"] = "status";
    statusMsg["source"] = "camera";
    statusMsg["timestamp"] = millis();

    // Camera status data
    statusMsg["data"]["initialized"] = cameraStatus.initialized;
    statusMsg["data"]["streaming"] = streaming;
    statusMsg["data"]["connected_clients"] = cameraStatus.connectedClients;
    statusMsg["data"]["fps"] = lastFPS;
    statusMsg["data"]["total_frames"] = cameraStatus.totalFramesStreamed;
    statusMsg["data"]["dropped_frames"] = cameraStatus.droppedFrames;
    statusMsg["data"]["uptime"] = cameraStatus.uptimeSeconds;

    // System health indicators
    statusMsg["data"]["wifi_connected"] = wifiConnected;
    statusMsg["data"]["camera_working"] = cameraInitialized;

    // Serialize and send to master
    String output;
    serializeJson(statusMsg, output);
    masterSerial.println(output);
}

void receiveCommands()
{
    // Check if data available on UART from master controller
    if (masterSerial.available() > 0)
    {
        String message = "";

        // Read until newline
        while (masterSerial.available())
        {
            char c = masterSerial.read();
            if (c == '\n')
                break;
            if (c != '\r')
                message += c;
        }

        if (message.length() > 0)
        {
            DEBUG_PRINT("[COMMAND] Received: ");
            DEBUG_PRINTLN(message);

            // Parse JSON command
            StaticJsonDocument<512> doc;
            DeserializationError error = deserializeJson(doc, message);

            if (error)
            {
                DEBUG_PRINT("[COMMAND] JSON parse error: ");
                DEBUG_PRINTLN(error.c_str());
                return;
            }

            // Process command
            processCameraCommand(doc);
        }
    }
}

void processCameraCommand(const JsonDocument &cmd)
{
    const char *type = cmd["type"];

    if (!type)
    {
        DEBUG_PRINTLN("[COMMAND] No type field in message");
        return;
    }

    // Handle different command types
    if (strcmp(type, "command") == 0)
    {
        const char *cmdStr = cmd["data"]["cmd"];

        if (!cmdStr)
        {
            return;
        }

        if (strcmp(cmdStr, "flash_on") == 0)
        {
            digitalWrite(FLASH_LED, HIGH);
            DEBUG_PRINTLN("[COMMAND] ✓ Flash LED turned ON");
        }
        else if (strcmp(cmdStr, "flash_off") == 0)
        {
            digitalWrite(FLASH_LED, LOW);
            DEBUG_PRINTLN("[COMMAND] ✓ Flash LED turned OFF");
        }
        else if (strcmp(cmdStr, "capture") == 0)
        {
            // Capture single high-quality frame
            DEBUG_PRINTLN("[COMMAND] Capturing frame...");
            fb = esp_camera_fb_get();
            if (fb)
            {
                DEBUG_PRINT("[COMMAND] Frame captured: ");
                DEBUG_PRINT(fb->len);
                DEBUG_PRINTLN(" bytes");
                esp_camera_fb_return(fb);
                fb = nullptr;
            }
            else
            {
                DEBUG_PRINTLN("[COMMAND] ❌ Frame capture failed");
            }
        }
        else if (strcmp(cmdStr, "status") == 0)
        {
            logCameraStatus();
        }
    }
    else if (strcmp(type, "heartbeat") == 0)
    {
        // Acknowledge master heartbeat
        handleHeartbeat();

        // Optional: log distance data if present
        if (cmd["data"].containsKey("front_distance"))
        {
            float fd = cmd["data"]["front_distance"].as<float>();
            float rd = cmd["data"]["rear_distance"].as<float>();
            DEBUG_PRINT("[CAM] Heartbeat distances - Front: ");
            DEBUG_PRINT(fd);
            DEBUG_PRINT(" cm, Rear: ");
            DEBUG_PRINT(rd);
            DEBUG_PRINTLN(" cm");
        }
    }
    else if (strcmp(type, "sensor_data") == 0)
    {
        handleSensorUpdate(cmd);
    }
}

void handleSensorUpdate(const JsonDocument &sensorMsg)
{
    float frontDist = sensorMsg["data"]["front_distance"] | -1.0f;
    float rearDist = sensorMsg["data"]["rear_distance"] | -1.0f;
    bool obstacleDetected = sensorMsg["data"]["obstacle_detected"] | false;
    bool emergencyTriggered = sensorMsg["data"]["emergency_triggered"] | false;
    int gasLevel = sensorMsg["data"]["gas_level"] | 0;

    DEBUG_PRINT("[CAM] Sensor Update - Front: ");
    DEBUG_PRINT(frontDist);
    DEBUG_PRINT(" cm, Rear: ");
    DEBUG_PRINT(rearDist);
    DEBUG_PRINT(" cm, Gas: ");
    DEBUG_PRINT(gasLevel);
    DEBUG_PRINTLN();

    if (obstacleDetected)
    {
        DEBUG_PRINT("[CAM] ⚠️ Obstacle detected at ");
        DEBUG_PRINT(frontDist);
        DEBUG_PRINTLN(" cm");
    }

    if (emergencyTriggered)
    {
        DEBUG_PRINTLN("[CAM] 🚨 EMERGENCY: Distance threshold breached!");
    }
}

void handleHeartbeat()
{
    // Send heartbeat acknowledgement back to master
    StaticJsonDocument<256> ack;

    ack["type"] = "heartbeat_ack";
    ack["source"] = "camera";
    ack["timestamp"] = millis();
    ack["data"]["streaming"] = streaming;
    ack["data"]["fps"] = lastFPS;
    ack["data"]["uptime"] = cameraStatus.uptimeSeconds;

    String output;
    serializeJson(ack, output);
    masterSerial.println(output);
}

// ============================================
// UTILITY FUNCTIONS
// ============================================

void setupWatchdog()
{
    // Initialize ESP32 watchdog timer
    esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
    esp_task_wdt_add(NULL);

    DEBUG_PRINTLN("  [SYSTEM] Watchdog timer initialized");
}

void resetWatchdog()
{
    // Feed the watchdog to prevent reset
    esp_task_wdt_reset();
}

void logCameraStatus()
{
    DEBUG_PRINTLN("\n========== CAMERA MODULE STATUS ==========");
    DEBUG_PRINT("Uptime: ");
    DEBUG_PRINT(cameraStatus.uptimeSeconds);
    DEBUG_PRINTLN(" seconds");
    DEBUG_PRINT("Initialized: ");
    DEBUG_PRINTLN(cameraInitialized ? "YES" : "NO");
    DEBUG_PRINT("WiFi Connected: ");
    DEBUG_PRINTLN(wifiConnected ? "YES" : "NO");
    DEBUG_PRINT("Streaming: ");
    DEBUG_PRINTLN(streaming ? "YES" : "NO");
    DEBUG_PRINT("Connected Clients: ");
    DEBUG_PRINTLN(cameraStatus.connectedClients);
    DEBUG_PRINT("Current FPS: ");
    DEBUG_PRINTLN(lastFPS);
    DEBUG_PRINT("Total Frames: ");
    DEBUG_PRINTLN(cameraStatus.totalFramesStreamed);
    DEBUG_PRINT("Dropped Frames: ");
    DEBUG_PRINTLN(cameraStatus.droppedFrames);
    if (cameraStatus.totalFramesStreamed > 0)
    {
        float dropRate = (float)cameraStatus.droppedFrames / cameraStatus.totalFramesStreamed * 100;
        DEBUG_PRINT("Drop Rate: ");
        DEBUG_PRINT(dropRate);
        DEBUG_PRINTLN("%");
    }
    DEBUG_PRINTLN("==========================================\n");
}
// ============================================
// SD CARD MANAGEMENT
// ============================================

void initializeSDCard()
{
#if SD_CARD_ENABLED

    DEBUG_PRINTLN("  [SD] Mounting SD card (1-bit mode)...");

    // Mount SD card using SD_MMC in 1-bit mode
    // true = 1-bit mode (frees pins 4, 12, 13 for other use)
    // Uses only: CMD=15, CLK=14, DATA0=2
    if (!SD_MMC.begin("/sdcard", true))
    {
        DEBUG_PRINTLN("  ❌ SD card mount failed!");
        sdCardInitialized = false;
        return;
    }

    DEBUG_PRINTLN("  [SD] ✓ SD card mounted successfully");

    // Check card size
    uint64_t cardSize = SD_MMC.cardSize();
    DEBUG_PRINT("  [SD] Physical card size: ");
    DEBUG_PRINT(cardSize / (1024 * 1024 * 1024));
    DEBUG_PRINTLN(" GB");

    // Check available space
    uint64_t cardFreeSpace = SD_MMC.totalBytes() - SD_MMC.usedBytes();
    DEBUG_PRINT("  [SD] Total space: ");
    DEBUG_PRINT(SD_MMC.totalBytes() / (1024 * 1024 * 1024));
    DEBUG_PRINTLN(" GB");

    DEBUG_PRINT("  [SD] Free space: ");
    DEBUG_PRINT(cardFreeSpace / (1024 * 1024 * 1024));
    DEBUG_PRINTLN(" GB");

    DEBUG_PRINT("  [SD] Usable limit set to: ");
    DEBUG_PRINT(SD_MAX_SIZE_GB);
    DEBUG_PRINTLN(" GB");

    // Create storage directory if it doesn't exist
    if (!SD_MMC.exists("/storage"))
    {
        DEBUG_PRINTLN("  [SD] Creating /storage directory...");
        SD_MMC.mkdir("/storage");
    }

    sdCardInitialized = true;
    sdCardUsedBytes = 0;

    DEBUG_PRINTLN("  [SD] SD card ready for use (8GB full capacity)");

#else
    DEBUG_PRINTLN("  [SD] SD card support disabled in config");
    sdCardInitialized = false;
#endif
}

void checkSDCardSpace()
{
#if SD_CARD_ENABLED

    if (!sdCardInitialized)
        return;

    // Get current used space
    sdCardUsedBytes = SD_MMC.usedBytes();

    // Check if approaching capacity limit
    if (sdCardUsedBytes >= sdCardMaxBytes)
    {
        DEBUG_PRINTLN("\n⚠️⚠️⚠️ SD CARD: STORAGE LIMIT REACHED ⚠️⚠️⚠️");
        DEBUG_PRINTLN("Storage full - cannot write more data");
        DEBUG_PRINTLN("Please backup and clear SD card\n");

        // Send alert to master controller
        StaticJsonDocument<256> alert;
        alert["type"] = "alert";
        alert["source"] = "camera";
        alert["timestamp"] = millis();
        alert["data"]["alert_type"] = "storage_full";
        alert["data"]["used_bytes"] = sdCardUsedBytes;
        alert["data"]["max_bytes"] = sdCardMaxBytes;

        String output;
        serializeJson(alert, output);
        masterSerial.println(output);

        return;
    }

    // Check if 80% full
    if (sdCardUsedBytes >= (sdCardMaxBytes * 0.8))
    {
        float percentUsed = (float)sdCardUsedBytes / sdCardMaxBytes * 100;
        DEBUG_PRINT("[SD] ⚠️ Storage warning: ");
        DEBUG_PRINT(percentUsed);
        DEBUG_PRINTLN("% full");
    }

#endif
}