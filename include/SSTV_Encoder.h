#pragma once
#include <Arduino.h>

class SSTV_Encoder {
public:
    SSTV_Encoder();

    void begin(uint8_t modulationPin);
    
    // Low-level tone generation
    void playTone(float freqHz, uint32_t durationMs);
    void playToneMicros(float freqHz, uint32_t durationUs);

    // Protocol components
    void sendCalibrationHeader();
    void sendVIS(uint8_t visCode);
    void sendToneTest();

    // Full Frame Transmission
    void sendRobot36TestPattern(const char* callsign = "ESP32-CC1101");

private:
    uint8_t modPin;
    void sendPixelTone(uint8_t intensity, uint32_t durationUs);
};

extern SSTV_Encoder Encoder;
