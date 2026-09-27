#include "SD_Recorder.h"

SD_Recorder Recorder;

#pragma pack(push, 1)
struct WavHeaderData {
    char riffTag[4];        // "RIFF"
    uint32_t riffSize;      // File size - 8
    char waveTag[4];        // "WAVE"
    char fmtTag[4];         // "fmt "
    uint32_t fmtSize;       // 16
    uint16_t audioFormat;   // 1 (PCM)
    uint16_t numChannels;   // 1 (Mono)
    uint32_t sampleRate;    // 11025
    uint32_t byteRate;      // 11025 * 1 * 1 = 11025
    uint16_t blockAlign;    // 1
    uint16_t bitsPerSample; // 8
    char dataTag[4];        // "data"
    uint32_t dataSize;      // Total audio bytes
};
#pragma pack(pop)

SD_Recorder::SD_Recorder()
    : pinCS(10), pinSCK(12), pinMOSI(11), pinMISO(13),
      cardAvailable(false), recording(false), currentFilename(""),
      totalDataBytesWritten(0), bufferIndex(0), lastFlushTime(0),
      sdSpi(FSPI) {}

bool SD_Recorder::begin(uint8_t cs, uint8_t sck, uint8_t mosi, uint8_t miso) {
    pinCS = cs;
    pinSCK = sck;
    pinMOSI = mosi;
    pinMISO = miso;

    pinMode(pinCS, OUTPUT);
    digitalWrite(pinCS, HIGH);

    sdSpi.begin(pinSCK, pinMISO, pinMOSI, pinCS);
    if (!SD.begin(pinCS, sdSpi, 20000000)) {
        cardAvailable = false;
        return false;
    }

    uint8_t cardType = SD.cardType();
    if (cardType == CARD_NONE) {
        cardAvailable = false;
        return false;
    }

    cardAvailable = true;
    return true;
}

String SD_Recorder::generateNextFilename() {
    for (int i = 1; i <= 999; i++) {
        char buf[32];
        snprintf(buf, sizeof(buf), "/sstv_pass_%03d.wav", i);
        if (!SD.exists(buf)) {
            return String(buf);
        }
    }
    return String("/sstv_pass_999.wav");
}

void SD_Recorder::writeWavHeader() {
    WavHeaderData header;
    memcpy(header.riffTag, "RIFF", 4);
    header.riffSize = 0; // Updated on close
    memcpy(header.waveTag, "WAVE", 4);
    memcpy(header.fmtTag, "fmt ", 4);
    header.fmtSize = 16;
    header.audioFormat = 1;   // Uncompressed PCM
    header.numChannels = 1;   // Mono
    header.sampleRate = 11025;
    header.bitsPerSample = 8;
    header.byteRate = 11025 * 1 * 1;
    header.blockAlign = 1;
    memcpy(header.dataTag, "data", 4);
    header.dataSize = 0; // Updated on close

    wavFile.write((const uint8_t*)&header, sizeof(header));
}

void SD_Recorder::finalizeWavHeader() {
    if (!wavFile) return;

    // Flush any remaining samples in RAM buffer
    if (bufferIndex > 0) {
        wavFile.write(sampleBuffer, bufferIndex);
        totalDataBytesWritten += bufferIndex;
        bufferIndex = 0;
    }

    uint32_t riffSize = totalDataBytesWritten + 36;

    // Seek and update RIFF size (offset 4)
    wavFile.seek(4);
    wavFile.write((const uint8_t*)&riffSize, 4);

    // Seek and update Data size (offset 40)
    wavFile.seek(40);
    wavFile.write((const uint8_t*)&totalDataBytesWritten, 4);

    wavFile.flush();
    wavFile.close();
}

bool SD_Recorder::startRecording() {
    if (!cardAvailable) return false;
    if (recording) stopRecording();

    currentFilename = generateNextFilename();
    wavFile = SD.open(currentFilename.c_str(), FILE_WRITE);
    if (!wavFile) {
        recording = false;
        return false;
    }

    totalDataBytesWritten = 0;
    bufferIndex = 0;
    writeWavHeader();

    recording = true;
    lastFlushTime = millis();
    return true;
}

void SD_Recorder::stopRecording() {
    if (!recording) return;
    finalizeWavHeader();
    recording = false;
}

void SD_Recorder::writeSample(uint8_t sample) {
    if (!recording || !wavFile) return;

    sampleBuffer[bufferIndex++] = sample;
    if (bufferIndex >= BUFFER_SIZE) {
        wavFile.write(sampleBuffer, BUFFER_SIZE);
        totalDataBytesWritten += BUFFER_SIZE;
        bufferIndex = 0;
        lastFlushTime = millis();
    }
}

void SD_Recorder::update() {
    if (!recording) return;

    // Flush remaining buffer every 1 second
    if (millis() - lastFlushTime > 1000 && bufferIndex > 0) {
        wavFile.write(sampleBuffer, bufferIndex);
        totalDataBytesWritten += bufferIndex;
        bufferIndex = 0;
        wavFile.flush();
        lastFlushTime = millis();
    }
}

std::vector<WavFileInfo> SD_Recorder::listFiles() {
    std::vector<WavFileInfo> list;
    if (!cardAvailable) return list;

    File root = SD.open("/");
    if (!root || !root.isDirectory()) return list;

    File file = root.openNextFile();
    while (file) {
        String name = file.name();
        if (name.endsWith(".wav") || name.endsWith(".WAV")) {
            WavFileInfo info;
            info.filename = name.startsWith("/") ? name : ("/" + name);
            info.sizeBytes = file.size();
            // 11025 samples per sec, 1 byte per sample
            info.durationSec = info.sizeBytes > 44 ? (float)(info.sizeBytes - 44) / 11025.0f : 0.0f;
            list.push_back(info);
        }
        file = root.openNextFile();
    }
    return list;
}

bool SD_Recorder::deleteFile(const String& path) {
    if (!cardAvailable) return false;
    return SD.remove(path.c_str());
}

File SD_Recorder::openFile(const String& path, const char* mode) {
    if (!cardAvailable) return File();
    return SD.open(path.c_str(), mode);
}

uint64_t SD_Recorder::getTotalCapacityMB() {
    if (!cardAvailable) return 0;
    return SD.totalBytes() / (1024 * 1024);
}

uint64_t SD_Recorder::getUsedSpaceMB() {
    if (!cardAvailable) return 0;
    return SD.usedBytes() / (1024 * 1024);
}
