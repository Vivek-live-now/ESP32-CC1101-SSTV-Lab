#include "CC1101_Driver.h"

CC1101_Driver Radio;

CC1101_Driver::CC1101_Driver()
    : pinCSN(5), pinSCK(18), pinMOSI(23), pinMISO(19), currentFrequencyMHz(437.550f) {}

void CC1101_Driver::select() {
    digitalWrite(pinCSN, LOW);
}

void CC1101_Driver::deselect() {
    digitalWrite(pinCSN, HIGH);
}

void CC1101_Driver::waitMisoLow() {
    uint32_t start = micros();
    while (digitalRead(pinMISO) == HIGH) {
        if (micros() - start > 5000) break; // 5ms timeout
    }
}

uint8_t CC1101_Driver::cmdStrobe(uint8_t cmd) {
    select();
    waitMisoLow();
    uint8_t status = spi.transfer(cmd);
    deselect();
    return status;
}

void CC1101_Driver::writeReg(uint8_t reg, uint8_t value) {
    select();
    waitMisoLow();
    spi.transfer(reg);
    spi.transfer(value);
    deselect();
}

uint8_t CC1101_Driver::readReg(uint8_t reg) {
    select();
    waitMisoLow();
    spi.transfer(reg | CC1101_READ_SINGLE);
    uint8_t val = spi.transfer(0x00);
    deselect();
    return val;
}

uint8_t CC1101_Driver::readStatus(uint8_t reg) {
    select();
    waitMisoLow();
    spi.transfer(reg | CC1101_READ_BURST);
    uint8_t val = spi.transfer(0x00);
    deselect();
    return val;
}

void CC1101_Driver::reset() {
    deselect();
    delayMicroseconds(10);
    select();
    delayMicroseconds(20);
    deselect();
    delayMicroseconds(50);

    select();
    waitMisoLow();
    spi.transfer(CC1101_SRES);
    deselect();
    delay(10);
}

bool CC1101_Driver::begin(uint8_t csn, uint8_t sck, uint8_t mosi, uint8_t miso) {
    pinCSN = csn;
    pinSCK = sck;
    pinMOSI = mosi;
    pinMISO = miso;

    pinMode(pinCSN, OUTPUT);
    pinMode(pinMISO, INPUT);
    deselect();

    spi.begin(pinSCK, pinMISO, pinMOSI, -1);
    spi.beginTransaction(SPISettings(5000000, MSBFIRST, SPI_MODE0));

    reset();

    uint8_t part = getPartNum();
    uint8_t ver = getVersion();

    // CC1101 typically returns PartNum=0x00 and Version=0x14 (or 0x04)
    if (part != 0x00 && ver != 0x14 && ver != 0x04) {
        return false;
    }

    return true;
}

void CC1101_Driver::setFrequency(float freqMHz) {
    currentFrequencyMHz = freqMHz;
    
    // Formula: FREQ = (freqMHz * 2^16) / 26.0 MHz (for 26MHz crystal)
    uint32_t freqReg = (uint32_t)((freqMHz * 65536.0f) / 26.0f + 0.5f);

    uint8_t f2 = (freqReg >> 16) & 0xFF;
    uint8_t f1 = (freqReg >> 8) & 0xFF;
    uint8_t f0 = freqReg & 0xFF;

    writeReg(CC1101_FREQ2, f2);
    writeReg(CC1101_FREQ1, f1);
    writeReg(CC1101_FREQ0, f0);

    // Strobe frequency calibration
    cmdStrobe(CC1101_SIDLE);
    cmdStrobe(CC1101_SCAL);
    delayMicroseconds(800);
}

void CC1101_Driver::setRxAsyncDiscriminator(float freqMHz, float devKHz, float bwKHz) {
    cmdStrobe(CC1101_SIDLE);

    setFrequency(freqMHz);

    // Base RF modem configuration for 2-FSK
    writeReg(CC1101_FSCTRL1, 0x06); // IF frequency = 152.34 kHz
    writeReg(CC1101_FSCTRL0, 0x00);

    // MDMCFG4: Channel Filter Bandwidth (~100 kHz)
    // CHANBW_E = 2, CHANBW_M = 3 -> ~101 kHz filter
    writeReg(CC1101_MDMCFG4, 0x8C);
    writeReg(CC1101_MDMCFG3, 0x22); // Symbol rate

    // MDMCFG2: 2-FSK modulation (bits 6:4 = 000), no sync word (bits 2:0 = 000)
    writeReg(CC1101_MDMCFG2, 0x00);
    writeReg(CC1101_MDMCFG1, 0x22); // 4 preamble bytes, no FEC
    writeReg(CC1101_MDMCFG0, 0xF8);

    // DEVIATN: ~5.15 kHz deviation (matching standard amateur FM voice/SSTV deviation)
    writeReg(CC1101_DEVIATN, 0x15);

    // MCSM0: Auto-calibrate when going from IDLE to RX/TX
    writeReg(CC1101_MCSM0, 0x18);
    writeReg(CC1101_FOCCFG, 0x16);
    writeReg(CC1101_AGCCTRL2, 0x43);
    writeReg(CC1101_AGCCTRL1, 0x40);
    writeReg(CC1101_AGCCTRL0, 0x91);

    // PKTCTRL0: Asynchronous serial mode! (Bits 5:4 = 11 -> 0x32)
    // Data stream directly routed to/from GDO pins, bypassing packet handler
    writeReg(CC1101_PKTCTRL0, 0x32);
    writeReg(CC1101_PKTCTRL1, 0x00);

    // IOCFG0: 0x0D = Serial Data Output (unsynchronized raw demodulator output)
    writeReg(CC1101_IOCFG0, 0x0D);

    // Flush FIFOs and switch to RX
    cmdStrobe(CC1101_SFRX);
    cmdStrobe(CC1101_SRX);
}

void CC1101_Driver::setTxAsyncMode(float freqMHz, float devKHz, int8_t powerIndex) {
    cmdStrobe(CC1101_SIDLE);

    setFrequency(freqMHz);

    writeReg(CC1101_FSCTRL1, 0x06);
    writeReg(CC1101_FSCTRL0, 0x00);

    // 2-FSK modulation
    writeReg(CC1101_MDMCFG2, 0x00);

    // ~5 kHz deviation
    writeReg(CC1101_DEVIATN, 0x15);

    // PKTCTRL0: Asynchronous serial mode (0x32)
    writeReg(CC1101_PKTCTRL0, 0x32);

    // Set IOCFG0 to high-impedance input or Serial Data Input for TX
    writeReg(CC1101_IOCFG0, 0x2E);

    // Output power table: 0xC0 = +10dBm, 0x60 = 0dBm, 0x12 = -10dBm, 0x03 = -30dBm
    // Default to ultra-low safe power for lab experiments: -10dBm
    writeReg(CC1101_PATABLE, 0x12);

    cmdStrobe(CC1101_SFTX);
    cmdStrobe(CC1101_STX);
}

void CC1101_Driver::setIdle() {
    cmdStrobe(CC1101_SIDLE);
}

void CC1101_Driver::setOutputPower(uint8_t paTableByte) {
    writeReg(CC1101_PATABLE, paTableByte);
}

int8_t CC1101_Driver::getRSSI() {
    uint8_t rawRssi = readStatus(CC1101_RSSI);
    int16_t rssi;
    if (rawRssi >= 128) {
        rssi = (int16_t)((int16_t)rawRssi - 256) / 2 - 74;
    } else {
        rssi = (int16_t)rawRssi / 2 - 74;
    }
    return (int8_t)rssi;
}

uint8_t CC1101_Driver::getPartNum() {
    return readStatus(CC1101_PARTNUM);
}

uint8_t CC1101_Driver::getVersion() {
    return readStatus(CC1101_VERSION);
}

uint8_t CC1101_Driver::getMarcState() {
    return readStatus(CC1101_MARCSTATE) & 0x1F;
}
