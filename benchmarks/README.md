# Hero's Walls V2 — Benchmark Suite & Reproducibility Guide

This directory contains the automated test harness used to measure latency percentiles, throughput (QPS), cache acceleration, and false-positive immunity directly on physical ESP32 hardware.

---

## 📁 Directory Contents

- `run_benchmarks.py`: RFC 1035 UDP benchmark test harness (Pure Python standard library, zero third-party dependencies).
- `legitimate_domains.txt`: Curated corpus of top global websites, infrastructure endpoints, and edge-case test domains.
- `results.csv`: Detailed individual test query records.
- `results.json`: Summary telemetry containing aggregate latency percentiles and throughput.
- `power_results.csv`: Power and current audit table.

---

## 🚀 Running the Benchmarks

To execute the benchmark against your active Hero's Walls appliance:

```bash
python benchmarks/run_benchmarks.py <device-ip>
```

Example:
```bash
python benchmarks/run_benchmarks.py 192.168.1.50
```

### What the Test Measures:
1. **Connectivity Check**: Verifies Port 53 UDP response.
2. **Sinkhole Verification**: Queries known ad and tracker domains, asserting `0.0.0.0` or `::` responses.
3. **False-Positive Regression**: Evaluates domains from `legitimate_domains.txt`, asserting no false `0.0.0.0` sinkholing.
4. **Cache Acceleration**: Measures latency delta between an upstream resolution and repeat on-chip SRAM lookups.
5. **Throughput & Percentile Stress**: Executes 400 continuous queries, outputting p50 (median), p95, and p99 response times.
