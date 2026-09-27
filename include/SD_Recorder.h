#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include <SPI.h>
#include <vector>

struct WavFileInfo {
    String filename;
    uint32_t sizeBytes;
    float durationSec;
};

class SD_Recorder {
public:
    SD_Recorder();

    bool begin(uint8_t cs, uint8_t sck, uint8_t mosi, uint8_t miso);
    bool isAvailable() const { return cardAvailable; }

    // Recording Controls
    bool startRecording();
    void stopRecording();
    bool isRecordingActive() const { return recording; }
    
    // Audio Sample Feeder
    void writeSample(uint8_t sample);
    void update();

    // File Management
    String getCurrentFilename() const { return currentFilename; }
    uint32_t getBytesWritten() const { return totalDataBytesWritten; }
    std::vector<WavFileInfo> listFiles();
    bool deleteFile(const String& path);
    File openFile(const String& path, const char* mode);

    // Card Stats
    uint64_t getTotalCapacityMB();
    uint64_t getUsedSpaceMB();

private:
    uint8_t pinCS;
    uint8_t pinSCK;
    uint8_t pinMOSI;
    uint8_t pinMISO;

    bool cardAvailable;
    bool recording;
    String currentFilename;
    File wavFile;
    SPIClass sdSpi;

    uint32_t totalDataBytesWritten;
    static const size_t BUFFER_SIZE = 512;
    uint8_t sampleBuffer[BUFFER_SIZE];
    size_t bufferIndex;
    uint32_t lastFlushTime;

    void writeWavHeader();
    void finalizeWavHeader();
    String generateNextFilename();
};

extern SD_Recorder Recorder;
