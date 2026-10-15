# Hero's Walls V2 — Hardware Profiles & Pinouts

Hero's Walls provides first-class support for both ultra-compact single-core RISC-V microcontrollers and high-capacity dual-core Xtensa microcontrollers with external PSRAM.

---

## 1. Hardware Matrix

| Parameter | ESP32-C3 SuperMini (Current Hardware) | ESP32-S3 DevKit N8R8 (Secondary Target) |
| :--- | :--- | :--- |
| **Microcontroller** | ESP32-C3FH4 (QFN32) | ESP32-S3-WROOM-1 |
| **ISA / Cores** | 32-bit RISC-V Single-Core @ 160 MHz | 32-bit Xtensa LX7 Dual-Core @ 240 MHz |
| **Embedded Flash** | 4 MB SPI Flash (DIO, 80 MHz) | 8 MB / 16 MB Octal Flash |
| **External PSRAM** | None (0 MB) | 8 MB Octal PSRAM |
| **Operating RAM** | ~400 KB Internal SRAM | 512 KB SRAM + 8 MB PSRAM |
| **Blocklist Capacity** | 1,500 – 3,000 curated domains + patterns | 250,000 – 400,000 full blocklist domains |
| **Power Consumption**| ~90 mA @ 5V (~0.45W) | ~170 mA @ 5V (~0.85W) |
| **Status Indicator** | Built-in Blue LED (GPIO 8) | RGB WS2812 NeoPixel (GPIO 48 or 38) |
| **USB Interface** | Native USB-CDC / JTAG (GPIO 18/19) | Native USB-CDC or CH340 / CP2102 |

---

## 2. ESP32-C3 SuperMini Pinout & Wiring

The ESP32-C3 SuperMini board is the size of a coin (approx. 22.5 mm × 18 mm), making it ideal for plugging directly into a router's USB port for power.

```
                  +-------------------+
                  |    USB Type-C     |
                  +---+-----------+---+
                      |           |
             GPIO 0 --| 1      16 |-- 5V (Power In)
             GPIO 1 --| 2      15 |-- GND
             GPIO 2 --| 3      14 |-- 3V3
             GPIO 3 --| 4      13 |-- GPIO 10
             GPIO 4 --| 5      12 |-- GPIO 9 (BOOT)
             GPIO 5 --| 6      11 |-- GPIO 8 (Onboard Blue LED)
             GPIO 6 --| 7      10 |-- GPIO 7
             GPIO 18--| 8       9 |-- GPIO 19
                      +---------------+
```

### Onboard Status LED Behavior:
- **Steady Lit**: Normal filtering mode (Connected to Wi-Fi, DNS server active).
- **Slow Blink (1s)**: Offline / Disconnected from Wi-Fi.
- **Fast Blink (300ms)**: Downloading / updating blocklist.
- **Medium Pulse (600ms)**: SoftAP Setup Mode (`ESP32_AdBlocker`).
- **Rapid Strobe (150ms)**: Hardware or configuration fault.

---

## 3. Power Supply Recommendations
- Any standard 5V USB Type-A port (such as the USB port on your home Wi-Fi router) provides 500mA–1000mA, which is more than sufficient to power Hero's Walls indefinitely.
- Power consumption is less than 0.5 Watts, costing less than $0.50 per year in electricity.
