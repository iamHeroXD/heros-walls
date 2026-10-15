# Hero's Walls V2 — System Architecture & Internals

## 1. Overview & Architectural Philosophy

**Hero's Walls** is an embedded DNS filtering appliance engineered specifically for the Espressif ESP32 family (ESP32-C3 single-core RISC-V and ESP32-S3 dual-core Xtensa LX7 with PSRAM). Unlike software packages running atop Linux distributions (such as Pi-hole or AdGuard Home), Hero's Walls operates on bare metal with FreeRTOS and lwIP, eliminating operating system overhead, context-switching latency, and flash file-system churn.

```
                              +---------------------------------------+
                              |         LAN DNS Clients               |
                              |   (Phones, PCs, IoT, Smart TVs)       |
                              +-------------------+-------------------+
                                                  |
                                                  | UDP Port 53 Queries
                                                  v
                              +---------------------------------------+
                              |           Hero's Walls V2             |
                              |              (ESP32)                  |
                              +-------------------+-------------------+
                                                  |
                     +----------------------------+----------------------------+
                     |                                                         |
                     v                                                         v
         +-----------------------+                                 +-----------------------+
         |   Allowlist Check     |                                 |    Ad / Tracker Engine|
         | (Precedence Priority) |                                 |  (Label-Boundary Rule)|
         +-----------+-----------+                                 +-----------+-----------+
                     |                                                         |
          [Match]    |    [No Match]                                [Match]    |    [No Match]
       +-------------+-------------+                             +-------------+-------------+
       |                           |                             |                           |
       v                           v                             v                           v
+--------------+          +-----------------+             +--------------+          +-----------------+
| Fast Forward |          |  Pattern Filter |             | Sinkhole     |          | Label-Walking   |
| to Resolver  |          | (Exact Suffix)  |             | 0.0.0.0 (A)  |          | Binary Search   |
+--------------+          +--------+--------+             | :: (AAAA)    |          +--------+--------+
                                   |                      +--------------+                   |
                        [Match]    |    [No Match]                                [Match]    |    [No Match]
                     +-------------+-------------+                             +-------------+-------------+
                     |                           |                             |                           |
                     v                           v                             v                           v
              +--------------+            +--------------+              +--------------+            +--------------+
              | Sinkhole     |            | Storage BL   |              | Sinkhole     |            | Upstream     |
              | 0.0.0.0 / :: |            | Search       |              | 0.0.0.0 / :: |            | Resolver     |
              +--------------+            +--------------+              +--------------+            +--------------+
```

---

## 2. Core Subsystems

### 2.1 The Single-Task Socket Worker (`externalDNS.cpp`)
Hero's Walls operates a dedicated UDP listener on port 53 (`dnsTask`):
- **Port Reuse**: Incoming queries from client devices arrive on port 53. The worker replies directly over the identical socket descriptor, guaranteeing the response packet originates from source port 53 as required by RFC 1035.
- **Concurrency & Non-Interference**: Upstream resolver probes do not reuse the inbound socket; instead, upstream resolutions instantiate transient ephemeral sockets (`queryUpstream`). This guarantees that incoming LAN queries are never cross-contaminated with delayed upstream WAN responses.
- **Queue Non-Blocking Optimization**: In high-throughput conditions, diagnostic logging in `dnsTask` is routed through non-blocking verbose channels (`LOG_VRB`), eliminating logger queue saturation delays.

### 2.2 Suffix Label-Boundary Matching Engine
Upstream prototypes frequently rely on crude substring matching (`strstr(normName, "rule")`), which suffers from severe false positives:
- Matching `"doubleclick.net"` would inadvertently block `notdoubleclick.net` or `doubleclick.net.example.org`.
- Matching `"telemetry"` would block legitimate services like `mytelemetryapp.com` or `cloudtelemetry.org`.

Hero's Walls V2 introduces `matchDomainSuffix`:
```cpp
static inline bool matchDomainSuffix(const char* domain, const char* rule) {
  size_t dlen = strlen(domain);
  size_t rlen = strlen(rule);
  if (dlen == rlen) return (strcmp(domain, rule) == 0);
  if (dlen > rlen) {
    if (domain[dlen - rlen - 1] == '.' && strcmp(domain + dlen - rlen, rule) == 0) {
      return true;
    }
  }
  return false;
}
```
This guarantees matching occurs strictly at domain label boundaries (`.`), correctly blocking `ad.doubleclick.net` while strictly permitting `notdoubleclick.net`.

### 2.3 Label-Walking Subdomain Binary Search
Blocklists downloaded in hosts format frequently contain parent domains without listing every possible sub-domain variant. Hero's Walls V2 implements recursive label walking:
1. Binary search the exact FQDN: e.g. `sub.ad.doubleclick.net`.
2. If no hit, advance to the next label: `ad.doubleclick.net`.
3. If no hit, advance to `doubleclick.net`.
4. Boundary condition: stop when only the TLD remains (`.net` or `.com`) to prevent accidental TLD sinkholing.

Because binary search over 3,000 domains completes in $\approx 11$ comparisons ($\sim 2\ \mu\text{s}$), a 3-label walk completes in less than $10\ \mu\text{s}$, remaining undetectable in network RTT.

### 2.4 Precedence-Enforced Allowlist Engine
Allowlist entries (both built-in system infrastructure like captive portals, NTP, and critical services, plus user-defined dynamic unblocks) take **absolute precedence** over ad patterns and blocklist storage:
- Stored in a RAM table (`userAllowlist`) and flash `/data/custom.txt`.
- Checked before ad pattern rules and before binary search.
- Allows immediate, reliable whitelisting of services with zero code recompilation.

---

## 3. Dual-Tier Memory Architecture

| Attribute | Tier 1: ESP32-C3 SuperMini | Tier 2: ESP32-S3 DevKit |
| :--- | :--- | :--- |
| **CPU Architecture** | Single-core 32-bit RISC-V @ 160 MHz | Dual-core Xtensa 32-bit LX7 @ 240 MHz |
| **RAM Configuration** | ~400 KB Internal SRAM (0 MB PSRAM) | 512 KB Internal SRAM + 8 MB Octal PSRAM |
| **Blocklist Storage Arena** | Internal Heap Arena (capped at ~36 KB) | PSRAM Arena (up to 4–6 MB) |
| **Domain Capacity** | 1,500 – 3,000 curated high-signal domains | 250,000 – 400,000 full enterprise domains |
| **Pattern Acceleration** | Active (blocks millions of ad hosts via patterns) | Active + full bulk domain tables |
| **Cache Slots** | 100 entries positive round-robin cache | 500+ entries positive cache |
| **Power Consumption** | ~0.45W (ultra-compact, low thermals) | ~0.85W |

---

## 4. RFC 1035 Compliance & Wire Protocol

1. **Transaction ID & Question Section**: Every response strictly mirrors the client's 16-bit transaction ID, query name, and question records.
2. **Sinkhole RDATA Formats**:
   - `TYPE A` queries for blocked domains return `0.0.0.0` with `RDLENGTH = 4` and standard `TTL = 60s`.
   - `TYPE AAAA` queries for blocked domains return `::` (16 bytes of zero) with `RDLENGTH = 16` and `TTL = 60s`.
   - Other query types (MX, TXT, SRV) return `NODATA` (ANCOUNT = 0, RCODE = 0).
3. **Upstream RCODE Propagation**:
   - Upstream `NXDOMAIN` (RCODE 3) returns `qdcount=1`, `ancount=0`, `RCODE 3`.
   - Upstream `SERVFAIL` (RCODE 2) returns `qdcount=1`, `ancount=0`, `RCODE 2`.
   - Upstream negative answers are **never cached**, ensuring immediate recovery when WAN connectivity resumes.
