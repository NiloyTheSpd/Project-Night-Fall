#ifndef MESSAGE_PROTOCOL_H
#define MESSAGE_PROTOCOL_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Unified JSON Message Protocol for all ESP32 boards
 * Supports: motor commands, sensor updates, telemetry, hazard alerts, UI commands
 */

namespace Msg
{
    // Message types
    static const char *TYPE_MOTOR_CMD = "motor_cmd";
    static const char *TYPE_SENSOR_UPDATE = "sensor_update";
    static const char *TYPE_TELEMETRY = "telemetry";
    static const char *TYPE_HAZARD_ALERT = "hazard_alert";
    static const char *TYPE_UI_CMD = "ui_cmd";
    static const char *TYPE_STATUS = "status";
    static const char *TYPE_PING = "ping";
    static const char *TYPE_ACK = "ack";

    // Board roles
    static const char *ROLE_BACK = "back";
    static const char *ROLE_FRONT = "front";
    static const char *ROLE_CAMERA = "camera";

    // Hazard types
    static const char *HAZARD_GAS = "gas_detected";
    static const char *HAZARD_COLLISION = "collision";
    static const char *HAZARD_TILT = "excessive_tilt";

    /**
     * Build a motor command message
     * @param doc JSON document to populate
     * @param leftSpeed -255 to 255
     * @param rightSpeed -255 to 255
     * @param targetBoard "front" or "back"
     */
    inline void buildMotorCmd(JsonDocument &doc, int leftSpeed, int rightSpeed, const char *targetBoard = "front")
    {
        doc["type"] = TYPE_MOTOR_CMD;
        doc["target"] = targetBoard;
        doc["left"] = leftSpeed;
        doc["right"] = rightSpeed;
        doc["ts"] = millis();
    }

    /**
     * Build a sensor update message
     * @param doc JSON document
     * @param frontDist Front ultrasonic distance (cm)
     * @param rearDist Rear ultrasonic distance (cm)
     * @param gasLevel MQ-2 analog reading (0-4095)
     */
    inline void buildSensorUpdate(JsonDocument &doc, float frontDist, float rearDist, int gasLevel)
    {
        doc["type"] = TYPE_SENSOR_UPDATE;
        doc["from"] = ROLE_BACK;
        doc["front_dist"] = frontDist;
        doc["rear_dist"] = rearDist;
        doc["gas"] = gasLevel;
        doc["ts"] = millis();
    }

    /**
     * Build telemetry message (comprehensive state)
     * @param doc JSON document
     */
    inline void buildTelemetry(JsonDocument &doc,
                               float frontDist, float rearDist, int gasLevel,
                               int frontLeft, int frontRight,
                               int rearLeft, int rearRight,
                               bool autonomousMode, const char *navState)
    {
        doc["type"] = TYPE_TELEMETRY;
        doc["from"] = ROLE_BACK;

        // Sensors
        doc["sensors"]["front_dist"] = frontDist;
        doc["sensors"]["rear_dist"] = rearDist;
        doc["sensors"]["gas"] = gasLevel;

        // Motors
        doc["motors"]["front_left"] = frontLeft;
        doc["motors"]["front_right"] = frontRight;
        doc["motors"]["rear_left"] = rearLeft;
        doc["motors"]["rear_right"] = rearRight;

        // State
        doc["state"]["autonomous"] = autonomousMode;
        doc["state"]["nav_state"] = navState;
        doc["ts"] = millis();
    }

    /**
     * Build hazard alert message
     */
    inline void buildHazardAlert(JsonDocument &doc, const char *hazardType, const char *message, bool critical = true)
    {
        doc["type"] = TYPE_HAZARD_ALERT;
        doc["from"] = ROLE_BACK;
        doc["hazard"] = hazardType;
        doc["message"] = message;
        doc["critical"] = critical;
        doc["ts"] = millis();
    }

    /**
     * Build UI command message
     */
    inline void buildUICmd(JsonDocument &doc, const char *command, const char *parameter = "")
    {
        doc["type"] = TYPE_UI_CMD;
        doc["from"] = ROLE_CAMERA;
        doc["cmd"] = command;
        if (strlen(parameter) > 0)
            doc["param"] = parameter;
        doc["ts"] = millis();
    }

    /**
     * Build status message
     */
    inline void buildStatus(JsonDocument &doc, const char *from, const char *status, const char *details = "")
    {
        doc["type"] = TYPE_STATUS;
        doc["from"] = from;
        doc["status"] = status;
        if (strlen(details) > 0)
            doc["details"] = details;
        doc["ts"] = millis();
    }

    /**
     * Build ping message
     */
    inline void buildPing(JsonDocument &doc, const char *from)
    {
        doc["type"] = TYPE_PING;
        doc["from"] = from;
        doc["ts"] = millis();
    }

    /**
     * Build acknowledgment message
     */
    inline void buildAck(JsonDocument &doc, const char *from, const char *msgType, bool success = true)
    {
        doc["type"] = TYPE_ACK;
        doc["from"] = from;
        doc["ack_type"] = msgType;
        doc["success"] = success;
        doc["ts"] = millis();
    }
}

#endif // MESSAGE_PROTOCOL_H
