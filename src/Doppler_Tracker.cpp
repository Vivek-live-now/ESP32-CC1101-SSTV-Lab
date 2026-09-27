#include "Doppler_Tracker.h"

Doppler_Tracker Doppler;

Doppler_Tracker::Doppler_Tracker()
    : centerFreqMHz(437.550f), currentFreqMHz(437.550f), maxShiftKHz(10.0f),
      passActive(false), passStartTime(0), passDurationMs(600000), lastUpdateTime(0) {}

void Doppler_Tracker::begin(float centerFreq, float maxShift) {
    centerFreqMHz = centerFreq;
    currentFreqMHz = centerFreq;
    maxShiftKHz = maxShift;
    passActive = false;
}

void Doppler_Tracker::applyFrequency(CC1101_Driver& radio) {
    radio.setFrequency(currentFreqMHz);
}

void Doppler_Tracker::stepUp(CC1101_Driver& radio, float stepKHz) {
    currentFreqMHz += (stepKHz / 1000.0f);
    applyFrequency(radio);
}

void Doppler_Tracker::stepDown(CC1101_Driver& radio, float stepKHz) {
    currentFreqMHz -= (stepKHz / 1000.0f);
    applyFrequency(radio);
}

void Doppler_Tracker::resetToCenter(CC1101_Driver& radio) {
    currentFreqMHz = centerFreqMHz;
    applyFrequency(radio);
}

void Doppler_Tracker::startPass(uint32_t durationSeconds) {
    passActive = true;
    passStartTime = millis();
    passDurationMs = durationSeconds * 1000UL;
    lastUpdateTime = millis();
    // Start at positive maximum Doppler shift (AOS - satellite approaching)
    currentFreqMHz = centerFreqMHz + (maxShiftKHz / 1000.0f);
}

void Doppler_Tracker::stopPass() {
    passActive = false;
}

void Doppler_Tracker::update(CC1101_Driver& radio) {
    if (!passActive) return;

    // Update frequency every 2 seconds
    if (millis() - lastUpdateTime < 2000) return;
    lastUpdateTime = millis();

    uint32_t elapsed = millis() - passStartTime;
    if (elapsed >= passDurationMs) {
        passActive = false;
        currentFreqMHz = centerFreqMHz - (maxShiftKHz / 1000.0f);
        applyFrequency(radio);
        return;
    }

    // S-curve approximation or linear progression from +maxShift to -maxShift
    float progress = (float)elapsed / (float)passDurationMs; // 0.0 to 1.0
    float shiftKHz = maxShiftKHz * (1.0f - 2.0f * progress);

    currentFreqMHz = centerFreqMHz + (shiftKHz / 1000.0f);
    applyFrequency(radio);
}
