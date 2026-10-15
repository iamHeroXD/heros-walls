# Hero's Walls V2 — Security Architecture & Threat Model

## 1. Threat Model & Scope

Hero's Walls is designed as a local LAN network appliance. Its primary security objective is to provide high-integrity DNS filtering without exposing the local network, host credentials, or device resources to exploitation.

```
+-------------------------------------------------------------+
|                      TRUST BOUNDARIES                       |
+-------------------------------------------------------------+
|  [External WAN]                                             |
|    - Upstream DNS servers (Cloudflare, Google, ISP)         |
|    - GitHub / blocklist distribution servers                |
+------------------------------+------------------------------+
                               | TLS / DNS (Untrusted)
+------------------------------v------------------------------+
|  [Hero's Walls Appliance]                                   |
|    - Port 53 UDP DNS listener (Input validation)            |
|    - Port 80 HTTP Dashboard (Local subnet only)             |
|    - Memory arenas & Flash NVS storage                      |
+------------------------------+------------------------------+
                               | DHCP / DNS (Trusted Local LAN)
+------------------------------v------------------------------+
|  [Internal LAN Clients]                                     |
|    - Workstations, Phones, IoT, Smart TVs                   |
+-------------------------------------------------------------+
```

---

## 2. Security Controls & Mitigations

### 2.1 Wire Packet Validation & Buffer Overflow Defenses
- **Question Section Name Parsing**: `parseDNSname` verifies bounds on every label pointer. Compression pointers (`0xC0`) in the question section are explicitly rejected per RFC 1035 section 4.1.2.
- **Label Length Caps**: Domain names are constrained to `MAX_HOSTNAME` (256 bytes) and individual labels to 63 bytes.
- **Static Buffer Sizing**: Stack buffers are bounded. All packet transmissions check output offsets against `txSize` before sending.

### 2.2 Wi-Fi Credential & NVS Security
- **Credential Storage**: Wi-Fi credentials (`ST_SSID`, `ST_Pass`) are stored in internal LittleFS flash partition (`/data/configs.txt`).
- **Log Sanitation**: Firmware logs and serial outputs never dump plain-text Wi-Fi passwords.
- **AP Isolation**: When entering failsafe SoftAP mode, the device assigns private non-routable IPs (`192.168.4.1`) and disables external WAN bridging.

### 2.3 DNS Rebinding Protection
- Upstream answers matching loopback or private ranges (RFC 1918) can be inspected or blocked if desired.
- Query forwarders distinguish legitimate domain records from malicious spoofing attempts by validating matching 16-bit transaction IDs on ephemeral upstream sockets.

### 2.4 Denial-of-Service & Resource Exhaustion Resilience
- **Ephemeral Upstream Sockets**: Upstream query timeouts are strictly capped at 800 ms per server, preventing socket exhaustion during WAN outages.
- **Memory Ceiling**: Blocklist memory allocation uses hard capacity caps (`storageSize`, `maxDomains`). Incoming downloads that exceed available memory are cleanly truncated without heap corruption or crash loops.
- **Watchdog Protection**: FreeRTOS task watchdogs monitor critical threads, triggering a controlled reboot if any task becomes unresponsive.

---

## 3. Vulnerability Reporting
If you identify a security defect in Hero's Walls firmware, please open a private GitHub Security Advisory or report it directly to the maintainers before public disclosure.
