# Hero's Walls V2 — Failsafe Modes & Recovery Guide

Hero's Walls is designed with multi-stage recovery protections so you can never be permanently locked out of your device or your home network.

---

## 1. Automatic Failsafe Modes

### Stage 1: Automatic SoftAP Fallback
If Hero's Walls cannot connect to the configured Wi-Fi network (e.g. router rebooted, SSID changed, or weak signal after 30 seconds):
1. The onboard LED shifts to the **AP Mode** pulse (amber / orange).
2. The device spawns a standalone wireless Access Point:
   - **SSID**: `ESP32_AdBlocker`
   - **Password**: None (Open network)
   - **Default Gateway**: `192.168.4.1`
3. Connect any smartphone or laptop to `ESP32_AdBlocker`.
4. Open your browser and navigate to `http://192.168.4.1`.
5. Enter your new router SSID and password in **Network Settings**, click **Save**, and restart.

---

## 2. Flash Snapshot Instant Rollback
When updating blocklists over the web:
- If a downloaded blocklist is corrupted, times out, or exceeds memory capacity, Hero's Walls automatically rolls back to the previous known-good flash snapshot stored in LittleFS partition (`/data/snapshot.bin`).
- The device continues filtering using the verified previous generation without interrupting DNS service.

---

## 3. Serial Console & Manual USB Reflash

If the device enters a bootloop or configuration becomes unrecoverable:
1. Connect the ESP32 to your PC via USB cable.
2. Open Device Manager to identify the COM port (e.g. `COM9`).
3. Open a serial monitor at **115200 baud** (e.g. PuTTY, Arduino IDE Serial Monitor, or Python pyserial) to view boot diagnostic messages.
4. To perform a clean reflash without erasing filesystem configurations:
   ```bash
   arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,FlashMode=dio,FlashFreq=80,CDCOnBoot=default .
   arduino-cli upload -p COM9 --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,FlashMode=dio,FlashFreq=80,CDCOnBoot=default .
   ```
5. If the device does not respond, hold the **BOOT** button, tap **RESET / EN**, and release **BOOT** to force ROM download mode before uploading.
