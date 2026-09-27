#pragma once
#include <Arduino.h>
#include "PinConfig.h"

class SSTV_Demodulator {
public:
    SSTV_Demodulator();

    void begin(uint8_t inputPin, uint8_t outputPin);
    void stop();
    void update();

    // Stats
    float getInstantaneousFrequency() const { return currentFrequencyHz; }
    uint32_t getValidToneCount() const { return validToneCount; }
    bool isReceivingTone() const { return toneActive; }

    // Interrupt handler (internal)
    static void IRAM_ATTR handleEdgeISR();

private:
    uint8_t inputPin;
    uint8_t outputPin;
    bool running;

    static volatile uint32_t lastEdgeTimeMicros;
    static volatile uint32_t currentPeriodMicros;
    static volatile bool newPeriodAvailable;

    float currentFrequencyHz;
    uint32_t validToneCount;
    bool toneActive;
    uint32_t lastToneActivityMillis;

    // DAC phase accumulator for smooth sine output
    float phase;
    uint32_t lastDacUpdateMicros;

    void outputToneSample(float freq);
};

extern SSTV_Demodulator Demodulator;
