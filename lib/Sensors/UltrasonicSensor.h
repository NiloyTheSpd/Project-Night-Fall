#ifndef ULTRASONIC_SENSOR_H
#define ULTRASONIC_SENSOR_H

#include <Arduino.h>

/**
 * HC-SR04 Ultrasonic Sensor Wrapper
 * Non-blocking distance measurement with filtering
 */
class UltrasonicSensor
{
public:
    UltrasonicSensor(uint8_t trigPin, uint8_t echoPin);

    void begin();

    /**
     * Get latest filtered distance (non-blocking)
     * @return distance in cm, or -1 if invalid/timeout
     */
    float getDistance();

    /**
     * Trigger a measurement (call periodically)
     */
    void update();

    /**
     * Check if obstacle detected below threshold
     */
    bool isObstacleDetected(float thresholdCm = 20.0f);

    /**
     * Get smoothed distance (EMA filter)
     */
    float getSmoothedDistance() { return _smoothedDistance; }

    /**
     * Reset sensor state
     */
    void reset();

private:
    uint8_t _trigPin, _echoPin;
    float _lastDistance;
    float _smoothedDistance;
    unsigned long _lastMeasureTime;
    unsigned long _pulseStart;

    // Non-blocking measurement states
    enum MeasureState
    {
        IDLE,
        WAITING_FOR_ECHO,
        MEASURING
    };
    MeasureState _state;

    static constexpr float EMA_ALPHA = 0.3f;
    static constexpr unsigned long ECHO_TIMEOUT_US = 30000UL;
    static constexpr unsigned long MEASURE_INTERVAL_MS = 60UL;
};

#endif // ULTRASONIC_SENSOR_H
