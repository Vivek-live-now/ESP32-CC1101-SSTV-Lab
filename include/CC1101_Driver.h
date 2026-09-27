#pragma once
#include <Arduino.h>
#include <SPI.h>

// CC1101 Configuration Registers
#define CC1101_IOCFG2       0x00
#define CC1101_IOCFG1       0x01
#define CC1101_IOCFG0       0x02
#define CC1101_FIFOTHR      0x03
#define CC1101_SYNC1        0x04
#define CC1101_SYNC0        0x05
#define CC1101_PKTLEN       0x06
#define CC1101_PKTCTRL1     0x07
#define CC1101_PKTCTRL0     0x08
#define CC1101_ADDR         0x09
#define CC1101_CHANNR       0x0A
#define CC1101_FSCTRL1      0x0B
#define CC1101_FSCTRL0      0x0C
#define CC1101_FREQ2        0x0D
#define CC1101_FREQ1        0x0E
#define CC1101_FREQ0        0x0F
#define CC1101_MDMCFG4      0x10
#define CC1101_MDMCFG3      0x11
#define CC1101_MDMCFG2      0x12
#define CC1101_MDMCFG1      0x13
#define CC1101_MDMCFG0      0x14
#define CC1101_DEVIATN      0x15
#define CC1101_MCSM2        0x16
#define CC1101_MCSM1        0x17
#define CC1101_MCSM0        0x18
#define CC1101_FOCCFG       0x19
#define CC1101_BSCFG        0x1A
#define CC1101_AGCCTRL2     0x1B
#define CC1101_AGCCTRL1     0x1C
#define CC1101_AGCCTRL0     0x1D
#define CC1101_WOREVT1      0x1E
#define CC1101_WOREVT0      0x1F
#define CC1101_WORCTRL      0x20
#define CC1101_FREND1       0x21
#define CC1101_FREND0       0x22
#define CC1101_FSCAL3       0x23
#define CC1101_FSCAL2       0x24
#define CC1101_FSCAL1       0x25
#define CC1101_FSCAL0       0x26
#define CC1101_RCCTRL1      0x27
#define CC1101_RCCTRL0      0x28
#define CC1101_FSTEST       0x29
#define CC1101_PTEST        0x2A
#define CC1101_AGCTEST      0x2B
#define CC1101_TEST2        0x2C
#define CC1101_TEST1        0x2D
#define CC1101_TEST0        0x2E

// Status Registers (Read with burst bit 0xC0)
#define CC1101_PARTNUM      0x30
#define CC1101_VERSION      0x31
#define CC1101_FREQEST      0x32
#define CC1101_LQI          0x33
#define CC1101_RSSI         0x34
#define CC1101_MARCSTATE    0x35
#define CC1101_PKTSTATUS    0x38

// Command Strobes
#define CC1101_SRES         0x30
#define CC1101_SFSTXON      0x31
#define CC1101_SXOFF        0x32
#define CC1101_SCAL         0x33
#define CC1101_SRX          0x34
#define CC1101_STX          0x35
#define CC1101_SIDLE        0x36
#define CC1101_SWOR         0x38
#define CC1101_SPWD         0x39
#define CC1101_SFRX         0x3A
#define CC1101_SFTX         0x3B
#define CC1101_SNOP         0x3D
#define CC1101_PATABLE      0x3E

// Burst bit
#define CC1101_WRITE_BURST  0x40
#define CC1101_READ_SINGLE  0x80
#define CC1101_READ_BURST   0xC0

class CC1101_Driver {
public:
    CC1101_Driver();

    bool begin(uint8_t csn, uint8_t sck, uint8_t mosi, uint8_t miso);
    void reset();
    
    // Carrier & Frequency Synthesis
    void setFrequency(float freqMHz);
    float getFrequency() const { return currentFrequencyMHz; }
    
    // Operating Modes
    void setRxAsyncDiscriminator(float freqMHz = 437.550f, float devKHz = 5.0f, float bwKHz = 100.0f);
    void setTxAsyncMode(float freqMHz = 433.920f, float devKHz = 5.0f, int8_t powerIndex = 0);
    void setIdle();
    
    // Power & Status
    void setOutputPower(uint8_t paTableByte);
    int8_t getRSSI();
    uint8_t getPartNum();
    uint8_t getVersion();
    uint8_t getMarcState();
    
    // Low-level SPI
    void writeReg(uint8_t reg, uint8_t value);
    uint8_t readReg(uint8_t reg);
    uint8_t readStatus(uint8_t reg);
    uint8_t cmdStrobe(uint8_t cmd);

private:
    uint8_t pinCSN;
    uint8_t pinSCK;
    uint8_t pinMOSI;
    uint8_t pinMISO;
    float currentFrequencyMHz;
    SPIClass spi;

    void select();
    void deselect();
    void waitMisoLow();
};

extern CC1101_Driver Radio;
