#include "MessageProtocol.h"

namespace Msg
{
    // ==========================================
    // CONSTANTS DEFINITION
    // ==========================================
    const char *TYPE_MOTOR_CMD = "motor_cmd";
    const char *TYPE_SENSOR_UPDATE = "sensor_update";
    const char *TYPE_TELEMETRY = "telemetry";
    const char *TYPE_HAZARD_ALERT = "hazard_alert";
    const char *TYPE_UI_CMD = "ui_cmd";
    const char *TYPE_STATUS = "status";
    const char *TYPE_PING = "ping";
    const char *TYPE_ACK = "ack";

    const char *ROLE_BACK = "back";
    const char *ROLE_FRONT = "front";
    const char *ROLE_CAMERA = "camera";

    const char *HAZARD_GAS = "gas_detected";
    const char *HAZARD_COLLISION = "collision";
    const char *HAZARD_TILT = "excessive_tilt";

    // ==========================================
    // BUILDERS
    // ==========================================

    void buildTelemetry(JsonDocument &doc, const TelemetryData &data)
    {
        doc["type"] = TYPE_TELEMETRY;
        doc["from"] = ROLE_BACK;

        // Sensors
        JsonObject sensors = doc.createNestedObject("sensors");
        sensors["front_dist"] = data.frontDist;
        sensors["rear_dist"] = data.rearDist;
        sensors["gas"] = data.gasLevel;

        // Motors
        JsonObject motors = doc.createNestedObject("motors");
        motors["front_left"] = data.frontLeftSpeed;
        motors["front_right"] = data.frontRightSpeed;
        motors["rear_left"] = data.rearLeftSpeed;
        motors["rear_right"] = data.rearRightSpeed;

        // State
        JsonObject state = doc.createNestedObject("state");
        state["autonomous"] = data.isAutonomous;
        state["nav_state"] = data.navState;
        
        // Metadata
        doc["server_clients"] = data.clientCount;
        
        // Network Status
        JsonObject network = doc.createNestedObject("network");
        network["front"] = data.frontOnline;
        network["camera"] = data.cameraOnline;
        
        // Control debug (Phase 2.5)
        JsonObject control = doc.createNestedObject("control");
        control["out"] = data.pidOutput;
        control["err"] = data.pidError;
        control["sp"] = data.pidSetpoint;
        control["P"] = data.pidP;
        control["I"] = data.pidI;
        control["D"] = data.pidD;
        
        // Timing
        JsonObject timing = doc.createNestedObject("timing");
        timing["loop_us"] = data.loopTimeUs;
        
        doc["ts"] = millis();
    }

    void buildMotorCmd(JsonDocument &doc, const MotorCmd &cmd)
    {
        doc["type"] = TYPE_MOTOR_CMD;
        doc["target"] = cmd.target;
        doc["left"] = cmd.leftSpeed;
        doc["right"] = cmd.rightSpeed;
        doc["ts"] = millis();
    }

    void buildStatus(JsonDocument &doc, const char *role, const char *status, const char *msg)
    {
        doc["type"] = TYPE_STATUS;
        doc["role"] = role;
        doc["status"] = status;
        doc["msg"] = msg;
        doc["ts"] = millis();
    }

    void buildHazardAlert(JsonDocument &doc, const char *hazardType, const char *message, bool critical)
    {
        doc["type"] = TYPE_HAZARD_ALERT;
        doc["hazard"] = hazardType;
        doc["msg"] = message;
        doc["critical"] = critical;
        doc["ts"] = millis();
    }

    // ==========================================
    // PARSERS
    // ==========================================

    bool parseMotorCmd(const JsonDocument &doc, MotorCmd &outCmd)
    {
        const char *type = doc["type"] | "";
        if (strcmp(type, TYPE_MOTOR_CMD) != 0) return false;

        outCmd.target = doc["target"].as<String>();
        outCmd.leftSpeed = doc["left"] | 0;
        outCmd.rightSpeed = doc["right"] | 0;
        
        // P0 Fix #5: VALIDATION - Clamp motor speeds to safe range
        outCmd.leftSpeed = constrain(outCmd.leftSpeed, -255, 255);
        outCmd.rightSpeed = constrain(outCmd.rightSpeed, -255, 255);
        
        return true;
    }
}
