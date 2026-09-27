#include "SSTV_Encoder.h"

SSTV_Encoder Encoder;

SSTV_Encoder::SSTV_Encoder() : modPin(4) {}

void SSTV_Encoder::begin(uint8_t modulationPin) {
    modPin = modulationPin;
    pinMode(modPin, OUTPUT);
    digitalWrite(modPin, LOW);
}

void SSTV_Encoder::playToneMicros(float freqHz, uint32_t durationUs) {
    if (freqHz <= 0.0f) {
        digitalWrite(modPin, LOW);
        delayMicroseconds(durationUs);
        return;
    }

    uint32_t halfPeriodUs = (uint32_t)(500000.0f / freqHz + 0.5f);
    uint32_t startTime = micros();

    while (micros() - startTime < durationUs) {
        digitalWrite(modPin, HIGH);
        delayMicroseconds(halfPeriodUs);
        digitalWrite(modPin, LOW);
        delayMicroseconds(halfPeriodUs);
    }
}

void SSTV_Encoder::playTone(float freqHz, uint32_t durationMs) {
    playToneMicros(freqHz, durationMs * 1000UL);
}

void SSTV_Encoder::sendCalibrationHeader() {
    // 1900 Hz leader tone: 300 ms
    playTone(1900.0f, 300);
    // 1200 Hz break: 10 ms
    playTone(1200.0f, 10);
    // 1900 Hz leader: 100 ms
    playTone(1900.0f, 100);
}

void SSTV_Encoder::sendVIS(uint8_t visCode) {
    // VIS start bit: 1200 Hz, 30 ms
    playTone(1200.0f, 30);

    uint8_t parityCount = 0;
    // 7 data bits, LSB first (1300 Hz = 0, 1200 Hz = 1), 30 ms each
    for (int i = 0; i < 7; i++) {
        if (visCode & (1 << i)) {
            playTone(1200.0f, 30);
            parityCount++;
        } else {
            playTone(1300.0f, 30);
        }
    }

    // Even parity bit (30 ms)
    if (parityCount % 2 != 0) {
        playTone(1200.0f, 30); // 1
    } else {
        playTone(1300.0f, 30); // 0
    }

    // VIS stop bit: 1200 Hz, 30 ms
    playTone(1200.0f, 30);
}

void SSTV_Encoder::sendToneTest() {
    // Calibration test tones
    playTone(1200.0f, 1000); // Sync (1200 Hz)
    playTone(1500.0f, 1000); // Black porch (1500 Hz)
    playTone(1900.0f, 1000); // Leader (1900 Hz)
    playTone(2300.0f, 1000); // White peak (2300 Hz)
}

void SSTV_Encoder::sendRobot36TestPattern(const char* callsign) {
    // 1. Send Header + Robot 36 VIS Code (0x08)
    sendCalibrationHeader();
    sendVIS(0x08);

    // Standard 8 color bar intensities in Robot 36:
    // White, Yellow, Cyan, Green, Magenta, Red, Blue, Black
    const uint8_t barY[8]  = { 255, 226, 179, 150, 105,  76,  29,   0 };
    const uint8_t barRY[8] = { 128, 110,  16,   0, 239, 223, 146, 128 }; // R - Y chrominance
    const uint8_t barBY[8] = { 128,  16, 239, 128, 128,  16, 239, 128 }; // B - Y chrominance

    // 240 scanlines total
    for (int line = 0; line < 240; line++) {
        // Horizontal Sync pulse: 1200 Hz, 9.0 ms
        playToneMicros(1200.0f, 9000);

        // Sync Porch: 1500 Hz, 3.0 ms
        playToneMicros(1500.0f, 3000);

        // Y (Luminance) sweep: 88.0 ms (11.0 ms per color bar across 8 bars)
        for (int b = 0; b < 8; b++) {
            float yFreq = 1500.0f + ((float)barY[b] * 800.0f / 255.0f);
            playToneMicros(yFreq, 11000);
        }

        // Color difference scan (interleaved: R-Y on even lines, B-Y on odd lines)
        if (line % 2 == 0) {
            // Even line: R-Y Porch & Scan
            playToneMicros(1500.0f, 4500); // Separator
            playToneMicros(1900.0f, 1500); // Porch

            // R-Y sweep: 44.0 ms (5.5 ms per bar)
            for (int b = 0; b < 8; b++) {
                float cFreq = 1500.0f + ((float)barRY[b] * 800.0f / 255.0f);
                playToneMicros(cFreq, 5500);
            }
        } else {
            // Odd line: B-Y Porch & Scan
            playToneMicros(2300.0f, 4500); // Separator
            playToneMicros(1900.0f, 1500); // Porch

            // B-Y sweep: 44.0 ms (5.5 ms per bar)
            for (int b = 0; b < 8; b++) {
                float cFreq = 1500.0f + ((float)barBY[b] * 800.0f / 255.0f);
                playToneMicros(cFreq, 5500);
            }
        }
    }

    // End-of-transmission tone
    playTone(1200.0f, 50);
}
