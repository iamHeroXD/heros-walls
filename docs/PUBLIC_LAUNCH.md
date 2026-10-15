# Hero's Walls V2 — Public Open-Source Launch Assets

This document contains copy, social media releases, and launch strategies for introducing Hero's Walls to the global maker, cybersecurity, and open-source networking communities.

---

## 1. Project Positioning & Value Proposition
- **One-Liner**: *Hero's Walls is a sub-millisecond, privacy-first DNS ad and tracker blocker running entirely on low-cost ESP32 microcontrollers.*
- **Core Hook**: *Why run a 15-Watt Raspberry Pi or an old PC 24/7 just to block ads when a $3 ESP32-C3 powered by your router's USB port can filter your entire home network at sub-millisecond decision speeds with zero false positives?*

---

## 2. Launch Announcement Channels & Posts

### A. Hacker News Show HN
```
Show HN: Hero's Walls – Sub-millisecond DNS ad blocker on a $3 ESP32

Link: https://github.com/<user>/HerosWalls

I built Hero's Walls because I wanted home-wide ad and tracker blocking without running a dedicated Linux SBC, maintaining OS updates, or wearing out SD cards.

Hero's Walls is written in C++ on bare-metal FreeRTOS/lwIP for the ESP32-C3 (single-core RISC-V) and ESP32-S3 (with 8MB PSRAM). 

Key technical characteristics:
- Decision latency: ~0.8ms on-chip binary search with label-boundary suffix matching.
- Zero false positives: Suffix matching operates strictly on domain label boundaries (unlike naive substring filters that break domains like 'notdoubleclick.net').
- Subdomain inheritance: Recursive label walking automatically blocks ad subdomains when the parent domain is listed.
- Dual-tier memory model: ~3,000 curated high-signal domains on C3 internal SRAM; up to 250k+ domains on S3 PSRAM.
- Built-in real-time HTTP dashboard with instant flash snapshot rollback if a blocklist download is interrupted.
- Power: Under 0.5 Watts via router USB.

Full architecture docs, benchmark harness, and reproducible latency percentiles are in the repo. Would love your feedback!
```

### B. Reddit (r/esp32, r/homelab, r/privacy)
```
Title: [Project] Hero's Walls: I turned a $3 ESP32-C3 SuperMini into a home-wide DNS ad blocker with sub-millisecond latency

Hey everyone!

Like many here, I've run Pi-hole on Raspberry Pis for years. But SBC prices, power supplies, and SD card corruption always felt like overkill for basic DNS sinkholing.

I've spent the past few weeks hardening Hero's Walls — an open-source bare-metal firmware that turns an ESP32 into a dedicated home DNS filtering appliance.

Highlights:
1. Bare-metal lwIP/FreeRTOS socket architecture: Outbound DNS replies mirror inbound port 53 sockets strictly per RFC 1035.
2. Label-boundary matching: No false positives on benign domains.
3. YouTube tracker sinkholing: Blocks telemetry and ad beacons ('video-stats.l.google.com', 's.youtube.com') without breaking video streams.
4. On-chip Web Dashboard: Real-time query stats, allowlist controls, and custom domain injection over WebSockets.
5. Failsafe recovery: If Wi-Fi fails, it drops into SoftAP setup mode; if a blocklist is malformed, it automatically rolls back to a flash snapshot.

Full GitHub repo with hardware pinouts, benchmarks, and binary releases: [link]
```

### C. Twitter / X Launch Thread
```
1/ Tired of running a Raspberry Pi just for Pi-hole? 

Introducing Hero's Walls V2 🛡️ — a high-performance, privacy-first DNS ad and tracker blocker powered by a $3 ESP32.

Here is how it works and why bare-metal embedded DNS is awesome 🧵👇

2/ The Hardware:
Powered by an ESP32-C3 (coin-sized RISC-V) or ESP32-S3 with 8MB PSRAM.
Plugs straight into your home router's USB port for power. Consumes < 0.5W. Cost per year: ~$0.40.

3/ Zero False Positives:
Many naive blockers use crude substring matching that breaks legitimate sites. Hero's Walls uses strict RFC label-boundary suffix checks and recursive label-walking for subdomains.

4/ Speed & Benchmarks:
On-chip binary search lookup takes microseconds. With on-chip LRU positive caching, repeat queries resolve instantly.

5/ Features:
- Web dashboard with real-time stats
- Instant flash snapshot rollback
- Failsafe SoftAP recovery
- 100% open source (AGPLv3)

Check it out on GitHub: [link]
```
