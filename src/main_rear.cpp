/**
 * Project Nightfall - Rear Controller (ESP32 #2)
 * Listens for motor commands from Front via UART and reports rear telemetry.
 * No WiFi/AP enabled here to avoid AP conflicts; data is proxied by Front.
 */

#include <Arduino.h>
#include <ArduinoJson.h>
#include <esp_task_wdt.h>

#ifndef REAR_CONTROLLER
#define REAR_CONTROLLER 1
#endif
#include "config.h"
#include "pins.h"

#include "MotorControl.h"
#include "UltrasonicSensor.h"
#include "SafetyMonitor.h"
#include "UARTComm.h"

// Rear controller objects
MotorControl rearMotors(
    MOTOR_RL_ENA, MOTOR_RL_IN1, MOTOR_RL_IN2,
    MOTOR_RR_ENB, MOTOR_RR_IN3, MOTOR_RR_IN4,
    PWM_CHANNEL_RL, PWM_CHANNEL_RR);

UltrasonicSensor rearSensor(US_REAR_TRIG, US_REAR_ECHO);
SafetyMonitor safetyMonitor;
UARTComm masterComm(Serial1, UART_BAUD_RATE);

// State
RobotState currentState = STATE_INIT;
MovementCommand lastCommand = CMD_STOP;
float lastRearDistance = 0.0f;
bool emergencyStopTriggered = false;

// Timing
unsigned long lastHeartbeatReceived = 0;
unsigned long lastSafetyCheck = 0;
unsigned long lastSensorPush = 0;
unsigned long loopCounter = 0;

#ifdef SERIAL_DEBUG
#define DEBUG_PRINT(x) Serial.print(x)
#define DEBUG_PRINTLN(x) Serial.println(x)
#else
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#endif

static void setupWatchdog()
{
    esp_task_wdt_init(WATCHDOG_TIMEOUT / 1000, true);
    esp_task_wdt_add(NULL);
}

static void resetWatchdog()
{
    esp_task_wdt_reset();
}

static void executeMotorCommand(MovementCommand cmd)
{
    switch (cmd)
    {
    case CMD_FORWARD:
        currentState = STATE_MANUAL;
        rearMotors.forward(MOTOR_NORMAL_SPEED);
        break;
    case CMD_BACKWARD:
        currentState = STATE_MANUAL;
        rearMotors.backward(MOTOR_NORMAL_SPEED);
        break;
    case CMD_TURN_LEFT:
        currentState = STATE_TURNING;
        rearMotors.turnLeft(MOTOR_TURN_SPEED);
        break;
    case CMD_TURN_RIGHT:
        currentState = STATE_TURNING;
        rearMotors.turnRight(MOTOR_TURN_SPEED);
        break;
    case CMD_ROTATE_360:
        currentState = STATE_TURNING;
        rearMotors.rotate360(true);
        break;
    case CMD_CLIMB_BOOST:
        currentState = STATE_CLIMBING;
        rearMotors.forward(MOTOR_CLIMB_SPEED);
        break;
    case CMD_STOP:
    default:
        currentState = STATE_IDLE;
        rearMotors.stop();
        break;
    }
    lastCommand = cmd;
}

static MovementCommand mapStringToCommand(const char *cmd)
{
    if (!cmd)
        return CMD_STOP;
    if (!strcmp(cmd, "forward"))
        return CMD_FORWARD;
    if (!strcmp(cmd, "backward") || !strcmp(cmd, "back"))
        return CMD_BACKWARD;
    if (!strcmp(cmd, "left"))
        return CMD_TURN_LEFT;
    if (!strcmp(cmd, "right"))
        return CMD_TURN_RIGHT;
    if (!strcmp(cmd, "rotate_360"))
        return CMD_ROTATE_360;
    if (!strcmp(cmd, "climb_boost"))
        return CMD_CLIMB_BOOST;
    if (!strcmp(cmd, "stop"))
        return CMD_STOP;
    return CMD_STOP;
}

static void receiveMasterCommands()
{
    StaticJsonDocument<512> doc = masterComm.receiveMessage();
    if (doc.size() == 0)
        return;

    const char *type = doc["type"] | "";
    if (strcmp(type, "heartbeat") == 0)
    {
        lastHeartbeatReceived = millis();
        return;
    }

    if (strcmp(type, "motor_command") == 0)
    {
        const char *cmdStr = doc["cmd"] | "";
        MovementCommand cmd = mapStringToCommand(cmdStr);
        executeMotorCommand(cmd);
        return;
    }
}

static void sendRearStatusToMaster()
{
    StaticJsonDocument<256> status;
    status["type"] = "sensor_update";
    status["source"] = "rear";
    status["timestamp"] = millis();
    status["data"]["rear_distance"] = lastRearDistance;
    status["data"]["robot_state"] = (int)currentState;
    masterComm.sendMessage(status);
}

static void handleEmergencyStop()
{
    safetyMonitor.triggerEmergencyStop();
    rearMotors.stop();
    currentState = STATE_EMERGENCY;
    emergencyStopTriggered = true;
}

void setup()
{
    Serial.begin(115200);
    delay(800);

    setupWatchdog();

    pinMode(LED_BUILTIN, OUTPUT);
    digitalWrite(LED_BUILTIN, LOW);

    DEBUG_PRINTLN("\nPROJECT NIGHTFALL - REAR CONTROLLER STARTUP\n");

    rearMotors.begin();
    rearSensor.begin();
    safetyMonitor.begin();
    // Initialize hardware UART with proper pins
    Serial1.begin(UART_BAUD_RATE, SERIAL_8N1, UART_MASTER_RX, UART_MASTER_TX);
    masterComm.begin();

    currentState = STATE_IDLE;
    emergencyStopTriggered = false;

    digitalWrite(LED_BUILTIN, HIGH);
    delay(300);
    digitalWrite(LED_BUILTIN, LOW);
}

void loop()
{
    resetWatchdog();
    unsigned long now = millis();

    // Update rear distance @ 10Hz
    if (now - lastSensorPush >= 100)
    {
        lastSensorPush = now;
        lastRearDistance = rearSensor.getAverageDistance(3);
        safetyMonitor.checkCollisionRisk(0.0f, lastRearDistance);
        sendRearStatusToMaster();
    }

    // Safety @ 5Hz
    if (now - lastSafetyCheck >= 200)
    {
        lastSafetyCheck = now;
        safetyMonitor.update();
        if (!safetyMonitor.isSafe() || safetyMonitor.isEmergency())
        {
            if (!emergencyStopTriggered)
            {
                handleEmergencyStop();
            }
        }
        else if (emergencyStopTriggered && safetyMonitor.isSafe())
        {
            emergencyStopTriggered = false;
            safetyMonitor.resetEmergencyStop();
            currentState = STATE_IDLE;
        }
    }

    // Receive commands from front
    receiveMasterCommands();

    loopCounter++;
}
