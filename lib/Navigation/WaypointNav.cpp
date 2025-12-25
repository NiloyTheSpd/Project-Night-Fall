// lib/Navigation/WaypointNav.cpp
#include "WaypointNav.h"
#include <ArduinoJson.h>
#include <math.h>

WaypointNav::WaypointNav()
    : _currentWaypointIndex(0),
      _paused(false),
      _autoAdvance(true),
      _currentX(0),
      _currentY(0),
      _waypointTolerance(1.0),
      _navigationSpeed(MOTOR_NORMAL_SPEED),
      _lastUpdate(0),
      _totalDistance(0),
      _missionStartTime(0)
{

    _currentMission.active = false;
    _currentMission.completed = false;
}

void WaypointNav::begin()
{
    DEBUG_PRINTLN("Waypoint Navigation initialized");
}

void WaypointNav::update()
{
    if (!_currentMission.active || _paused)
    {
        return;
    }

    // Rate limiting
    if (millis() - _lastUpdate < 100)
    {
        return;
    }
    _lastUpdate = millis();

    // Check if current waypoint reached
    if (_currentWaypointIndex < _currentMission.waypoints.size())
    {
        Waypoint &current = _currentMission.waypoints[_currentWaypointIndex];

        if (isWaypointReached(current))
        {
            current.reached = true;
            current.reachedTime = millis();

            DEBUG_PRINT("Waypoint reached: ");
            DEBUG_PRINTLN(current.name);

            // Execute waypoint action
            if (current.action == "stop")
            {
                pauseMission();
            }
            else if (current.action == "scan")
            {
                // Trigger 360 scan
                DEBUG_PRINTLN("Performing 360 scan at waypoint");
            }

            // Auto-advance to next waypoint
            if (_autoAdvance)
            {
                if (_currentWaypointIndex + 1 < _currentMission.waypoints.size())
                {
                    _currentWaypointIndex++;
                    DEBUG_PRINT("Advancing to waypoint ");
                    DEBUG_PRINTLN(_currentWaypointIndex + 1);
                }
                else
                {
                    // Mission completed
                    _currentMission.completed = true;
                    _currentMission.active = false;
                    _currentMission.endTime = millis();
                    DEBUG_PRINTLN("Mission completed!");
                }
            }
        }
    }
}

bool WaypointNav::addWaypoint(float x, float y, const char *name, const char *action, float tolerance)
{
    Waypoint wp;
    wp.x = x;
    wp.y = y;
    wp.name = String(name);
    if (wp.name.length() == 0)
    {
        wp.name = "WP" + String(_currentMission.waypoints.size() + 1);
    }
    wp.action = String(action);
    wp.tolerance = tolerance;
    wp.reached = false;
    wp.reachedTime = 0;

    _currentMission.waypoints.push_back(wp);

    DEBUG_PRINT("Waypoint added: ");
    DEBUG_PRINT(wp.name);
    DEBUG_PRINT(" (");
    DEBUG_PRINT(x);
    DEBUG_PRINT(", ");
    DEBUG_PRINT(y);
    DEBUG_PRINTLN(")");

    return true;
}

bool WaypointNav::removeWaypoint(int index)
{
    if (index < 0 || index >= _currentMission.waypoints.size())
    {
        return false;
    }

    _currentMission.waypoints.erase(_currentMission.waypoints.begin() + index);

    // Adjust current index if necessary
    if (_currentWaypointIndex >= index && _currentWaypointIndex > 0)
    {
        _currentWaypointIndex--;
    }

    return true;
}

bool WaypointNav::clearWaypoints()
{
    _currentMission.waypoints.clear();
    _currentWaypointIndex = 0;
    _currentMission.completed = false;

    DEBUG_PRINTLN("All waypoints cleared");
    return true;
}

int WaypointNav::getWaypointCount()
{
    return _currentMission.waypoints.size();
}

Waypoint *WaypointNav::getWaypoint(int index)
{
    if (index < 0 || index >= _currentMission.waypoints.size())
    {
        return nullptr;
    }
    return &_currentMission.waypoints[index];
}

Waypoint *WaypointNav::getCurrentWaypoint()
{
    return getWaypoint(_currentWaypointIndex);
}

bool WaypointNav::startMission()
{
    if (_currentMission.waypoints.size() == 0)
    {
        DEBUG_PRINTLN("Cannot start mission: No waypoints");
        return false;
    }

    _currentMission.active = true;
    _currentMission.completed = false;
    _currentMission.startTime = millis();
    _currentWaypointIndex = 0;
    _paused = false;
    _missionStartTime = millis();

    // Generate mission ID
    _currentMission.id = generateMissionID();

    // Calculate total distance
    _totalDistance = getTotalMissionDistance();

    DEBUG_PRINTLN("Mission started!");
    DEBUG_PRINT("Total waypoints: ");
    DEBUG_PRINTLN(_currentMission.waypoints.size());
    DEBUG_PRINT("Total distance: ");
    DEBUG_PRINT(_totalDistance);
    DEBUG_PRINTLN(" m");

    return true;
}

bool WaypointNav::pauseMission()
{
    if (!_currentMission.active)
    {
        return false;
    }

    _paused = true;
    DEBUG_PRINTLN("Mission paused");
    return true;
}

bool WaypointNav::resumeMission()
{
    if (!_currentMission.active)
    {
        return false;
    }

    _paused = false;
    DEBUG_PRINTLN("Mission resumed");
    return true;
}

bool WaypointNav::stopMission()
{
    _currentMission.active = false;
    _currentMission.endTime = millis();
    _paused = false;

    DEBUG_PRINTLN("Mission stopped");
    return true;
}

bool WaypointNav::isMissionActive()
{
    return _currentMission.active;
}

bool WaypointNav::isMissionPaused()
{
    return _paused;
}

bool WaypointNav::isMissionCompleted()
{
    return _currentMission.completed;
}

bool WaypointNav::navigateToWaypoint(int index)
{
    if (index < 0 || index >= _currentMission.waypoints.size())
    {
        return false;
    }

    _currentWaypointIndex = index;
    DEBUG_PRINT("Navigating to waypoint ");
    DEBUG_PRINTLN(index + 1);

    return true;
}

bool WaypointNav::navigateToNext()
{
    if (_currentWaypointIndex + 1 >= _currentMission.waypoints.size())
    {
        return false;
    }

    _currentWaypointIndex++;
    return true;
}

float WaypointNav::getDistanceToWaypoint(int index)
{
    Waypoint *wp = getWaypoint(index);
    if (!wp)
    {
        return -1;
    }

    return calculateDistance(_currentX, _currentY, wp->x, wp->y);
}

float WaypointNav::getDistanceToCurrent()
{
    return getDistanceToWaypoint(_currentWaypointIndex);
}

float WaypointNav::getBearingToWaypoint(int index)
{
    Waypoint *wp = getWaypoint(index);
    if (!wp)
    {
        return -1;
    }

    return calculateBearing(_currentX, _currentY, wp->x, wp->y);
}

float WaypointNav::getBearingToCurrent()
{
    return getBearingToWaypoint(_currentWaypointIndex);
}

void WaypointNav::updatePosition(float x, float y)
{
    _currentX = x;
    _currentY = y;
}

void WaypointNav::getPosition(float &x, float &y)
{
    x = _currentX;
    y = _currentY;
}

int WaypointNav::getCurrentWaypointIndex()
{
    return _currentWaypointIndex;
}

int WaypointNav::getCompletedWaypointCount()
{
    int count = 0;
    for (const auto &wp : _currentMission.waypoints)
    {
        if (wp.reached)
        {
            count++;
        }
    }
    return count;
}

float WaypointNav::getTotalMissionDistance()
{
    if (_currentMission.waypoints.size() < 2)
    {
        return 0;
    }

    float total = 0;

    // Distance from current position to first waypoint
    total += calculateDistance(_currentX, _currentY,
                               _currentMission.waypoints[0].x,
                               _currentMission.waypoints[0].y);

    // Distance between consecutive waypoints
    for (size_t i = 0; i < _currentMission.waypoints.size() - 1; i++)
    {
        total += calculateDistance(_currentMission.waypoints[i].x,
                                   _currentMission.waypoints[i].y,
                                   _currentMission.waypoints[i + 1].x,
                                   _currentMission.waypoints[i + 1].y);
    }

    return total;
}

float WaypointNav::getRemainingDistance()
{
    if (!_currentMission.active || _currentWaypointIndex >= _currentMission.waypoints.size())
    {
        return 0;
    }

    float remaining = 0;

    // Distance to current waypoint
    remaining += getDistanceToCurrent();

    // Distance between remaining waypoints
    for (size_t i = _currentWaypointIndex; i < _currentMission.waypoints.size() - 1; i++)
    {
        remaining += calculateDistance(_currentMission.waypoints[i].x,
                                       _currentMission.waypoints[i].y,
                                       _currentMission.waypoints[i + 1].x,
                                       _currentMission.waypoints[i + 1].y);
    }

    return remaining;
}

unsigned long WaypointNav::getElapsedTime()
{
    if (!_currentMission.active)
    {
        return 0;
    }

    return (millis() - _currentMission.startTime) / 1000; // Return in seconds
}

unsigned long WaypointNav::getEstimatedTimeRemaining()
{
    if (!_currentMission.active || _totalDistance == 0)
    {
        return 0;
    }

    float remainingDist = getRemainingDistance();
    float elapsedTime = getElapsedTime();
    float completedDist = _totalDistance - remainingDist;

    if (completedDist <= 0)
    {
        return 0;
    }

    // Estimate based on average speed
    float avgSpeed = completedDist / elapsedTime; // meters per second

    if (avgSpeed <= 0)
    {
        return 0;
    }

    return (unsigned long)(remainingDist / avgSpeed); // seconds
}

void WaypointNav::setWaypointTolerance(float meters)
{
    _waypointTolerance = meters;
}

void WaypointNav::setNavigationSpeed(uint8_t speed)
{
    _navigationSpeed = constrain(speed, 0, 255);
}

void WaypointNav::setAutoAdvance(bool enable)
{
    _autoAdvance = enable;
}

String WaypointNav::exportMissionJSON()
{
    StaticJsonDocument<4096> doc;

    doc["name"] = _currentMission.name;
    doc["id"] = _currentMission.id;
    doc["created"] = millis();

    JsonArray waypoints = doc.createNestedArray("waypoints");

    for (const auto &wp : _currentMission.waypoints)
    {
        JsonObject wpObj = waypoints.createNestedObject();
        wpObj["x"] = wp.x;
        wpObj["y"] = wp.y;
        wpObj["name"] = wp.name;
        wpObj["action"] = wp.action;
        wpObj["tolerance"] = wp.tolerance;
    }

    String output;
    serializeJson(doc, output);

    return output;
}

bool WaypointNav::importMissionJSON(const String &json)
{
    StaticJsonDocument<4096> doc;

    DeserializationError error = deserializeJson(doc, json);
    if (error)
    {
        DEBUG_PRINT("JSON parse error: ");
        DEBUG_PRINTLN(error.c_str());
        return false;
    }

    clearWaypoints();

    _currentMission.name = doc["name"].as<String>();
    _currentMission.id = doc["id"].as<String>();

    JsonArray waypoints = doc["waypoints"];

    for (JsonObject wpObj : waypoints)
    {
        float x = wpObj["x"];
        float y = wpObj["y"];
        const char *name = wpObj["name"];
        const char *action = wpObj["action"];
        float tolerance = wpObj["tolerance"] | _waypointTolerance;

        addWaypoint(x, y, name, action, tolerance);
    }

    DEBUG_PRINTLN("Mission imported successfully");
    return true;
}

float WaypointNav::calculateDistance(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;
    return sqrt(dx * dx + dy * dy);
}

float WaypointNav::calculateBearing(float x1, float y1, float x2, float y2)
{
    float dx = x2 - x1;
    float dy = y2 - y1;

    // Calculate angle in radians, then convert to degrees
    float angle = atan2(dy, dx) * 180.0 / PI;

    // Normalize to 0-360
    if (angle < 0)
    {
        angle += 360;
    }

    return angle;
}

bool WaypointNav::isWaypointReached(const Waypoint &wp)
{
    float distance = calculateDistance(_currentX, _currentY, wp.x, wp.y);
    return distance <= wp.tolerance;
}

MovementCommand WaypointNav::calculateNavigationCommand()
{
    Waypoint *target = getCurrentWaypoint();
    if (!target)
    {
        return CMD_STOP;
    }

    float bearing = getBearingToCurrent();
    float distance = getDistanceToCurrent();

    // Simple navigation logic
    if (distance < target->tolerance)
    {
        return CMD_STOP;
    }

    // Determine turn direction based on bearing
    // This is simplified - real implementation would use IMU/compass
    if (bearing >= 45 && bearing < 135)
    {
        return CMD_TURN_RIGHT;
    }
    else if (bearing >= 135 && bearing < 225)
    {
        return CMD_BACKWARD;
    }
    else if (bearing >= 225 && bearing < 315)
    {
        return CMD_TURN_LEFT;
    }
    else
    {
        return CMD_FORWARD;
    }
}

String WaypointNav::generateMissionID()
{
    char buffer[32];
    sprintf(buffer, "MISSION_%lu", millis());
    return String(buffer);
}