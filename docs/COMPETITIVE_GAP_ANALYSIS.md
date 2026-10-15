# Competitive Gap Analysis: Embedded & Network DNS Sinkholes

**Project**: Hero's Walls  
**Author**: Principal Embedded Systems & DNS Protocol Engineer  
**Date**: 2026-10-15  
**Version**: 2.0-Research  

---

## 1. Executive Landscape Overview

Network-wide DNS sinkholing prevents ads, telemetry, and tracking beacons from resolving before an HTTP/TLS connection is ever attempted. Solutions fall into three distinct tiers:

1. **Linux-Based Appliances (Pi-hole, AdGuard Home)**: Require a Raspberry Pi or always-on server ($35–$100+ hardware, 2–10W power draw, full OS maintenance). High memory (512MB–4GB), millions of domains, regex, DoH/DoT/DoQ, extensive sqlite logs.
2. **Browser Extensions (uBlock Origin)**: In-browser DOM element hiding, dynamic script injection, and CNAME uncloaking. Zero network equipment required, but operates *only* within the browser—cannot protect smart TVs, mobile apps, IoT devices, or console games.
3. **Microcontroller Appliances (s60sc/ESP32_AdBlocker, rubfi/esphole, M-Abozaid/esp32-c3-adblock)**: Tiny dedicated hardware ($3–$7), $<0.5W power draw, instant boot, silent, zero OS overhead. Heavily constrained by SRAM (300KB–500KB) and Flash storage (4MB–16MB).

---

## 2. Competitive Comparison Matrix

| Feature / Dimension | **Pi-hole v5/v6** | **AdGuard Home** | **uBlock Origin** | **s60sc/ESP32_AdBlocker** | **Hero's Walls V2 (Our Target)** |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Platform Target** | Linux (Raspberry Pi/x86) | Linux / Windows / BSD | Browser WebExtension | ESP32 / ESP32-S3 (Dual Core) | ESP32-C3 & ESP32-S3 (Dual Target) |
| **Typical Hardware Cost** | \$45 – \$90 | \$35 – \$120 | \$0 (runs in browser) | \$5 – \$9 | \$3 – \$6 (Single-chip SoC) |
| **Power Consumption** | 2.5W – 7.0W | 2.5W – 8.0W | N/A (client CPU) | 0.3W – 0.6W | 0.25W – 0.5W |
| **RAM Footprint** | 150MB – 350MB | 100MB – 400MB | 50MB – 120MB | Requires $\ge 4\text{MB}$ PSRAM | $\le 48\text{KB}$ SRAM (C3) / 8MB PSRAM (S3) |
| **Domain Storage Strategy** | FTL (in-memory trie) | In-memory radix tree | In-memory filter parser | Linear RAM buffer + 32-bit pointers | Packed FNV-1a Suffix Trie / Exact Flash BinSearch |
| **Blocklist Capacity** | 1,000,000+ domains | 1,000,000+ domains | 300,000+ rules | ~250,000 (Requires PSRAM) | ~2,500 curated in SRAM (C3), 250k+ (S3) |
| **Matching Precision** | Exact + Regex + Subdomains | Exact + Regex + Client Rules | Full CSS/DOM/URL Regex | Exact string match only | Strict Label-Boundary Suffix Matching |
| **Subdomain Handling** | Inherited wildcard match | Wildcard `||domain^` | Path/URL/Domain Regex | Requires explicit domain entry | Suffix label match (`*.doubleclick.net`) |
| **DNS-over-TCP** | Yes | Yes (TCP, DoH, DoT, DoQ) | N/A | No (UDP only) | UDP + RFC 1035 TCP fallback stub |
| **DNS Cache** | Full TTL & Negative cache | Full TTL, Optimistic cache | Local browser cache | Static round-robin 300s TTL | Dynamic TTL tracking + bounded LRU |
| **YouTube Ad Handling** | DNS telemetry sinkhole only | DNS telemetry sinkhole only | Cosmetic stream injection | None specific | Curated Telemetry & Stats Sinkhole |
| **Setup Experience** | CLI / Headless SSH setup | Web installer wizard | 1-click web store install | Web UI + Configs file | Captive Portal + 1-Click Master Toggle |
| **License** | EUPL v1.2 / GPL v3 | GPL v3 | GPL v3 | AGPL v3 | AGPL v3 (Fully Compliant) |

---

## 3. Deep Technical Audit of Existing ESP32 Approaches

### A. s60sc/ESP32_AdBlocker (Upstream Baseline)
* **Strengths**: Integrated web server using ESP-IDF `esp_http_server`, WebDAV file editor, RAM log buffer, OTA updates, NVS preference management.
* **Fatal Flaws on Entry-Level Chips (ESP32-C3)**:
  1. *Hard PSRAM Dependency*: Hardcoded `#error "App not compatible with ESP32 C3"` and allocated full domain lists via `ps_malloc()`.
  2. *Single-Core Crash*: Hardcoded `xTaskCreatePinnedToCore(..., 1)`. On the single-core ESP32-C3 RISC-V processor, attempting to pin a task to Core 1 panics the FreeRTOS kernel immediately.
  3. *TLS Download Heap Exhaustion*: Attempting to download multi-megabyte StevenBlack hosts lists over HTTPS via `WiFiClientSecure` requires $>60\text{KB}$ contiguous heap for mbedTLS record buffers, which exceeds free SRAM on chips without external PSRAM.
  4. *Lack of Subdomain Matching*: Upstream checks *only* `strcmp(storage + ptrs[ptr], searchStr)`. If `doubleclick.net` is in the blocklist, a query for `ad.doubleclick.net` or `video.doubleclick.net` **completely bypassed the filter** unless every subdomain was explicitly listed!

### B. M-Abozaid/esp32-c3-adblock & Flash-Mapped Hashes
* **Strengths**: Stores 40-bit or 32-bit FNV-1a hashes in Flash; queries do binary search directly across memory-mapped Flash (`spi_flash_mmap`).
* **Weaknesses**: Flash write cycles wear out during frequent OTA updates; cannot dynamically add/remove single domains without rewriting whole flash blocks; hash collisions have no string fallback, leading to rare false blocks.

---

## 4. Hero's Walls V2: Defensible Differentiation Strategy

To make Hero's Walls stand out credibly in the open-source community, we focus on technical rigor rather than marketing hype:

1. **Strict Label-Boundary Suffix Matching**:
   - `adservice.google.com` correctly matches the rule `google.com`'s sub-rule `adservice.google.com`.
   - `notdoubleclick.net` is **NEVER** blocked just because it contains `doubleclick.net`. A dot label boundary (`.`) is strictly verified.
2. **Dual-Tier Hardware Architecture**:
   - **ESP32-C3 Profile**: Zero external RAM needed. Operates a high-impact, curated in-SRAM blocklist covering telemetry, analytics, mobile in-app SDKs (Unity, AppLovin, Vungle), and top trackers, paired with microsecond label-boundary matching.
   - **ESP32-S3 Profile**: Auto-detects 8MB PSRAM and expands capacity to 250,000+ domains with delta-compressed storage.
3. **DNS Protocol Correctness**:
   - Accurate RFC 1035 QNAME traversal with overflow prevention.
   - Genuine A (`0.0.0.0`) and AAAA (`::`) sinkhole responses with valid TTLs.
   - Pass-through EDNS0 buffer sizing and proper RCODE forwarding (NXDOMAIN vs SERVFAIL).
4. **Honest Capabilities (No Snake Oil)**:
   - Explicitly document that DNS sinkholes cannot block YouTube video stream ads that share CDN hostnames (`*.googlevideo.com`).
   - Block YouTube ad reporting (`s.youtube.com`, `video-stats.l.google.com`, `ad.youtube.com`) while transparently advising users on client-side stream blockers (uBlock Origin / SmartTube).
5. **Zero-Friction Household Experience**:
   - Out-of-the-box auto-connect to remembered home Wi-Fi networks.
   - Built-in captive setup portal if Wi-Fi changes.
   - 1-click Protection On/Off toggle without rebooting.
