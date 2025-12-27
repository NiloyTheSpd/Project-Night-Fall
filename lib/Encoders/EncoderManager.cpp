#include "EncoderManager.h"

EncoderManager::EncoderManager() {
    // Initialize all wheel states
    for (int i = 0; i < WHEEL_COUNT; i++) {
        _wheels[i].pcntUnit = PCNT_UNIT_MAX;  // Invalid initially
        _wheels[i].pinA = -1;
        _wheels[i].pinB = -1;
        _wheels[i].enabled = false;
        _wheels[i].lastPcntCount = 0;
        _wheels[i].totalCount = 0;
        _wheels[i].rpm = 0.0f;
        _wheels[i].rpmBufferIndex = 0;
        _wheels[i].lastUpdate = 0;
        
        // Initialize RPM filter buffer
        for (int j = 0; j < RPM_FILTER_SIZE; j++) {
            _wheels[i].rpmBuffer[j] = 0.0f;
        }
    }
    
    // Phase 3.1: Configure REAR wheels only
    // Rear Left: GPIO 16 (A), GPIO 17 (B)
    _wheels[WHEEL_REAR_LEFT].pcntUnit = PCNT_UNIT_0;
    _wheels[WHEEL_REAR_LEFT].pinA = 16;
    _wheels[WHEEL_REAR_LEFT].pinB = 17;
    _wheels[WHEEL_REAR_LEFT].enabled = true;
    
    // Rear Right: GPIO 2 (A), GPIO 0 (B)
    _wheels[WHEEL_REAR_RIGHT].pcntUnit = PCNT_UNIT_1;
    _wheels[WHEEL_REAR_RIGHT].pinA = 2;
    _wheels[WHEEL_REAR_RIGHT].pinB = 0;
    _wheels[WHEEL_REAR_RIGHT].enabled = true;
}

void EncoderManager::begin() {
    // Initialize PCNT for each enabled wheel
    for (int i = 0; i < WHEEL_COUNT; i++) {
        if (_wheels[i].enabled) {
            initPCNT((WheelID)i);
        }
    }
}

void EncoderManager::initPCNT(WheelID wheel) {
    WheelState& w = _wheels[wheel];
    
    // PCNT Channel 0: Count on Signal A edges, controlled by Signal B
    pcnt_config_t pcnt_config_a = {
        .pulse_gpio_num = w.pinA,
        .ctrl_gpio_num = w.pinB,
        .lctrl_mode = PCNT_MODE_REVERSE,  // Reverse when B is low
        .hctrl_mode = PCNT_MODE_KEEP,     // Keep when B is high
        .pos_mode = PCNT_COUNT_INC,       // Increment on rising edge
        .neg_mode = PCNT_COUNT_DEC,       // Decrement on falling edge
        .counter_h_lim = 32767,
        .counter_l_lim = -32768,
        .unit = w.pcntUnit,
        .channel = PCNT_CHANNEL_0,
    };
    pcnt_unit_config(&pcnt_config_a);
    
    // PCNT Channel 1: Count on Signal B edges, controlled by Signal A
    pcnt_config_t pcnt_config_b = {
        .pulse_gpio_num = w.pinB,
        .ctrl_gpio_num = w.pinA,
        .lctrl_mode = PCNT_MODE_KEEP,     // Keep when A is low
        .hctrl_mode = PCNT_MODE_REVERSE,  // Reverse when A is high
        .pos_mode = PCNT_COUNT_INC,       // Increment on rising edge
        .neg_mode = PCNT_COUNT_DEC,       // Decrement on falling edge
        .counter_h_lim = 32767,
        .counter_l_lim = -32768,
        .unit = w.pcntUnit,
        .channel = PCNT_CHANNEL_1,
    };
    pcnt_unit_config(&pcnt_config_b);
    
    // Set glitch filter (debounce): ignore pulses shorter than ~1µs
    pcnt_set_filter_value(w.pcntUnit, 80);  // 80 APB clock cycles @ 80MHz = 1µs
    pcnt_filter_enable(w.pcntUnit);
    
    // Clear counter
    pcnt_counter_pause(w.pcntUnit);
    pcnt_counter_clear(w.pcntUnit);
    pcnt_counter_resume(w.pcntUnit);
    
    w.lastUpdate = millis();
}

void EncoderManager::update() {
    unsigned long now = millis();
    
    for (int i = 0; i < WHEEL_COUNT; i++) {
        if (!_wheels[i].enabled) continue;
        
        WheelState& w = _wheels[i];
        
        // Read current PCNT value
        int16_t currentCount;
        pcnt_get_counter_value(w.pcntUnit, &currentCount);
        
        // Calculate delta and handle overflow
        int16_t delta = currentCount - w.lastPcntCount;
        w.totalCount += delta;
        w.lastPcntCount = currentCount;
        
        // Calculate time delta
        float dt = (now - w.lastUpdate) / 1000.0f;  // Convert to seconds
        
        // Avoid division by zero
        if (dt > 0.001f) {
            // Calculate RPM
            calculateRPM((WheelID)i, dt);
            w.lastUpdate = now;
        }
    }
}

void EncoderManager::calculateRPM(WheelID wheel, float dt) {
    WheelState& w = _wheels[wheel];
    
    // Get count delta since last update
    int16_t currentCount;
    pcnt_get_counter_value(w.pcntUnit, &currentCount);
    int16_t delta = currentCount - w.lastPcntCount;
    
    // RPM = (counts / CPR) × (60 / dt) × gear_ratio
    // CPR = counts per revolution (ENCODER_PPR × 4 for quadrature)
    float rpm = (delta / (float)ENCODER_CPR) * (60.0f / dt) * GEAR_RATIO;
    
    // Add to circular buffer
    w.rpmBuffer[w.rpmBufferIndex] = rpm;
    w.rpmBufferIndex = (w.rpmBufferIndex + 1) % RPM_FILTER_SIZE;
    
    // Calculate moving average
    w.rpm = calculateMovingAverage(w.rpmBuffer, RPM_FILTER_SIZE);
}

float EncoderManager::calculateMovingAverage(const float* buffer, uint8_t size) {
    float sum = 0.0f;
    for (uint8_t i = 0; i < size; i++) {
        sum += buffer[i];
    }
    return sum / size;
}

// ========================================
// PUBLIC QUERY METHODS
// ========================================

int32_t EncoderManager::getCounts(WheelID wheel) const {
    if (wheel >= WHEEL_COUNT) return 0;
    return _wheels[wheel].totalCount;
}

float EncoderManager::getRPM(WheelID wheel) const {
    if (wheel >= WHEEL_COUNT) return 0.0f;
    return _wheels[wheel].rpm;
}

float EncoderManager::getDistanceCm(WheelID wheel) const {
    if (wheel >= WHEEL_COUNT) return 0.0f;
    
    // Distance = (counts / CPR) × circumference
    float revolutions = _wheels[wheel].totalCount / (float)ENCODER_CPR;
    return revolutions * WHEEL_CIRCUMFERENCE_CM;
}

bool EncoderManager::isStale(WheelID wheel) const {
    if (wheel >= WHEEL_COUNT) return true;
    if (!_wheels[wheel].enabled) return true;
    
    unsigned long timeSinceUpdate = millis() - _wheels[wheel].lastUpdate;
    return timeSinceUpdate > STALE_TIMEOUT_MS;
}

unsigned long EncoderManager::getLastUpdate(WheelID wheel) const {
    if (wheel >= WHEEL_COUNT) return 0;
    return _wheels[wheel].lastUpdate;
}

// ========================================
// CONTROL METHODS
// ========================================

void EncoderManager::resetCounts(WheelID wheel) {
    if (wheel >= WHEEL_COUNT) return;
    if (!_wheels[wheel].enabled) return;
    
    // Reset total count
    _wheels[wheel].totalCount = 0;
    
    // Reset PCNT hardware counter
    pcnt_counter_pause(_wheels[wheel].pcntUnit);
    pcnt_counter_clear(_wheels[wheel].pcntUnit);
    pcnt_counter_resume(_wheels[wheel].pcntUnit);
    
    _wheels[wheel].lastPcntCount = 0;
}

void EncoderManager::resetAll() {
    for (int i = 0; i < WHEEL_COUNT; i++) {
        if (_wheels[i].enabled) {
            resetCounts((WheelID)i);
        }
    }
}
