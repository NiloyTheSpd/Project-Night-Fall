/**
 * Project Nightfall - Back ESP32 (Master Brain)
 *
 * Responsibilities:
 * - Sensor acquisition (2x ultrasonic, gas sensor)
 * - Safety monitoring & hazard detection
 * - Obstacle avoidance & auto-climb logic
 * - Autonomous navigation
 * - Rear motor control (L298N direct)
 * - Front motor command distribution (via WebSocket)
 * - Telemetry broadcast (via WebSocket)
 * - WiFi Access Point & WebSocket Server
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#define BACK_CONTROLLER
#include "config.h"
#include "pins.h"

#include "L298N.h"
#include "UltrasonicSensor.h"
#include "MQ2Sensor.h"
#include "WiFiManager.h"
#include "MessageProtocol.h"
#include "Autonomy.h"
#include "SafetyManager.h"
#include "SensorManager.h"
#include "StateMachine.h"

// ============================================
// GLOBAL OBJECTS
// ============================================

// Motors
L298N rearMotors(
    MOTOR_REAR_LEFT_ENA, MOTOR_REAR_LEFT_IN1, MOTOR_REAR_LEFT_IN2,
    MOTOR_REAR_RIGHT_ENB, MOTOR_REAR_RIGHT_IN3, MOTOR_REAR_RIGHT_IN4,
    PWM_CHANNEL_REAR_LEFT, PWM_CHANNEL_REAR_RIGHT);

// Modules
Autonomy autonomyModule;
SafetyManager safetyManager;
SensorManager sensorManager;
StateMachine fsm;

// WebSocket Server (AP + Server)
WSServer_Manager wsServer(WIFI_SERVER_PORT);

// ============================================
// STATE VARIABLES
// ============================================

NavigationState navState = NAV_FORWARD;

// Motor speeds
int rearLeftSpeed = 0;
int rearRightSpeed = 0;
int frontLeftSpeed = 0;
int frontRightSpeed = 0;

// Timing
unsigned long lastNavUpdate = 0;
unsigned long lastTelemetryBroadcast = 0;
uint16_t g_lastLoopTimeUs = 0;  // Phase 2.5: Loop timing for telemetry

// ============================================
// FUNCTION DECLARATIONS
// ============================================

void initMotors();
void initComms();
void updateAutonomousNav();
void broadcastTelemetry();
void sendMotorCommandToFront(int leftSpeed, int rightSpeed);
void sendMotorCommandToFront(int leftSpeed, int rightSpeed);
void handleWebSocketMessage(const JsonDocument &doc, AsyncWebSocketClient *client);

// ============================================
// SETUP
// ============================================

void setup()
{
    Serial.begin(SERIAL_BAUD_RATE);
    delay(500);

    DEBUG_PRINTLN("\n\n=== PROJECT NIGHTFALL - BACK ESP32 (MASTER) ===");
    DEBUG_PRINTLN("Initializing...");

    // Initialize components
    initMotors();
    sensorManager.begin();
    initComms();

    fsm.setIdle();
    DEBUG_PRINTLN("INIT COMPLETE - Ready for connections");

    esp_task_wdt_init(WATCHDOG_TIMEOUT_MS / 1000, true);
    esp_task_wdt_add(NULL);
}

// ============================================
// MAIN LOOP
// ============================================

void loop()
{
    unsigned long loopStart = millis();
    unsigned long loopStartUs = micros();  // Phase 2.5: Microsecond timing
    esp_task_wdt_reset();

    // WS Server Cleanup (Keep Alive)
    wsServer.update();

    // Update Sensors (Non-blocking internal)
    sensorManager.update();

    // ========================================
    // SAFETY FIRST - Check before any control logic
    // ========================================
    if (!safetyManager.check(sensorManager.getGasLevel(), sensorManager.getFrontDistance()))
    {
        if (!fsm.isEmergency()) 
        {
            // Transition to Emergency
            fsm.triggerEmergency();
            
            rearMotors.stopMotors();
            autonomyModule.reset();
            autonomyModule.setPIDEnabled(false);  // P1 Fix #1: Disable PID during emergency
            sendMotorCommandToFront(0, 0);

            // P0 Fix #12: Use correct hazard type
            const char* hazardType = (safetyManager.getHazardType() == HAZARD_GAS) 
                ? Msg::HAZARD_GAS 
                : Msg::HAZARD_COLLISION;

            StaticJsonDocument<256> alert;
            Msg::buildHazardAlert(alert, hazardType, safetyManager.getHazardDescription().c_str());
            wsServer.broadcast(alert);
            
            DEBUG_PRINTLN("[Safety] Hazard Triggered: " + safetyManager.getHazardDescription());
        }
        
        // Skip navigation/telemetry if in emergency
        // Still track loop time and enforce rate limit
        goto end_loop;
    }

    // ========================================
    // NAVIGATION - Only if safe
    // ========================================
    if (fsm.isAutonomous() && (loopStart - lastNavUpdate >= NAVIGATION_UPDATE_INTERVAL_MS))
    {
        lastNavUpdate = loopStart;
        updateAutonomousNav();
    }

    // Broadcast telemetry
    if (loopStart - lastTelemetryBroadcast >= TELEMETRY_INTERVAL_MS)
    {
        lastTelemetryBroadcast = loopStart;
        broadcastTelemetry();
    }

end_loop:
    // Phase 2.5: Track loop execution time
    unsigned long loopEndUs = micros();
    g_lastLoopTimeUs = (uint16_t)(loopEndUs - loopStartUs);

    // Frame rate limiting
    unsigned long loopDuration = millis() - loopStart;
    if (loopDuration < MAIN_LOOP_RATE_MS)
    {
        delay(MAIN_LOOP_RATE_MS - loopDuration);
    }
}

// ============================================
// INITIALIZATION FUNCTIONS
// ============================================

void initMotors()
{
    DEBUG_PRINTLN("[Motors] Initializing rear L298N driver...");
    rearMotors.begin();
    rearMotors.stopMotors();
}

void initComms()
{
    // Start AP and WebSocket Server
    wsServer.begin();
    
    // Register Callback
    wsServer.setMessageHandler([](const JsonDocument &doc, AsyncWebSocketClient *client)
                                 { handleWebSocketMessage(doc, client); });
}

// ============================================
// AUTONOMOUS NAVIGATION
// ============================================

// ============================================
// AUTONOMOUS NAVIGATION
// ============================================

void updateAutonomousNav()
{
    if (fsm.isEmergency())
    {
        navState = NAV_IDLE;
        autonomyModule.reset();
        rearMotors.stopMotors();
        return;
    }

    // Update Autonomy Module
    autonomyModule.update(sensorManager.getFrontDistance(), sensorManager.getRearDistance());
    
    // Get Results
    navState = autonomyModule.getNavState();
    int leftSpd = autonomyModule.getLeftSpeed();
    int rightSpd = autonomyModule.getRightSpeed();
    
    // Apply to Rear Motors
    rearMotors.setMotors(leftSpd, rightSpd);

    // Sync state to variables for telemetry (redundant but keeps compatibility for now)
    rearLeftSpeed = leftSpd;
    rearRightSpeed = rightSpd;
    frontLeftSpeed = leftSpd;
    frontRightSpeed = rightSpd;

    // Sync to front
    sendMotorCommandToFront(frontLeftSpeed, frontRightSpeed);
}

// ============================================
// COMMUNICATION & LOGIC
// ============================================

void sendMotorCommandToFront(int leftSpeed, int rightSpeed)
{
    StaticJsonDocument<256> doc;
    Msg::MotorCmd cmd;
    cmd.leftSpeed = leftSpeed;
    cmd.rightSpeed = rightSpeed;
    cmd.target = "front";
    
    Msg::buildMotorCmd(doc, cmd);
    wsServer.broadcast(doc); 
}

void broadcastTelemetry()
{
    StaticJsonDocument<1024> doc;  // P2 Fix #9: Increased to 1KB for safety margin
    Msg::TelemetryData data;
    
    // Populate Data
    data.frontDist = sensorManager.getFrontDistance();
    data.rearDist = sensorManager.getRearDistance();
    data.gasLevel = sensorManager.getGasLevel();
    data.frontLeftSpeed = frontLeftSpeed;
    data.frontRightSpeed = frontRightSpeed;
    data.rearLeftSpeed = rearLeftSpeed;
    data.rearRightSpeed = rearRightSpeed;
    data.isAutonomous = fsm.isAutonomous();
    data.navState = autonomyModule.getNavStateName();
    data.clientCount = wsServer.getClientCount();
    
    // Check specific roles
    data.frontOnline = wsServer.isRoleConnected("front");
    data.cameraOnline = wsServer.isRoleConnected("camera");
    
    // Phase 2.5: PID telemetry
    data.pidOutput = autonomyModule.getPIDOutput();
    data.pidError = autonomyModule.getPIDError();
    data.pidSetpoint = autonomyModule.getPIDSetpoint();
    data.pidP = autonomyModule.getPIDProportional();
    data.pidI = autonomyModule.getPIDIntegral();
    data.pidD = autonomyModule.getPIDDerivative();
    
    // Phase 2.5: Loop timing (from global variable set in loop)
    extern uint16_t g_lastLoopTimeUs;
    data.loopTimeUs = g_lastLoopTimeUs;

    // Build & Send
    Msg::buildTelemetry(doc, data);
    
    // P2 Fix #9: Check for JSON overflow before sending
    if (doc.overflowed()) {
        DEBUG_PRINTLN("[TELEMETRY] ERROR: JSON overflow!");
        return;  // Don't broadcast corrupted data
    }
    
    wsServer.broadcast(doc);
}

void handleWebSocketMessage(const JsonDocument &doc, AsyncWebSocketClient *client)
{
    const char *msgType = doc["type"] | "";

    DEBUG_PRINTF("[WS] Received: %s\n", msgType);

    // Filter by type
    if (strcmp(msgType, Msg::TYPE_UI_CMD) == 0)
    {
        const char *cmd = doc["cmd"] | "";

        if (strcmp(cmd, "auto_on") == 0)
        {
            fsm.setAutonomous();
        }
        else if (strcmp(cmd, "auto_off") == 0)
        {
            fsm.setIdle();
            rearMotors.stopMotors();
            sendMotorCommandToFront(0, 0);
            autonomyModule.reset();  // Clear PID integral/state
        }
        else if (strcmp(cmd, "forward") == 0)
        {
            autonomyModule.reset();  // Clear stale PID state
            fsm.setManual();
            rearMotors.setMotorsForward(MOTOR_NORMAL_SPEED);
            sendMotorCommandToFront(MOTOR_NORMAL_SPEED, MOTOR_NORMAL_SPEED);
        }
        else if (strcmp(cmd, "backward") == 0)
        {
            fsm.setManual();
            rearMotors.setMotorsBackward(MOTOR_NORMAL_SPEED);
            sendMotorCommandToFront(-MOTOR_NORMAL_SPEED, -MOTOR_NORMAL_SPEED);
        }
        else if (strcmp(cmd, "left") == 0)
        {
            fsm.setManual();
            // Spin Left: Left Back, Right Forward
            rearMotors.setMotors(-MOTOR_TURN_SPEED, MOTOR_TURN_SPEED); 
            sendMotorCommandToFront(-MOTOR_TURN_SPEED, MOTOR_TURN_SPEED);
        }
        else if (strcmp(cmd, "right") == 0)
        {
            fsm.setManual();
            // Spin Right: Left Forward, Right Back
            rearMotors.setMotors(MOTOR_TURN_SPEED, -MOTOR_TURN_SPEED);
            sendMotorCommandToFront(MOTOR_TURN_SPEED, -MOTOR_TURN_SPEED);
        }
        else if (strcmp(cmd, "stop") == 0)
        {
            fsm.setIdle();
            rearMotors.stopMotors();
            sendMotorCommandToFront(0,0);
        }
        else if (strcmp(cmd, "clear_emergency") == 0)
        {
            // Only allow reset if actually in emergency state
            if (fsm.isEmergency())
            {
                DEBUG_PRINTLN("[SAFETY] Emergency cleared by operator");
                
                // Reset safety manager latch
                safetyManager.reset();
                
                // Reset FSM to idle
                fsm.clearEmergency();
                
                // Re-enable PID for next autonomous run
                autonomyModule.setPIDEnabled(true);
                
                // Ensure motors are stopped (safety)
                rearMotors.stopMotors();
                sendMotorCommandToFront(0, 0);
                
                // Broadcast status update
                StaticJsonDocument<256> alert;
                Msg::buildStatus(alert, Msg::ROLE_BACK, "emergency_cleared", "Operator reset");
                wsServer.broadcast(alert);
            }
        }
        else if (strcmp(cmd, "pid_tune") == 0)
        {
            // Extract with defaults
            float kP = doc["kP"] | 4.0f;
            float kI = doc["kI"] | 0.0f;
            float kD = doc["kD"] | 1.0f;
            
            // SAFETY: Clamp to safe ranges
            kP = constrain(kP, 0.0f, 20.0f);   // Max P prevents oscillation
            kI = constrain(kI, 0.0f, 2.0f);    // Max I prevents overshoot
            kD = constrain(kD, 0.0f, 10.0f);   // Max D prevents noise amplification
            
            autonomyModule.setApproachPID(kP, kI, kD);
            
            // Acknowledge to dashboard
            StaticJsonDocument<128> ack;
            ack["type"] = "pid_ack";
            ack["kP"] = kP;
            ack["kI"] = kI;
            ack["kD"] = kD;
            wsServer.broadcast(ack);
            
            DEBUG_PRINTF("[PID] Tuned: P=%.2f I=%.2f D=%.2f\n", kP, kI, kD);
        }
        else if (strcmp(cmd, "pid_enable") == 0)
        {
            bool enable = doc["enable"] | true;
            autonomyModule.setPIDEnabled(enable);
            DEBUG_PRINTF("[PID] %s\n", enable ? "Enabled" : "Disabled");
        }
    }
}

