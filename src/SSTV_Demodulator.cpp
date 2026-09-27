#include "SSTV_Demodulator.h"
#include <cmath>

SSTV_Demodulator Demodulator;

volatile uint32_t SSTV_Demodulator::lastEdgeTimeMicros = 0;
volatile uint32_t SSTV_Demodulator::currentPeriodMicros = 0;
volatile bool SSTV_Demodulator::newPeriodAvailable = false;

void IRAM_ATTR SSTV_Demodulator::handleEdgeISR() {
    uint32_t now = micros();
    uint32_t dt = now - lastEdgeTimeMicros;
    lastEdgeTimeMicros = now;

    // Bandpass pre-filter for SSTV tones (1100 Hz to 2400 Hz)
    // 2400 Hz = 416 us, 1100 Hz = 909 us
    // Reject high-frequency noise spikes (< 380 us) and LF noise (> 980 us)
    if (dt >= 380 && dt <= 980) {
        currentPeriodMicros = dt;
        newPeriodAvailable = true;
    }
}

SSTV_Demodulator::SSTV_Demodulator()
    : inputPin(4), outputPin(25), running(false), currentFrequencyHz(0.0f),
      validToneCount(0), toneActive(false), lastToneActivityMillis(0),
      phase(0.0f), lastDacUpdateMicros(0), sampleCallback(nullptr) {}

void SSTV_Demodulator::begin(uint8_t inPin, uint8_t outPin) {
    inputPin = inPin;
    outputPin = outPin;
    running = true;
    currentFrequencyHz = 0.0f;
    validToneCount = 0;
    toneActive = false;
    lastToneActivityMillis = millis();

    pinMode(inputPin, INPUT);

#if HAS_HARDWARE_DAC
    // Enable built-in DAC on GPIO 25 (DAC channel 1)
    dacWrite(outputPin, 128); // Midpoint DC bias
#else
    // Configure LEDC PWM channel on ESP32-S3
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcAttach(outputPin, 1000, 8);
    #else
    ledcSetup(0, 1000, 8);
    ledcAttachPin(outputPin, 0);
    ledcWrite(0, 0);
    #endif
#endif

    lastEdgeTimeMicros = micros();
    attachInterrupt(digitalPinToInterrupt(inputPin), handleEdgeISR, RISING);
}

void SSTV_Demodulator::stop() {
    running = false;
    detachInterrupt(digitalPinToInterrupt(inputPin));
#if HAS_HARDWARE_DAC
    dacWrite(outputPin, 0);
#else
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWrite(outputPin, 0);
    #else
    ledcWrite(0, 0);
    #endif
#endif
}

void SSTV_Demodulator::update() {
    if (!running) return;

    if (newPeriodAvailable) {
        newPeriodAvailable = false;
        uint32_t period = currentPeriodMicros;

        if (period > 0) {
            float freq = 1000000.0f / (float)period;
            // Exponential moving average filter for smoothing
            currentFrequencyHz = (currentFrequencyHz * 0.7f) + (freq * 0.3f);
            validToneCount++;
            toneActive = true;
            lastToneActivityMillis = millis();
        }
    }

    // Tone activity timeout (if no valid edge for 50ms, squelch audio)
    if (millis() - lastToneActivityMillis > 50) {
        toneActive = false;
    }

    outputToneSample(toneActive ? currentFrequencyHz : 0.0f);
}

void SSTV_Demodulator::outputToneSample(float freq) {
    uint32_t now = micros();
    uint32_t dt = now - lastDacUpdateMicros;
    if (dt < 25) return; // ~40 kHz sample rate limiter
    lastDacUpdateMicros = now;

    if (freq < 1000.0f || freq > 2500.0f) {
#if HAS_HARDWARE_DAC
        dacWrite(outputPin, 128); // Squelch / silence DC center
#else
        ledcWrite(0, 0);
#endif
        return;
    }

#if HAS_HARDWARE_DAC
    // Phase accumulation for pure reconstructed sine wave
    float phaseIncrement = (2.0f * 3.14159265f * freq * (float)dt) / 1000000.0f;
    phase += phaseIncrement;
    if (phase >= 2.0f * 3.14159265f) phase -= 2.0f * 3.14159265f;

    // 8-bit DAC output (0 - 255)
    uint8_t dacValue = (uint8_t)(128.0f + 110.0f * sinf(phase));
#if HAS_HARDWARE_DAC
    dacWrite(outputPin, dacValue);
#else
    // Hardware PWM tone generation on ESP32-S3
    #if defined(ESP_ARDUINO_VERSION_MAJOR) && (ESP_ARDUINO_VERSION_MAJOR >= 3)
    ledcWriteTone(outputPin, (uint32_t)freq);
    #else
    ledcWriteTone(0, (uint32_t)freq);
    #endif
#endif

    if (sampleCallback) {
        sampleCallback(dacValue);
    }
}
