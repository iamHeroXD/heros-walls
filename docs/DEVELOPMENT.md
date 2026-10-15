# Hero's Walls V2 — Developer & Maintainer Guide

## 1. Toolchain Setup

Hero's Walls is compiled using `arduino-cli` with the official Espressif `esp32` Arduino core (version 3.x).

### CLI Flags for ESP32-C3:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,FlashMode=dio,FlashFreq=80,CDCOnBoot=default ESP32_AdBlocker
```

### CLI Flags for ESP32-S3:
```bash
arduino-cli compile --fqbn esp32:esp32:esp32s3:FlashMode=qio,FlashSize=8M,PartitionScheme=default_8MB,PSRAM=opi ESP32_AdBlocker
```

---

## 2. Partition Schemes & Flash Layout

For 4 MB Flash boards (ESP32-C3):
- **Partition Scheme**: `min_spiffs`
- **Application Binary**: 1.9 MB (leaving ample headroom for TLS, HTTP server, and lwIP stacks)
- **LittleFS Filesystem**: 190 KB (holds `/data/configs.txt`, `/data/custom.txt`, `/data/snapshot.bin`)

---

## 3. Running Automated Benchmarks & Tests

Before submitting any Pull Request or releasing new firmware, run the Python verification suite:
```bash
python benchmarks/run_benchmarks.py <device-ip>
```

This automated test checks:
1. Sinkhole response for known ad domains (`0.0.0.0` check).
2. Zero false positive verification against `legitimate_domains.txt`.
3. Decision latency percentiles (p50, p95, p99).
4. Cache acceleration speedup.
5. Emits `benchmarks/results.csv` and `benchmarks/results.json`.
