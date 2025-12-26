#ifndef MQ2_SENSOR_H
#define MQ2_SENSOR_H

#include <Arduino.h>

/**
 * MQ-2 Smoke/Gas Sensor Wrapper
 * Analog reading with filtering and threshold detection
 */
class MQ2Sensor
{
public:
    MQ2Sensor(uint8_t analogPin, uint8_t digitalPin = 0xFF);

    void begin();

    /**
     * Get current gas reading (0-4095 for analog)
     */
    int getReading();

    /**
     * Update sensor reading (call periodically)
     */
    void update();

    /**
     * Get smoothed reading (running average)
     */
    int getSmoothedReading() { return _smoothedReading; }

    /**
     * Check if gas level exceeds threshold
     */
    bool isGasDetected(int threshold = 400);

    /**
     * Get alert status (threshold exceeded)
     */
    bool getAlert() { return _alert; }

    /**
     * Get reading trend (-1: decreasing, 0: stable, 1: increasing)
     */
    int getTrend() { return _trend; }

    /**
     * Reset sensor state
     */
    void reset();

private:
    uint8_t _analogPin;
    uint8_t _digitalPin;
    int _lastReading;
    int _smoothedReading;
    unsigned long _lastUpdateTime;
    bool _alert;
    int _trend;

    static constexpr unsigned long UPDATE_INTERVAL_MS = 500UL;
    static constexpr int SAMPLES = 5;
};

#endif // MQ2_SENSOR_H
