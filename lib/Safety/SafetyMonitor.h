// lib/Safety/SafetyMonitor.h
#ifndef SAFETY_MONITOR_H
#define SAFETY_MONITOR_H

#include <Arduino.h>
#include "config.h"

// Safety alert levels
enum AlertLevel
{
    ALERT_NONE,
    ALERT_INFO,
    ALERT_WARNING,
    ALERT_CRITICAL
};

// Safety alert types
enum AlertType
{
    ALERT_COLLISION,
    ALERT_GAS_DETECTED,
    ALERT_LOW_BATTERY,
    ALERT_OVERHEAT,
    ALERT_COMMUNICATION_LOSS,
    ALERT_TILT_EXCESSIVE,
    ALERT_STUCK,
    ALERT_SENSOR_FAILURE
};

// Safety alert structure
struct SafetyAlert
{
    AlertType type;
    AlertLevel level;
    unsigned long timestamp;
    String message;
    bool active;
};

class SafetyMonitor
{
public:
    SafetyMonitor();

    void begin();
    void update();

    // Main safety checks
    bool isSafe();
    bool isEmergency();

    // Individual safety checks
    bool checkCollisionRisk(float frontDist, float rearDist);
    bool checkGasLevel(int gasValue);
    bool checkBatteryLevel(float voltage);
    bool checkCommunication(unsigned long lastHeartbeat);
    bool checkMotorHealth(float current1, float current2);
    bool checkTilt(float pitch, float roll);

    // Alert management
    void raiseAlert(AlertType type, AlertLevel level, const char *message);
    void clearAlert(AlertType type);
    void clearAllAlerts();
    SafetyAlert *getActiveAlerts(int &count);
    int getAlertCount();

    // Emergency handling
    void triggerEmergencyStop();
    void resetEmergencyStop();
    bool isEmergencyStopped();

    // Statistics
    unsigned long getUptimeSeconds();
    int getTotalAlertCount();
    float getAverageBatteryVoltage();

    // Configuration
    void setSafeDistance(float distance);
    void setGasThreshold(int threshold);
    void setBatteryThreshold(float voltage);
    void setCommunicationTimeout(unsigned long timeout);

private:
    // Safety thresholds
    float _safeDistance;
    float _emergencyDistance;
    int _gasThreshold;
    float _batteryLowThreshold;
    float _batteryCriticalThreshold;
    unsigned long _commTimeout;
    float _maxTiltAngle;
    float _maxMotorCurrent;

    // Alert tracking
    static const int MAX_ALERTS = 10;
    SafetyAlert _alerts[MAX_ALERTS];
    int _activeAlertCount;
    int _totalAlertCount;

    // Emergency state
    bool _emergencyStop;
    unsigned long _emergencyTimestamp;

    // Statistics
    unsigned long _startTime;
    float _batteryVoltageSum;
    int _batteryReadingCount;

    // Last check timestamps
    unsigned long _lastCollisionCheck;
    unsigned long _lastGasCheck;
    unsigned long _lastBatteryCheck;
    unsigned long _lastCommCheck;
    unsigned long _lastMotorCheck;

    // Helper methods
    int findAlertIndex(AlertType type);
    bool addAlert(SafetyAlert alert);
    void removeAlert(int index);
    const char *getAlertTypeName(AlertType type);
    const char *getAlertLevelName(AlertLevel level);
};

#endif // SAFETY_MONITOR_H
