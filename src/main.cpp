#include <Arduino.h>
#include "PinConfig.h"
#include "CC1101_Driver.h"
#include "SSTV_Demodulator.h"
#include "SSTV_Encoder.h"
#include "Doppler_Tracker.h"

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
    Serial.println(" Target Board  : ESP32-S3 DevKit");
    Serial.println(" Audio Output  : High-Speed PWM (GPIO 1)");
#else
    Serial.println(" Target Board  : ESP32-WROOM-32D");
    Serial.println(" Audio Output  : Hardware 8-bit DAC1 (GPIO 25)");
#endif
    Serial.printf(" Radio Center  : %.3f MHz\n", Doppler.getCurrentFreqMHz());
    Serial.printf(" Doppler Shift : %+.2f kHz\n", Doppler.getCurrentOffsetKHz());
    Serial.println("--------------------------------------------------------");
    Serial.println(" [1] Start ISS SSTV RX Mode (437.550 MHz -> Audio Out)");
    Serial.println(" [2] Step Doppler Frequency UP (+2.5 kHz)");
    Serial.println(" [3] Step Doppler Frequency DOWN (-2.5 kHz)");
    Serial.println(" [4] Start 10-Minute Auto-Doppler Satellite Pass");
    Serial.println(" [5] Transmit Robot 36 via CC1101 (433.92 MHz 2-FSK)");
    Serial.println(" [6] Transmit SSTV Calibration Tones (1200-2300 Hz)");
    Serial.println(" [7] Transmit Robot 36 via FS1000A (433.92 MHz ASK/OOK)");
    Serial.println(" [8] Radio Status & RSSI Diagnostic");
    Serial.println(" [0] Stop / Set Radio to IDLE");
    Serial.println("========================================================");
    Serial.print("Select command > ");
}

void setup() {
    Serial.begin(115200);
    delay(1000);

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

    Doppler.begin(437.550f, 10.0f);
    Encoder.begin(PIN_CC1101_GDO0);

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
            Serial.println("     Connect phone running Robot36 to audio output.");
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
            break;
        }

        case '0': {
            Serial.println("\n[STOP] Halting Demodulator & Setting Radio to IDLE.");
            Demodulator.stop();
            Radio.setIdle();
            Doppler.stopPass();
            currentMode = MODE_IDLE;
            printMenu();
            break;
        }

        default:
            printMenu();
            break;
    }
}

void loop() {
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

            Serial.printf("[RX-STATUS] Freq: %.3f MHz (%+.1f kHz) | RSSI: %3d dBm | Tone: %4.0f Hz %s\n",
                          Doppler.getCurrentFreqMHz(), Doppler.getCurrentOffsetKHz(),
                          rssi, toneHz, active ? "[TONE DETECTED]" : "[STATIC]");
        }
    }
}
