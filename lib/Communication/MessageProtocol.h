#ifndef MESSAGE_PROTOCOL_H
#define MESSAGE_PROTOCOL_H

#include <Arduino.h>
#include <ArduinoJson.h>

/**
 * Unified JSON Message Protocol for all ESP32 boards
 * Centralized constants and strong typing via structs.
 */

namespace Msg
{
    // ==========================================
    // CONSTANTS
    // ==========================================
    
    // Message Types
    extern const char *TYPE_MOTOR_CMD;
    extern const char *TYPE_SENSOR_UPDATE;
    extern const char *TYPE_TELEMETRY;
    extern const char *TYPE_HAZARD_ALERT;
    extern const char *TYPE_UI_CMD;
    extern const char *TYPE_STATUS;
    extern const char *TYPE_PING;
    extern const char *TYPE_ACK;

    // Roles
    extern const char *ROLE_BACK;
    extern const char *ROLE_FRONT;
    extern const char *ROLE_CAMERA;

    // Hazards
    extern const char *HAZARD_GAS;
    extern const char *HAZARD_COLLISION;
    extern const char *HAZARD_TILT;

    // ==========================================
    // STRUCTS
    // ==========================================

    struct TelemetryData {
        float frontDist;
        float rearDist;
        int gasLevel;
        int frontLeftSpeed;
        int frontRightSpeed;
        int rearLeftSpeed;
        int rearRightSpeed;
        bool isAutonomous;
        String navState;
        int clientCount;
        bool frontOnline;
        bool cameraOnline;
        
        // Control debug (Phase 2.5)
        float pidOutput;
        float pidError;
        float pidSetpoint;
        float pidP;
        float pidI;
        float pidD;
        uint16_t loopTimeUs;
    };

    struct MotorCmd {
        int leftSpeed;
        int rightSpeed;
        String target; // "front", "back", "all"
    };

    // ==========================================
    // BUILDERS (Serialize)
    // ==========================================

    void buildTelemetry(JsonDocument &doc, const TelemetryData &data);
    void buildMotorCmd(JsonDocument &doc, const MotorCmd &cmd);
    void buildStatus(JsonDocument &doc, const char *role, const char *status, const char *msg);
    void buildHazardAlert(JsonDocument &doc, const char *hazardType, const char *message, bool critical = true);

    // ==========================================
    // PARSERS (Deserialize)
    // ==========================================

    // Returns true if parsing successful and type matches
    bool parseMotorCmd(const JsonDocument &doc, MotorCmd &outCmd);

}

#endif // MESSAGE_PROTOCOL_H
