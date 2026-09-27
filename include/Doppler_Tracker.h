#pragma once
#include <Arduino.h>
#include "CC1101_Driver.h"

class Doppler_Tracker {
public:
    Doppler_Tracker();

    void begin(float centerFreqMHz = 437.550f, float maxShiftKHz = 10.0f);
    void update(CC1101_Driver& radio);

    // Manual controls
    void stepUp(CC1101_Driver& radio, float stepKHz = 2.5f);
    void stepDown(CC1101_Driver& radio, float stepKHz = 2.5f);
    void resetToCenter(CC1101_Driver& radio);

    // Automated Pass Simulation
    void startPass(uint32_t durationSeconds = 600); // 10 minute default pass
    void stopPass();
    bool isPassActive() const { return passActive; }

    // Getters
    float getCurrentFreqMHz() const { return currentFreqMHz; }
    float getCurrentOffsetKHz() const { return (currentFreqMHz - centerFreqMHz) * 1000.0f; }

private:
    float centerFreqMHz;
    float currentFreqMHz;
    float maxShiftKHz;

    bool passActive;
    uint32_t passStartTime;
    uint32_t passDurationMs;
    uint32_t lastUpdateTime;

    void applyFrequency(CC1101_Driver& radio);
};

extern Doppler_Tracker Doppler;
