#include <Arduino.h>
#include "PinConfig.h"
#include "CC1101_Driver.h"
#include "SSTV_Demodulator.h"
#include "SSTV_Encoder.h"
#include "Doppler_Tracker.h"
#include "SD_Recorder.h"
#include "WebPortal.h"

enum OperatingMode {
    MODE_IDLE,
    MODE_RX_DISCRIMINATOR,
    MODE_TX_TEST_PATTERN
};

OperatingMode currentMode = MODE_IDLE;
uint32_t lastTelemetryMillis = 0;

void printMenu() {
    Serial.println("\n========================================================");
    Serial.println("       ESP32-CC1101 SSTV Satellite & RF Lab             ");
    Serial.println("========================================================");
#if defined(BOARD_ESP32_S3)
    Serial.println(" Target Board  : ESP32-S3 DevKit (N16R8)");
    Serial.println(" Audio Output  : High-Speed PWM (GPIO 1)");
#else
    Serial.println(" Target Board  : ESP32-WROOM-32D");
    Serial.println(" Audio Output  : Hardware 8-bit DAC1 (GPIO 25)");
#endif
    Serial.printf(" Radio Center  : %.3f MHz\n", Doppler.getCurrentFreqMHz());
    Serial.printf(" Doppler Shift : %+.2f kHz\n", Doppler.getCurrentOffsetKHz());
    Serial.printf(" SD Card Status: %s\n", Recorder.isAvailable() ? "READY (Auto-Record Enabled)" : "NOT DETECTED");
    Serial.printf(" Web Portal    : http://esp-sstv.local (AP: ESP-SSTV-Station)\n");
    Serial.println("--------------------------------------------------------");
    Serial.println(" [1] Start ISS SSTV RX Mode & Record WAV (437.550 MHz)");
    Serial.println(" [2] Step Doppler Frequency UP (+2.5 kHz)");
    Serial.println(" [3] Step Doppler Frequency DOWN (-2.5 kHz)");
    Serial.println(" [4] Start 10-Minute Auto-Doppler Satellite Pass");
    Serial.println(" [5] Transmit Robot 36 via CC1101 (433.92 MHz 2-FSK)");
    Serial.println(" [6] Transmit SSTV Calibration Tones (1200-2300 Hz)");
    Serial.println(" [7] Transmit Robot 36 via FS1000A (433.92 MHz ASK/OOK)");
    Serial.println(" [8] Radio Status & RSSI Diagnostic");
    Serial.println(" [0] Stop / Set Radio to IDLE & Finalize WAV");
    Serial.println("--------------------------------------------------------");
    Serial.println(" [BOOT Button] Click: Start/Stop RX & Record | Hold: Transmit");
    Serial.println("========================================================");
    Serial.print("Select command > ");
}

void handleCommand(char cmd);

void setup() {
    Serial.begin(115200);
    delay(1000);

    pinMode(PIN_BOOT_BUTTON, INPUT_PULLUP);

    Serial.println("\n[INIT] Initializing SPI and CC1101...");

    bool ok = Radio.begin(PIN_CC1101_CSN, PIN_CC1101_SCK, PIN_CC1101_MOSI, PIN_CC1101_MISO);
    if (!ok) {
        Serial.println("[ERROR] CC1101 not detected! Check wiring:");
        Serial.printf("  CSN:  GPIO %d\n", PIN_CC1101_CSN);
        Serial.printf("  SCK:  GPIO %d\n", PIN_CC1101_SCK);
        Serial.printf("  MOSI: GPIO %d\n", PIN_CC1101_MOSI);
        Serial.printf("  MISO: GPIO %d\n", PIN_CC1101_MISO);
    } else {
        Serial.printf("[OK] CC1101 detected! Part: 0x%02X, Version: 0x%02X\n", 
                      Radio.getPartNum(), Radio.getVersion());
    }

    // Initialize MicroSD card
    Serial.println("[INIT] Initializing MicroSD card...");
    bool sdOk = Recorder.begin(PIN_SD_CS, PIN_SD_SCK, PIN_SD_MOSI, PIN_SD_MISO);
    if (sdOk) {
        Serial.printf("[OK] MicroSD Card mounted! Total Capacity: %llu MB\n", Recorder.getTotalCapacityMB());
    } else {
        Serial.println("[WARN] No MicroSD card detected. Audio recording disabled.");
    }

    // Route demodulated audio stream into SD WAV recorder
    Demodulator.setSampleCallback([](uint8_t sample) {
        if (Recorder.isRecordingActive()) {
            Recorder.writeSample(sample);
        }
    });

    Doppler.begin(437.550f, 10.0f);
    Encoder.begin(PIN_CC1101_GDO0);

    // Initialize Web Captive Portal & mDNS (http://esp-sstv.local)
    Serial.println("[INIT] Starting WiFi Captive Portal & mDNS responder...");
    Portal.begin(handleCommand);
    Serial.printf("[WIFI] Captive Portal active on AP: 'ESP-SSTV-Station' (IP: %s)\n", Portal.getAPIP().c_str());
    if (Portal.isConnectedToStation()) {
        Serial.printf("[WIFI] Connected to Home Network! Station IP: %s\n", Portal.getStationIP().c_str());
    }
    Serial.println("[mDNS] Dashboard available at: http://esp-sstv.local/");

    printMenu();
}

void handleCommand(char cmd) {
    switch (cmd) {
        case '1': {
            Serial.println("\n[RX] Starting ISS SSTV Discriminator Mode...");
            Serial.printf("     Tuning CC1101 to %.3f MHz (2-FSK Async)...\n", Doppler.getCurrentFreqMHz());
            Radio.setRxAsyncDiscriminator(Doppler.getCurrentFreqMHz(), 5.0f, 100.0f);

#if HAS_HARDWARE_DAC
            Demodulator.begin(PIN_CC1101_GDO0, PIN_AUDIO_DAC);
            Serial.println("     Reconstructed audio stream active on DAC (GPIO 25)");
#else
            Demodulator.begin(PIN_CC1101_GDO0, PIN_AUDIO_PWM);
            Serial.println("     Audio stream active on PWM (GPIO 1)");
#endif
            // Start recording to SD card if card is available
            if (Recorder.isAvailable()) {
                if (Recorder.startRecording()) {
                    Serial.printf("     [SD-REC] Recording pass audio to: %s\n", Recorder.getCurrentFilename().c_str());
                }
            }

            Serial.println("     Connect phone running Robot36 or download WAV from Web Portal.");
            currentMode = MODE_RX_DISCRIMINATOR;
            break;
        }

        case '2': {
            Doppler.stepUp(Radio, 2.5f);
            Serial.printf("\n[FREQ] Frequency stepped up to %.3f MHz (Offset: %+.2f kHz)\n",
                          Doppler.getCurrentFreqMHz(), Doppler.getCurrentOffsetKHz());
            break;
        }

        case '3': {
            Doppler.stepDown(Radio, 2.5f);
            Serial.printf("\n[FREQ] Frequency stepped down to %.3f MHz (Offset: %+.2f kHz)\n",
                          Doppler.getCurrentFreqMHz(), Doppler.getCurrentOffsetKHz());
            break;
        }

        case '4': {
            Serial.println("\n[PASS] Starting 10-Minute Auto Doppler Pass tracking...");
            Doppler.startPass(600);
            Serial.printf("       Current Frequency: %.3f MHz\n", Doppler.getCurrentFreqMHz());
            break;
        }

        case '5': {
            Serial.println("\n[TX] Configuring CC1101 for Asynchronous TX @ 433.920 MHz (Low Power)...");
            Demodulator.stop();
            if (Recorder.isRecordingActive()) Recorder.stopRecording();
            Radio.setTxAsyncMode(433.920f, 5.0f, 0); // -10 dBm safe lab power

            Encoder.setModulationPin(PIN_CC1101_GDO0);
            Serial.println("[TX] Transmitting Robot 36 Test Pattern via CC1101 (~36 seconds)...");
            Serial.println("     Open Robot36 on your phone to capture!");
            Encoder.sendRobot36TestPattern("ESP32-CC1101");

            Serial.println("[TX] Transmission complete! Returning to IDLE.");
            Radio.setIdle();
            currentMode = MODE_IDLE;
            printMenu();
            break;
        }

        case '6': {
            Serial.println("\n[TEST] Transmitting SSTV calibration tones (1200 / 1500 / 1900 / 2300 Hz)...");
            Demodulator.stop();
            if (Recorder.isRecordingActive()) Recorder.stopRecording();
            Radio.setTxAsyncMode(433.920f, 5.0f, 0);
            Encoder.setModulationPin(PIN_CC1101_GDO0);
            Encoder.sendToneTest();
            Radio.setIdle();
            Serial.println("[TEST] Done!");
            printMenu();
            break;
        }

        case '7': {
            Serial.printf("\n[TX-FS1000A] Transmitting Robot 36 via FS1000A on GPIO %d (433.92 MHz ASK/OOK)...\n", PIN_FS1000A_DATA);
            Demodulator.stop();
            if (Recorder.isRecordingActive()) Recorder.stopRecording();
            Radio.setIdle();

            Encoder.setModulationPin(PIN_FS1000A_DATA);
            Serial.println("             Transmitting Robot 36 Pattern (~36 seconds)...");
            Encoder.sendRobot36TestPattern("ESP32-FS1000A");

            Serial.println("[TX-FS1000A] Transmission complete!");
            currentMode = MODE_IDLE;
            printMenu();
            break;
        }

        case '8': {
            Serial.println("\n[DIAGNOSTICS]");
            Serial.printf("  PartNum   : 0x%02X\n", Radio.getPartNum());
            Serial.printf("  Version   : 0x%02X\n", Radio.getVersion());
            Serial.printf("  MARCSTATE : 0x%02X\n", Radio.getMarcState());
            Serial.printf("  RSSI      : %d dBm\n", Radio.getRSSI());
            Serial.printf("  Frequency : %.3f MHz\n", Radio.getFrequency());
            Serial.printf("  SD Card   : %s (Total: %llu MB, Used: %llu MB)\n", 
                          Recorder.isAvailable() ? "YES" : "NO",
                          Recorder.getTotalCapacityMB(), Recorder.getUsedSpaceMB());
            Serial.printf("  WiFi Mode : %s | Portal: http://esp-sstv.local\n",
                          Portal.isConnectedToStation() ? "Connected (STA+AP)" : "Captive Portal (AP)");
            break;
        }

        case '0': {
            Serial.println("\n[STOP] Halting Demodulator & Finalizing Recording.");
            Demodulator.stop();
            Radio.setIdle();
            Doppler.stopPass();

            if (Recorder.isRecordingActive()) {
                Recorder.stopRecording();
                Serial.printf("       Saved WAV file: %s (%u bytes)\n", 
                              Recorder.getCurrentFilename().c_str(), Recorder.getBytesWritten());
            }

            currentMode = MODE_IDLE;
            printMenu();
            break;
        }

        default:
            printMenu();
            break;
    }
}

void checkBootButton() {
    static uint32_t pressStartTime = 0;
    static bool buttonWasPressed = false;

    bool isPressed = (digitalRead(PIN_BOOT_BUTTON) == LOW);

    if (isPressed && !buttonWasPressed) {
        // Button pressed down
        buttonWasPressed = true;
        pressStartTime = millis();
    } else if (!isPressed && buttonWasPressed) {
        // Button released
        buttonWasPressed = false;
        uint32_t duration = millis() - pressStartTime;

        if (duration >= 50 && duration < 1500) {
            // Short click: 1-Click Satellite Pass Start/Stop!
            if (currentMode == MODE_IDLE) {
                Serial.println("\n[BOOT BUTTON] 1-Click -> Starting ISS SSTV RX Mode, Auto Doppler & SD Recording!");
                handleCommand('1');
                handleCommand('4');
            } else {
                Serial.println("\n[BOOT BUTTON] 1-Click -> Halting RX Mode & Finalizing Recording");
                handleCommand('0');
            }
        } else if (duration >= 1500) {
            // Long hold (> 1.5s): Transmit Robot 36 Test Pattern
            Serial.println("\n[BOOT BUTTON] Hold -> Transmitting Robot 36 Test Pattern!");
            handleCommand('5');
        }
    }
}

void loop() {
    // Process Captive Portal & Web Server
    Portal.update();

    // Process SD WAV buffer flushing
    Recorder.update();

    // Check hardware BOOT button
    checkBootButton();

    // Process incoming Serial commands
    if (Serial.available()) {
        char c = (char)Serial.read();
        if (c != '\r' && c != '\n') {
            handleCommand(c);
        }
    }

    // Demodulator real-time processing
    if (currentMode == MODE_RX_DISCRIMINATOR) {
        Demodulator.update();
        Doppler.update(Radio);

        // Telemetry every 1.5 seconds
        if (millis() - lastTelemetryMillis > 1500) {
            lastTelemetryMillis = millis();
            int8_t rssi = Radio.getRSSI();
            float toneHz = Demodulator.getInstantaneousFrequency();
            bool active = Demodulator.isReceivingTone();

            Serial.printf("[RX-STATUS] Freq: %.3f MHz (%+.1f kHz) | RSSI: %3d dBm | Tone: %4.0f Hz %s | Rec: %u KB\n",
                          Doppler.getCurrentFreqMHz(), Doppler.getCurrentOffsetKHz(),
                          rssi, toneHz, active ? "[TONE DETECTED]" : "[STATIC]",
                          Recorder.isRecordingActive() ? Recorder.getBytesWritten() / 1024 : 0);
        }
    }
}
