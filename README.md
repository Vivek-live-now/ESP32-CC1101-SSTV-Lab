# ESP32-CC1101 SSTV Satellite Receiver & RF Lab

An experimental PlatformIO project enabling **Slow Scan Television (SSTV)** reception and transmission using the Texas Instruments **CC1101** Sub-1 GHz transceiver, an **ESP32-WROOM-32D** (utilizing its internal 8-bit DAC), and an **ESP32-S3 DevKit**.

Designed for experimenting with amateur radio satellite passes—specifically **ARISS ISS SSTV (Series 33 / Expedition 75)** on **437.550 MHz**—as well as generating standard **Robot 36** video test patterns on license-free 433 MHz ISM channels.

---

## Features

* **Zero-Crossing Asynchronous FM Demodulator:** Bypasses the CC1101 packet engine and configures 2-FSK raw discriminator output on `GDO0`. The ESP32 measures edge pulse intervals ($\Delta t$) to compute the instantaneous audio frequency ($1200\text{ Hz} - 2300\text{ Hz}$).
* **Hardware DAC Audio Regeneration:** On the ESP32-WROOM-32D, reconstructed SSTV audio tones are synthesized directly via the built-in 8-bit DAC (`GPIO 25`), allowing direct connection to headphones or your phone's microphone running [Robot36](https://play.google.com/store/apps/details?id=xdsopl.robot36).
* **Real-time Doppler Tracker:** Compensates for Low Earth Orbit satellite velocity ($27,600\text{ km/h}$). Tracks the $\pm 10\text{ kHz}$ shift from Acquisition of Signal (AOS at $437.560\text{ MHz}$) down to Loss of Signal (LOS at $437.540\text{ MHz}$) either manually or via automated 10-minute pass curve.
* **Full Robot 36 Video Frame Generator:** Includes an SSTV encoder with VIS calibration header (0x08), 1200 Hz sync pulses, and 240 interleaved Y / R-Y / B-Y scanlines to transmit real color images to test your decoder locally.
* **Dual-Target PlatformIO Configuration:** Out-of-the-box support for both `esp32dev` (WROOM-32D with DAC) and `esp32s3` (S3 DevKit).

---

## Hardware Wiring

### 1. ESP32-WROOM-32D (Receiver & Audio Out)

> [!WARNING]
> Connect CC1101 VCC **only to 3.3V**. It will be permanently damaged by 5V.

| CC1101 Pin | ESP32-WROOM-32D Pin | Notes |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Power (3.3V Only) |
| **GND** | **GND** | Common Ground |
| **CSN / SS** | **GPIO 5** | SPI Chip Select |
| **SCK** | **GPIO 18** | SPI Clock |
| **MOSI** | **GPIO 23** | SPI Data In |
| **MISO / GDO1** | **GPIO 19** | SPI Data Out |
| **GDO0** | **GPIO 4** | Raw Slicer Edge Capture |
| **(DAC Output)** | **GPIO 25** | Audio Out $\rightarrow$ $10\text{ k}\Omega$ resistor $\rightarrow$ Phone Mic / Earphone |
| **FS1000A DATA**  | **GPIO 26** | Optional 433.92 MHz ASK transmitter |

### 2. ESP32-S3 DevKit (Transmitter / DSP Engine)

| CC1101 / Module Pin | ESP32-S3 Pin | Notes |
| :--- | :--- | :--- |
| **VCC** | **3.3V** | Power (3.3V Only) |
| **GND** | **GND** | Ground |
| **CSN** | **GPIO 10** | SPI Chip Select |
| **SCK** | **GPIO 12** | SPI Clock |
| **MOSI** | **GPIO 11** | SPI Data In |
| **MISO** | **GPIO 13** | SPI Data Out |
| **GDO0** | **GPIO 14** | Asynchronous Modulation Output |
| **Audio Output** | **GPIO 1** | High-speed LEDC PWM audio (RC filter recommended) |
| **FS1000A DATA** | **GPIO 16** | Optional 433.92 MHz ASK transmitter |

#### ESP32-S3 Audio Output Circuit (RC Low-Pass Filter)
Because the ESP32-S3 does not have an internal DAC, it generates high-frequency PWM tones on **GPIO 1**. Use this simple filter to smooth the digital pulses into analog audio:

```text
[ ESP32-S3 GPIO 1 ] ───[ 1kΩ to 10kΩ Resistor ]───┬───> Audio Tip / Mic (Phone / Earphone)
                                                  │
                                            [ 100nF Cap ]
                                                  │
[ ESP32-S3 GND ]    ──────────────────────────────┴───> Audio Sleeve / Ground
```

---

## 437.55 MHz Antenna Construction

A standard 433 MHz helical spring antenna has high SWR at 437.55 MHz. For optimal satellite reception, make a simple **$\frac{1}{4}\lambda$ Ground-Plane or Whip Antenna**:

$$\lambda = \frac{c}{f} = \frac{300}{437.55} = 0.6857\text{ m} = 68.6\text{ cm}$$
$$\text{Whip Length } \left(\frac{\lambda}{4}\right) = \frac{68.6}{4} = \mathbf{17.15\text{ cm}}$$

1. Cut a stiff straight copper wire to **$17.2\text{ cm}$**.
2. Solder directly to the CC1101 antenna pad or center pin of the SMA connector.

---

## Software Installation & Flashing

### Using PlatformIO CLI

```bash
# Clone the repository
git clone https://github.com/Vivek-live-now/ESP32-CC1101-SSTV-Lab.git
cd ESP32-CC1101-SSTV-Lab

# Flash to ESP32-WROOM-32D
pio run -e esp32dev -t upload

# Monitor Serial output
pio device monitor -b 115200
```

For **ESP32-S3**:
```bash
pio run -e esp32s3 -t upload
```

---

## Interactive Serial Menu

Connect to the ESP32 via Serial at **115200 baud**:

```text
========================================================
       ESP32-CC1101 SSTV Satellite & RF Lab             
========================================================
 Target Board  : ESP32-WROOM-32D
 Audio Output  : Hardware 8-bit DAC1 (GPIO 25)
 Radio Center  : 437.550 MHz
 Doppler Shift : +0.00 kHz
--------------------------------------------------------
 [1] Start ISS SSTV RX Mode (437.550 MHz -> Audio Out)
 [2] Step Doppler Frequency UP (+2.5 kHz)
 [3] Step Doppler Frequency DOWN (-2.5 kHz)
 [4] Start 10-Minute Auto-Doppler Satellite Pass
 [5] Transmit Robot 36 via CC1101 (433.92 MHz 2-FSK)
 [6] Transmit SSTV Calibration Tones (1200-2300 Hz)
 [7] Transmit Robot 36 via FS1000A (433.92 MHz ASK/OOK)
 [8] Radio Status & RSSI Diagnostic
 [0] Stop / Set Radio to IDLE
--------------------------------------------------------
 [BOOT Button] Click: Start/Stop ISS RX | Hold: Transmit SSTV
========================================================
```

---

## Standalone Portable Operation (Using the BOOT Button)

You do **not** need a computer or serial monitor in the field! The onboard **BOOT button** (`GPIO 0`) works as a physical controller:

* **Short Click (< 1.5 seconds):** 
  * If IDLE: Instantly starts **ISS SSTV RX Mode** AND triggers the **10-minute automated Doppler tracking curve**!
  * If RX is running: Stops the receiver and returns to low-power IDLE.
* **Long Hold (> 1.5 seconds):** 
  * Instantly transmits the **Robot 36 Test Pattern** over the air so you can test decoding with your phone right away.

---

## Receiving ISS SSTV During a Pass

1. Track when the ISS passes over your location using [Look4Sat](https://play.google.com/store/apps/details?id=com.rtbishop.look4sat) or [N2YO.com](https://www.n2yo.com/).
2. When the pass begins, send `1` over the Serial console to start the demodulator.
3. Send `4` to initiate the **10-minute automated Doppler curve** (or use `2` and `3` to manually step frequency as the satellite passes).
4. Connect the ESP32 DAC output (`GPIO 25`) to your phone's microphone or place your phone running **Robot36** near an earphone connected to GPIO 25.
5. Watch the decoded picture render line by line!
6. If you get a clean picture, submit it to [ARISS SSTV Gallery](https://ariss-usa.org/ARISS_SSTV/) to claim your official certificate of participation.

---

## License
MIT License. Created by [Vivek-live-now](https://github.com/Vivek-live-now).
