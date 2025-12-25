// lib/Navigation/WaypointNav.h
#ifndef WAYPOINT_NAV_H
#define WAYPOINT_NAV_H

#include <Arduino.h>
#include <vector>
#include "config.h"

// Waypoint structure
struct Waypoint {
    float x;                  // GPS longitude or local X coordinate
    float y;                  // GPS latitude or local Y coordinate
    String name;              // Waypoint name
    String action;            // Action at waypoint: "navigate", "stop", "scan", etc.
    float tolerance;          // Distance tolerance to consider reached (meters)
    bool reached;             // Has this waypoint been reached?
    unsigned long reachedTime; // Timestamp when reached
};

// Mission structure
struct Mission {
    String name;
    String id;
    std::vector<Waypoint> waypoints;
    unsigned long startTime;
    unsigned long endTime;
    bool active;
    bool completed;
};

class WaypointNav {
public:
    WaypointNav();
    
    void begin();
    void update();
    
    // Waypoint management
    bool addWaypoint(float x, float y, const char* name = "", const char* action = "navigate", float tolerance = 1.0);
    bool removeWaypoint(int index);
    bool clearWaypoints();
    int getWaypointCount();
    Waypoint* getWaypoint(int index);
    Waypoint* getCurrentWaypoint();
    
    // Mission management
    bool startMission();
    bool pauseMission();
    bool resumeMission();
    bool stopMission();
    bool isMissionActive();
    bool isMissionPaused();
    bool isMissionCompleted();
    
    // Mission I/O
    bool saveMission(const char* filename);
    bool loadMission(const char* filename);
    String exportMissionJSON();
    bool importMissionJSON(const String& json);
    
    // Navigation
    bool navigateToWaypoint(int index);
    bool navigateToNext();
    float getDistanceToWaypoint(int index);
    float getDistanceToCurrent();
    float getBearingToWaypoint(int index);
    float getBearingToCurrent();
    
    // Position tracking
    void updatePosition(float x, float y);
    void getPosition(float& x, float& y);
    
    // Mission statistics
    int getCurrentWaypointIndex();
    int getCompletedWaypointCount();
    float getTotalMissionDistance();
    float getRemainingDistance();
    unsigned long getElapsedTime();
    unsigned long getEstimatedTimeRemaining();
    
    // Configuration
    void setWaypointTolerance(float meters);
    void setNavigationSpeed(uint8_t speed);
    void setAutoAdvance(bool enable);
    
private:
    // Mission data
    Mission _currentMission;
    int _currentWaypointIndex;
    bool _paused;
    bool _autoAdvance;
    
    // Robot position
    float _currentX;
    float _currentY;
    
    // Navigation parameters
    float _waypointTolerance;
    uint8_t _navigationSpeed;
    unsigned long _lastUpdate;
    
    // Statistics
    float _totalDistance;
    unsigned long _missionStartTime;
    
    // Helper methods
    float calculateDistance(float x1, float y1, float x2, float y2);
    float calculateBearing(float x1, float y1, float x2, float y2);
    bool isWaypointReached(const Waypoint& wp);
    MovementCommand calculateNavigationCommand();
    String generateMissionID();
};

#endif // WAYPOINT_NAV_H

