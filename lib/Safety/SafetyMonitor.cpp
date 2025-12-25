// lib/Safety/SafetyMonitor.cpp
#include "SafetyMonitor.h"

SafetyMonitor::SafetyMonitor()
    : _safeDistance(SAFE_DISTANCE),
      _emergencyDistance(EMERGENCY_STOP_DISTANCE),
      _gasThreshold(GAS_THRESHOLD_ANALOG),
      _batteryLowThreshold(LOW_BATTERY_VOLTAGE),
      _batteryCriticalThreshold(LOW_BATTERY_VOLTAGE - 1.0),
      _commTimeout(WATCHDOG_TIMEOUT),
      _maxTiltAngle(MAX_TILT_ANGLE),
      _maxMotorCurrent(2.5),
      _activeAlertCount(0),
      _totalAlertCount(0),
      _emergencyStop(false),
      _emergencyTimestamp(0),
      _startTime(0),
      _batteryVoltageSum(0),
      _batteryReadingCount(0),
      _lastCollisionCheck(0),
      _lastGasCheck(0),
      _lastBatteryCheck(0),
      _lastCommCheck(0),
      _lastMotorCheck(0)
{

    // Initialize alert array
    for (int i = 0; i < MAX_ALERTS; i++)
    {
        _alerts[i].active = false;
    }
}

void SafetyMonitor::begin()
{
    _startTime = millis();
    _emergencyStop = false;
    clearAllAlerts();

    DEBUG_PRINTLN("Safety Monitor initialized");
    DEBUG_PRINT("Safe distance: ");
    DEBUG_PRINT(_safeDistance);
    DEBUG_PRINTLN(" cm");
    DEBUG_PRINT("Gas threshold: ");
    DEBUG_PRINTLN(_gasThreshold);
    DEBUG_PRINT("Battery threshold: ");
    DEBUG_PRINT(_batteryLowThreshold);
    DEBUG_PRINTLN(" V");
}

void SafetyMonitor::update()
{
    // Regular safety checks are performed by calling individual check methods
    // This method can be used for periodic housekeeping

    // Clean up old cleared alerts (compact array)
    for (int i = 0; i < MAX_ALERTS; i++)
    {
        if (!_alerts[i].active && i < _activeAlertCount - 1)
        {
            // Shift alerts down
            for (int j = i; j < _activeAlertCount - 1; j++)
            {
                _alerts[j] = _alerts[j + 1];
            }
            _activeAlertCount--;
        }
    }
}

bool SafetyMonitor::isSafe()
{
    // Returns true if system is safe to operate
    if (_emergencyStop)
    {
        return false;
    }

    // Check for critical alerts
    for (int i = 0; i < _activeAlertCount; i++)
    {
        if (_alerts[i].active && _alerts[i].level == ALERT_CRITICAL)
        {
            return false;
        }
    }

    return true;
}

bool SafetyMonitor::isEmergency()
{
    return _emergencyStop;
}

bool SafetyMonitor::checkCollisionRisk(float frontDist, float rearDist)
{
    _lastCollisionCheck = millis();

    // Check front collision risk
    if (frontDist > 0 && frontDist < _emergencyDistance)
    {
        raiseAlert(ALERT_COLLISION, ALERT_CRITICAL,
                   "Imminent front collision!");
        triggerEmergencyStop();
        return false;
    }
    else if (frontDist > 0 && frontDist < _safeDistance)
    {
        raiseAlert(ALERT_COLLISION, ALERT_WARNING,
                   "Front obstacle too close");
        return false;
    }

    // Check rear collision risk
    if (rearDist > 0 && rearDist < _emergencyDistance)
    {
        raiseAlert(ALERT_COLLISION, ALERT_CRITICAL,
                   "Imminent rear collision!");
        triggerEmergencyStop();
        return false;
    }
    else if (rearDist > 0 && rearDist < _safeDistance)
    {
        raiseAlert(ALERT_COLLISION, ALERT_WARNING,
                   "Rear obstacle too close");
        return false;
    }

    // Clear collision alerts if safe
    clearAlert(ALERT_COLLISION);
    return true;
}

bool SafetyMonitor::checkGasLevel(int gasValue)
{
    _lastGasCheck = millis();

    if (gasValue > _gasThreshold + 200)
    {
        raiseAlert(ALERT_GAS_DETECTED, ALERT_CRITICAL,
                   "Critical gas/smoke level detected!");
        triggerEmergencyStop();
        return false;
    }
    else if (gasValue > _gasThreshold)
    {
        raiseAlert(ALERT_GAS_DETECTED, ALERT_WARNING,
                   "Gas/smoke detected");
        return false;
    }

    clearAlert(ALERT_GAS_DETECTED);
    return true;
}

bool SafetyMonitor::checkBatteryLevel(float voltage)
{
    _lastBatteryCheck = millis();

    // Update battery statistics
    _batteryVoltageSum += voltage;
    _batteryReadingCount++;

    if (voltage < _batteryCriticalThreshold)
    {
        raiseAlert(ALERT_LOW_BATTERY, ALERT_CRITICAL,
                   "Battery critically low!");
        triggerEmergencyStop();
        return false;
    }
    else if (voltage < _batteryLowThreshold)
    {
        raiseAlert(ALERT_LOW_BATTERY, ALERT_WARNING,
                   "Battery low");
        return false;
    }

    clearAlert(ALERT_LOW_BATTERY);
    return true;
}

bool SafetyMonitor::checkCommunication(unsigned long lastHeartbeat)
{
    _lastCommCheck = millis();

    if (millis() - lastHeartbeat > _commTimeout)
    {
        raiseAlert(ALERT_COMMUNICATION_LOSS, ALERT_CRITICAL,
                   "Communication timeout!");
        triggerEmergencyStop();
        return false;
    }

    clearAlert(ALERT_COMMUNICATION_LOSS);
    return true;
}

bool SafetyMonitor::checkMotorHealth(float current1, float current2)
{
    _lastMotorCheck = millis();

    if (current1 > _maxMotorCurrent || current2 > _maxMotorCurrent)
    {
        raiseAlert(ALERT_OVERHEAT, ALERT_WARNING,
                   "Motor current too high");
        return false;
    }

    clearAlert(ALERT_OVERHEAT);
    return true;
}

bool SafetyMonitor::checkTilt(float pitch, float roll)
{
    if (abs(pitch) > _maxTiltAngle || abs(roll) > _maxTiltAngle)
    {
        raiseAlert(ALERT_TILT_EXCESSIVE, ALERT_CRITICAL,
                   "Excessive tilt detected!");
        triggerEmergencyStop();
        return false;
    }

    clearAlert(ALERT_TILT_EXCESSIVE);
    return true;
}

void SafetyMonitor::raiseAlert(AlertType type, AlertLevel level, const char *message)
{
    // Check if alert already exists
    int existingIndex = findAlertIndex(type);

    if (existingIndex >= 0)
    {
        // Phase 3.2: Rate-limit alerts - only re-raise if sufficient time has passed
        unsigned long timeSinceLastAlert = millis() - _alerts[existingIndex].timestamp;
        if (timeSinceLastAlert >= 500) // 500ms rate-limit to prevent spam
        {
            // Update existing alert after rate-limit period
            _alerts[existingIndex].level = level;
            _alerts[existingIndex].timestamp = millis();
            _alerts[existingIndex].message = String(message);

            DEBUG_PRINT("SAFETY ALERT [");
            DEBUG_PRINT(getAlertLevelName(level));
            DEBUG_PRINT("] - ");
            DEBUG_PRINT(getAlertTypeName(type));
            DEBUG_PRINT(": ");
            DEBUG_PRINTLN(message);
        }
        // else: suppress re-raise within 500ms window
    }
    else
    {
        // Create new alert
        SafetyAlert newAlert;
        newAlert.type = type;
        newAlert.level = level;
        newAlert.timestamp = millis();
        newAlert.message = String(message);
        newAlert.active = true;

        if (addAlert(newAlert))
        {
            _totalAlertCount++;

            DEBUG_PRINT("SAFETY ALERT [");
            DEBUG_PRINT(getAlertLevelName(level));
            DEBUG_PRINT("] - ");
            DEBUG_PRINT(getAlertTypeName(type));
            DEBUG_PRINT(": ");
            DEBUG_PRINTLN(message);
        }
    }
}

void SafetyMonitor::clearAlert(AlertType type)
{
    int index = findAlertIndex(type);
    if (index >= 0)
    {
        removeAlert(index);
    }
}

void SafetyMonitor::clearAllAlerts()
{
    for (int i = 0; i < MAX_ALERTS; i++)
    {
        _alerts[i].active = false;
    }
    _activeAlertCount = 0;
}

SafetyAlert *SafetyMonitor::getActiveAlerts(int &count)
{
    count = _activeAlertCount;
    return _alerts;
}

int SafetyMonitor::getAlertCount()
{
    return _activeAlertCount;
}

void SafetyMonitor::triggerEmergencyStop()
{
    if (!_emergencyStop)
    {
        _emergencyStop = true;
        _emergencyTimestamp = millis();

        DEBUG_PRINTLN("╔════════════════════════════════╗");
        DEBUG_PRINTLN("║   EMERGENCY STOP ACTIVATED!    ║");
        DEBUG_PRINTLN("╚════════════════════════════════╝");
    }
}

void SafetyMonitor::resetEmergencyStop()
{
    _emergencyStop = false;
    _emergencyTimestamp = 0;

    DEBUG_PRINTLN("Emergency stop reset - System ready");
}

bool SafetyMonitor::isEmergencyStopped()
{
    return _emergencyStop;
}

unsigned long SafetyMonitor::getUptimeSeconds()
{
    return (millis() - _startTime) / 1000;
}

int SafetyMonitor::getTotalAlertCount()
{
    return _totalAlertCount;
}

float SafetyMonitor::getAverageBatteryVoltage()
{
    if (_batteryReadingCount == 0)
    {
        return 0.0;
    }
    return _batteryVoltageSum / _batteryReadingCount;
}

void SafetyMonitor::setSafeDistance(float distance)
{
    _safeDistance = distance;
    DEBUG_PRINT("Safe distance updated: ");
    DEBUG_PRINT(_safeDistance);
    DEBUG_PRINTLN(" cm");
}

void SafetyMonitor::setGasThreshold(int threshold)
{
    _gasThreshold = threshold;
    DEBUG_PRINT("Gas threshold updated: ");
    DEBUG_PRINTLN(_gasThreshold);
}

void SafetyMonitor::setBatteryThreshold(float voltage)
{
    _batteryLowThreshold = voltage;
    _batteryCriticalThreshold = voltage - 1.0;
    DEBUG_PRINT("Battery threshold updated: ");
    DEBUG_PRINT(_batteryLowThreshold);
    DEBUG_PRINTLN(" V");
}

void SafetyMonitor::setCommunicationTimeout(unsigned long timeout)
{
    _commTimeout = timeout;
    DEBUG_PRINT("Communication timeout updated: ");
    DEBUG_PRINT(_commTimeout);
    DEBUG_PRINTLN(" ms");
}

int SafetyMonitor::findAlertIndex(AlertType type)
{
    for (int i = 0; i < _activeAlertCount; i++)
    {
        if (_alerts[i].active && _alerts[i].type == type)
        {
            return i;
        }
    }
    return -1;
}

bool SafetyMonitor::addAlert(SafetyAlert alert)
{
    if (_activeAlertCount >= MAX_ALERTS)
    {
        DEBUG_PRINTLN("Alert buffer full!");
        return false;
    }

    _alerts[_activeAlertCount] = alert;
    _activeAlertCount++;
    return true;
}

void SafetyMonitor::removeAlert(int index)
{
    if (index < 0 || index >= _activeAlertCount)
    {
        return;
    }

    _alerts[index].active = false;

    // Shift alerts down
    for (int i = index; i < _activeAlertCount - 1; i++)
    {
        _alerts[i] = _alerts[i + 1];
    }

    _activeAlertCount--;
}

const char *SafetyMonitor::getAlertTypeName(AlertType type)
{
    switch (type)
    {
    case ALERT_COLLISION:
        return "COLLISION";
    case ALERT_GAS_DETECTED:
        return "GAS_DETECTED";
    case ALERT_LOW_BATTERY:
        return "LOW_BATTERY";
    case ALERT_OVERHEAT:
        return "OVERHEAT";
    case ALERT_COMMUNICATION_LOSS:
        return "COMM_LOSS";
    case ALERT_TILT_EXCESSIVE:
        return "EXCESSIVE_TILT";
    case ALERT_STUCK:
        return "STUCK";
    case ALERT_SENSOR_FAILURE:
        return "SENSOR_FAILURE";
    default:
        return "UNKNOWN";
    }
}

const char *SafetyMonitor::getAlertLevelName(AlertLevel level)
{
    switch (level)
    {
    case ALERT_NONE:
        return "NONE";
    case ALERT_INFO:
        return "INFO";
    case ALERT_WARNING:
        return "WARNING";
    case ALERT_CRITICAL:
        return "CRITICAL";
    default:
        return "UNKNOWN";
    }
}