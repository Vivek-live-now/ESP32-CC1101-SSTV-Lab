#pragma once
#include <Arduino.h>

#if defined(BOARD_ESP32_S3)
    // ESP32-S3 DevKit Pin Mapping
    #define PIN_CC1101_CSN     10
    #define PIN_CC1101_SCK     12
    #define PIN_CC1101_MOSI    11
    #define PIN_CC1101_MISO    13
    #define PIN_CC1101_GDO0    14
    #define PIN_CC1101_GDO2    15

    #define PIN_FS1000A_DATA   16
    #define PIN_AUDIO_PWM      1       // S3 has no DAC; uses high-speed LEDC PWM for audio
    #define HAS_HARDWARE_DAC   false

#else
    // Default: ESP32-WROOM-32D Pin Mapping
    #define PIN_CC1101_CSN     5
    #define PIN_CC1101_SCK     18
    #define PIN_CC1101_MOSI    23
    #define PIN_CC1101_MISO    19
    #define PIN_CC1101_GDO0    4       // Connected to GDO0 for edge-capture demodulation
    #define PIN_CC1101_GDO2    2

    #define PIN_FS1000A_DATA   26      // Optional FS1000A 433.92 MHz transmitter pin
    #define PIN_AUDIO_DAC      25      // Built-in hardware DAC1 (Channel 1)
    #define HAS_HARDWARE_DAC   true
#endif

// Onboard BOOT button (Active LOW, GPIO 0 on both WROOM-32 and ESP32-S3)
#define PIN_BOOT_BUTTON        0

