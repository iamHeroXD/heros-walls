# Hero's Walls V2 🛡️
### Ultra-Lightweight, Sub-Millisecond Privacy DNS Sinkhole for ESP32

[![Hardware](https://img.shields.io/badge/Hardware-ESP32--C3%20%7C%20ESP32--S3-blue.svg)](docs/HARDWARE.md)
[![License: AGPL v3](https://img.shields.io/badge/License-AGPL_v3-green.svg)](LICENSE)
[![Website](https://img.shields.io/badge/Website-Live_on_Vercel-black?logo=vercel&logoColor=white)](https://heros-walls.vercel.app)
[![Latency](https://img.shields.io/badge/Latency-Sub--millisecond-brightgreen.svg)](docs/PERFORMANCE.md)
[![False Positives](https://img.shields.io/badge/False_Positives-0%25-success.svg)](benchmarks/results.json)

**Hero's Walls** is a high-performance, embedded DNS ad and tracker blocker designed to run directly on Espressif microcontrollers (ESP32-C3 and ESP32-S3). Operating on bare metal with FreeRTOS and lwIP, it filters DNS requests for your entire home network at sub-millisecond speeds—consuming less than 0.5 Watts of power.

🌐 **Official Website & Interactive Showcase**: [https://heros-walls.vercel.app](https://heros-walls.vercel.app)

---

## ⚡ Why Hero's Walls?

| Feature | Hero's Walls V2 (ESP32) | Traditional Pi-hole (Raspberry Pi) | Generic Router DNS Blockers |
| :--- | :--- | :--- | :--- |
| **Power Consumption** | **< 0.5 Watts** (Router USB port) | 5 – 15 Watts (Dedicated brick) | Dependent on router load |
| **Hardware Cost** | **$3 – $6** | $35 – $80+ | N/A |
| **Boot & Decision Latency**| **< 1.0 ms** (On-chip SRAM) | 2 – 5 ms (Linux OS context switch) | Variable (CPU throttled) |
| **Storage & Reliability** | **NOR Flash (Zero SD corruption)** | MicroSD card (Prone to wearout) | Router NVRAM (Fragile) |
| **Matching Engine** | **Strict Label-Boundary Suffix Matching** | Regex / Exact table | Crude substring / hosts |
| **False Positives** | **0% Zero-Regression Guarantee** | Low | High |
| **Failsafe Recovery** | **Auto SoftAP + Flash Snapshot Rollback**| Manual SSH recovery | Router factory reset |

---

## 🚀 Key Features

- **Sub-Millisecond On-Chip Decision Engine**: High-speed binary search over sorted memory arrays coupled with an LRU positive answer cache delivers sub-millisecond decision latency.
- **Strict Label-Boundary Suffix Matching**: Eliminates false positives by enforcing matching only at domain label boundaries (`.`), preventing accidental blocking of benign domains like `notdoubleclick.net` or `mytelemetryapp.com`.
- **Recursive Label-Walking Subdomain Search**: Automatically cascades blocks to all subdomains (e.g. blocking `doubleclick.net` cascades to `ad.doubleclick.net` and `pubads.g.doubleclick.net`).
- **Priority-Enforced Allowlist**: Explicit user and system allowlist rules take absolute precedence over ad patterns and blocklist storage.
- **Dual-Tier Hardware Scalability**:
  - **ESP32-C3**: Optimized internal SRAM arena holding ~1,500–3,000 curated high-signal domains + pattern filters.
  - **ESP32-S3**: Leverages 8 MB Octal PSRAM to store 250,000+ domains.
- **Real-Time On-Chip Web Dashboard**: Real-time WebSocket queries, allow/block counters, live query inspection, and dynamic domain management.
- **Automatic Failsafe & Instant Rollback**: Auto SoftAP (`ESP32_AdBlocker`) if Wi-Fi disconnects; flash snapshot rollback (`/data/snapshot.bin`) if a blocklist download is interrupted.

---

## 🧭 Documentation Suite

- 📐 [**System Architecture & Internals**](docs/ARCHITECTURE.md)
- 📊 [**Performance & Benchmarks**](docs/PERFORMANCE.md)
- 🔒 [**Security Architecture & Threat Model**](docs/SECURITY.md)
- 🌐 [**Router Integration & Network Setup**](docs/NETWORK_SETUP.md)
- 🛠️ [**Hardware Profiles & Pinouts**](docs/HARDWARE.md)
- 🚨 [**Failsafe Modes & Recovery Guide**](docs/RECOVERY.md)
- 💻 [**Developer & Build Guide**](docs/DEVELOPMENT.md)
- 📢 [**Public Launch Assets & Social Copy**](docs/PUBLIC_LAUNCH.md)
- 🎬 [**2-Minute Live Demo Script**](docs/DEMO_SCRIPT.md)

---

## 🛠️ Quick Start

### 1. Compile & Flash
```bash
# Compile for ESP32-C3
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,FlashMode=dio,FlashFreq=80,CDCOnBoot=default .

# Flash to device over USB
arduino-cli upload -p COM9 --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,FlashMode=dio,FlashFreq=80,CDCOnBoot=default .
```

### 2. Connect & Configure
1. On initial boot, connect to the Wi-Fi access point: **`ESP32_AdBlocker`**.
2. Open `http://192.168.4.1` in your browser.
3. Configure your home Wi-Fi SSID and password in **Network Settings**.
4. The device reboots, joins your home Wi-Fi, and begins filtering!

### 3. Verify Filtering
In terminal or command prompt:
```bash
# Should return 0.0.0.0 (Sinkholed!)
nslookup doubleclick.net <device-ip>

# Should resolve normally
nslookup google.com <device-ip>
```

---

## 👤 Maintainer & Creator

**Hero.X**  
- 🌐 **Developer Portfolio**: [https://heroxportfolio.netlify.app/](https://heroxportfolio.netlify.app/)  
- 💻 **GitHub**: [@iamHeroXD](https://github.com/iamHeroXD)  
- ⚡ **Project Site**: [https://heros-walls.vercel.app](https://heros-walls.vercel.app)

---

## ⚖️ Upstream & License

Hero's Walls V2 is developed atop the foundational open-source architecture of [ESP32_AdBlocker](https://github.com/s60sc/ESP32_AdBlocker) by [@s60sc](https://github.com/s60sc) and [@dateno1](https://github.com/dateno1).

Released under the **GNU Affero General Public License v3.0 (AGPL-3.0)**.

