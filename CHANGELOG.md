# Changelog

All notable changes to **Hero's Walls** will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

---

## [2.0.0] - 2026-10-15

### Added
- **Strict Label-Boundary Suffix Matching (`matchDomainSuffix`)**: Replaced flawed `strstr()` substring matching, eliminating false-positive blocks on benign domains (e.g. `notdoubleclick.net`, `mytelemetryapp.com`).
- **Recursive Label-Walking Subdomain Search**: Binary search traverses domain labels left-to-right (`sub.ad.example.com` $\to$ `ad.example.com` $\to$ `example.com`), automatically inheriting domain blocks for subdomains.
- **Priority-Enforced Allowlist Architecture**: Built-in system allowlist and dynamic user allowlist take absolute precedence over ad patterns and blocklist storage.
- **Automated Python Benchmark Suite (`benchmarks/run_benchmarks.py`)**: Zero-dependency RFC 1035 UDP benchmark measuring p50/p95/p99 latency, sustained QPS, and generating `results.csv` and `results.json`.
- **False-Positive Test Corpus (`benchmarks/legitimate_domains.txt`)**: 75+ legitimate domains across cloud, search, media, finance, and edge cases.
- **Comprehensive Documentation Suite**: `ARCHITECTURE.md`, `COMPETITIVE_ANALYSIS.md`, `PERFORMANCE.md`, `SECURITY.md`, `THERMAL_POWER_AUDIT.md`, `POWER_BENCHMARKS.md`, `NETWORK_SETUP.md`, `RECOVERY.md`, `HARDWARE.md`, `DEVELOPMENT.md`, `PUBLIC_LAUNCH.md`, `DEMO_SCRIPT.md`.
- **CI Workflow (`.github/workflows/ci.yml`)**: Automated multi-target compilation for ESP32-C3 and ESP32-S3 via Arduino CLI.

### Changed
- **Logging Performance Fix**: Replaced blocking `LOG_INF` on per-packet DNS query processing in `externalDNS.cpp` with `LOG_VRB`, eliminating FreeRTOS logger queue saturation stalls under burst query loads.
- **Wi-Fi Power Management**: Configured modem sleep and tuned RF power to +15 dBm to lower operating temperatures and energy consumption on compact LDO boards.

### Fixed
- Fixed unblocking logic: deleting a domain via Web Dashboard now registers the domain into the dynamic RAM allowlist and persists unblock state across reboots.
