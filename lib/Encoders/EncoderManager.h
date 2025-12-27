#ifndef ENCODER_MANAGER_H
#define ENCODER_MANAGER_H

#include <Arduino.h>
#include <driver/pcnt.h>

/**
 * EncoderManager - Wheel encoder tracking using ESP32 PCNT
 * 
 * Manages quadrature encoders for Project Nightfall wheels
 * Features:
 * - Hardware PCNT counting (no interrupts needed)
 * - Per-wheel RPM calculation with moving average
 * - Distance tracking
 * - Direction detection
 * - Stale data detection
 */

enum WheelID {
    WHEEL_REAR_LEFT = 0,
    WHEEL_REAR_RIGHT = 1,
    WHEEL_FRONT_LEFT_1 = 2,
    WHEEL_FRONT_RIGHT_1 = 3,
    WHEEL_FRONT_LEFT_2 = 4,
    WHEEL_FRONT_RIGHT_2 = 5,
    WHEEL_COUNT = 6
};

// Encoder configuration
#define ENCODER_PPR 20              // Pulses per revolution (encoder disc slots)
#define ENCODER_CPR (ENCODER_PPR * 4)  // Counts per revolution (quadrature = 4×)
#define WHEEL_DIAMETER_CM 6.5f      // Wheel diameter in cm
#define WHEEL_CIRCUMFERENCE_CM (WHEEL_DIAMETER_CM * PI)
#define GEAR_RATIO 1.0f             // Motor gear ratio (1:1 if direct drive)
#define STALE_TIMEOUT_MS 100        // Data considered stale after 100ms

// Moving average filter size
#define RPM_FILTER_SIZE 5

class EncoderManager {
public:
    EncoderManager();
    
    /**
     * Initialize encoder hardware
     * Sets up PCNT units for configured wheels
     */
    void begin();
    
    /**
     * Update encoder readings
     * Call at ~200Hz for accurate RPM tracking
     */
    void update();
    
    // ========================================
    // QUERIES
    // ========================================
    
    /**
     * Get accumulated encoder counts (since reset)
     */
    int32_t getCounts(WheelID wheel) const;
    
    /**
     * Get current RPM (filtered)
     */
    float getRPM(WheelID wheel) const;
    
    /**
     * Get distance traveled in cm (since reset)
     */
    float getDistanceCm(WheelID wheel) const;
    
    /**
     * Check if encoder data is stale
     * Returns true if no updates for >STALE_TIMEOUT_MS
     */
    bool isStale(WheelID wheel) const;
    
    /**
     * Get last update time for wheel
     */
    unsigned long getLastUpdate(WheelID wheel) const;
    
    // ========================================
    // CONTROL
    // ========================================
    
    /**
     * Reset counts for specific wheel
     */
    void resetCounts(WheelID wheel);
    
    /**
     * Reset all wheels
     */
    void resetAll();
    
private:
    struct WheelState {
        // Hardware
        pcnt_unit_t pcntUnit;
        int pinA;
        int pinB;
        bool enabled;
        
        // Counting
        int16_t lastPcntCount;    // Last PCNT hardware count
        int32_t totalCount;       // Accumulated count (handles overflow)
        
        // Velocity
        float rpm;
        float rpmBuffer[RPM_FILTER_SIZE];
        uint8_t rpmBufferIndex;
        
        // Timing
        unsigned long lastUpdate;
    };
    
    WheelState _wheels[WHEEL_COUNT];
    
    // Helper methods
    void initPCNT(WheelID wheel);
    void calculateRPM(WheelID wheel, float dt);
    float calculateMovingAverage(const float* buffer, uint8_t size);
};

#endif // ENCODER_MANAGER_H
