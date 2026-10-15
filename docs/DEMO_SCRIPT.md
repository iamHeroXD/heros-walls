# Hero's Walls V2 — Live Demonstration Script

This 2-minute demonstration script is designed for recording video demos, screencasts, or live developer presentations.

---

## Equipment Setup
- **Hardware**: ESP32-C3 SuperMini connected to USB power.
- **Network**: Wi-Fi connected to home network.
- **Client Laptop**: Connected to same Wi-Fi, DNS pointing to Hero's Walls IP (`192.168.1.50` or assigned IP).
- **Screen**: Split-screen with Browser (Hero's Walls Dashboard) on left and Terminal on right.

---

## Demo Walkthrough (120 Seconds)

### Part 1: Physical Hardware & Dashboard Overview (0:00 - 0:30)
1. *Presenter*: "This is Hero's Walls V2. It's running on this tiny $3 ESP32-C3 SuperMini board, plugged into USB. Notice the solid blue LED indicating it's actively serving DNS on our home network."
2. Show Dashboard at `http://192.168.1.50/` (or your device's IP):
   - Point out real-time queries allowed, ads blocked, memory usage, and uptime.

### Part 2: Live DNS Sinkholing in Action (0:30 - 0:60)
1. *Presenter*: "Let's test an ad domain from the terminal."
2. In terminal, run:
   ```bash
   nslookup doubleclick.net
   ```
   Show output returning immediately:
   `Address: 0.0.0.0`
3. Point to Dashboard:
   - Notice the "Ads Blocked" counter increment by 1 in real time!
4. In terminal, test a subdomain:
   ```bash
   nslookup pubads.g.doubleclick.net
   ```
   Show output returning `0.0.0.0` via label-walking subdomain search.

### Part 3: Zero False Positives & Strict Label Boundaries (0:60 - 1:15)
1. *Presenter*: "Now let's test a benign domain that naive blockers falsely block, like `notdoubleclick.net`."
2. In terminal, run:
   ```bash
   nslookup notdoubleclick.net
   ```
   Show that it is NOT sinkholed to `0.0.0.0`—it forwards to upstream DNS cleanly.
3. Test `mytelemetryapp.com` or `google.com`—all legitimate traffic resolves without interruption.

### Part 4: Dynamic Allowlist & Web Control (1:15 - 1:45)
1. *Presenter*: "What if an essential domain is on a blocklist and you need to unblock it immediately without rebooting?"
2. In Dashboard:
   - Enter `doubleclick.net` into the domain box.
   - Click "Delete Domain" (Unblock / Allowlist).
3. Back in terminal:
   ```bash
   nslookup doubleclick.net
   ```
   Show that it now resolves normally!
4. Click "Add to Blocklist" $\to$ Immediately sinkholed again.

### Part 5: Wrap-up & Benchmark Verification (1:45 - 2:00)
1. Run `python benchmarks/run_benchmarks.py <device-ip>`.
2. Show the automated benchmark table: sub-millisecond p50 latency, 0% false positives, and clean RFC 1035 compliance.
3. Conclude: "Sub-millisecond latency, zero false positives, under 0.5 Watts of power. That is Hero's Walls V2."
