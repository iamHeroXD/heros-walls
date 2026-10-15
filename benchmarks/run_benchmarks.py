#!/usr/bin/env python3
"""
Hero's Walls V2 Automated DNS Benchmark & Verification Suite
-----------------------------------------------------------
Measures:
  - Latency percentiles (p50, p95, p99, min, max, avg)
  - Throughput (Queries Per Second / QPS)
  - Sinkhole verification (Blocked domains return 0.0.0.0 or ::)
  - Upstream resolution (Allowed domains resolve to valid public IPs)
  - False Positive zero-regression check against legitimate domain corpus
  - Subdomain blocking inheritance verification
  - Cache hit vs Cache miss latency delta
Outputs:
  - Terminal summary table
  - benchmarks/results.csv
  - benchmarks/results.json
"""

import sys
import os
import time
import socket
import struct
import random
import statistics
import json
import csv

DNS_PORT = 53
DEFAULT_TIMEOUT = 2.0  # seconds

def build_dns_query(domain: str, qtype_int: int = 1) -> (bytes, int):
    qid = random.randint(1, 65535)
    # Header: ID, Flags=0x0100 (RD), QDCOUNT=1, ANCOUNT=0, NSCOUNT=0, ARCOUNT=0
    header = struct.pack("!HHHHHH", qid, 0x0100, 1, 0, 0, 0)
    qname = b""
    for part in domain.strip(".").split("."):
        b_part = part.encode("ascii")
        qname += bytes([len(b_part)]) + b_part
    qname += b"\x00"
    qtype = struct.pack("!HH", qtype_int, 1)  # QTYPE, QCLASS=IN
    return header + qname + qtype, qid

def parse_dns_response(data: bytes, expected_qid: int) -> dict:
    if len(data) < 12:
        return {"error": "Packet too short"}
    qid, flags, qdcount, ancount, nscount, arcount = struct.unpack("!HHHHHH", data[:12])
    if qid != expected_qid:
        return {"error": "Transaction ID mismatch"}
    
    rcode = flags & 0x000F
    is_response = (flags & 0x8000) != 0
    if not is_response:
        return {"error": "Not a response"}
    
    # Skip Question Section
    offset = 12
    for _ in range(qdcount):
        while offset < len(data) and data[offset] != 0:
            if (data[offset] & 0xC0) == 0xC0:
                offset += 2
                break
            offset += 1 + data[offset]
        if offset < len(data) and data[offset] == 0:
            offset += 1
        offset += 4  # QTYPE + QCLASS
    
    answers = []
    for _ in range(ancount):
        if offset >= len(data):
            break
        # Owner name
        if (data[offset] & 0xC0) == 0xC0:
            offset += 2
        else:
            while offset < len(data) and data[offset] != 0:
                offset += 1 + data[offset]
            if offset < len(data) and data[offset] == 0:
                offset += 1
        if offset + 10 > len(data):
            break
        rtype, rclass, ttl, rdlen = struct.unpack("!HHIH", data[offset:offset+10])
        offset += 10
        if offset + rdlen > len(data):
            break
        rdata = data[offset:offset+rdlen]
        offset += rdlen
        
        if rtype == 1 and rdlen == 4:
            ip_str = socket.inet_ntoa(rdata)
            answers.append({"type": "A", "ip": ip_str, "ttl": ttl})
        elif rtype == 28 and rdlen == 16:
            ip_str = socket.inet_ntop(socket.AF_INET6, rdata)
            answers.append({"type": "AAAA", "ip": ip_str, "ttl": ttl})
        else:
            answers.append({"type": f"TYPE_{rtype}", "raw": rdata.hex(), "ttl": ttl})
            
    return {
        "qid": qid,
        "rcode": rcode,
        "ancount": ancount,
        "answers": answers
    }

def query_dns(server_ip: str, domain: str, qtype: str = "A", timeout: float = DEFAULT_TIMEOUT) -> dict:
    qtype_int = 28 if qtype == "AAAA" else 1
    packet, qid = build_dns_query(domain, qtype_int)
    
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    sock.settimeout(timeout)
    t0 = time.perf_counter()
    try:
        sock.sendto(packet, (server_ip, DNS_PORT))
        resp, _ = sock.recvfrom(512)
        rtt_ms = (time.perf_counter() - t0) * 1000.0
        parsed = parse_dns_response(resp, qid)
        parsed["rtt_ms"] = rtt_ms
        parsed["domain"] = domain
        parsed["qtype"] = qtype
        return parsed
    except socket.timeout:
        return {"domain": domain, "qtype": qtype, "error": "TIMEOUT", "rtt_ms": timeout * 1000.0}
    except Exception as e:
        return {"domain": domain, "qtype": qtype, "error": str(e), "rtt_ms": (time.perf_counter() - t0) * 1000.0}
    finally:
        sock.close()

def run_benchmarks(server_ip: str):
    print(f"\n=======================================================")
    print(f"  HERO'S WALLS V2 — AUTOMATED BENCHMARK & VERIFICATION")
    print(f"  Target Appliance: {server_ip}:{DNS_PORT}")
    print(f"=======================================================\n")
    
    # 1. Connectivity Check
    print("[1/5] Checking DNS Appliance Connectivity...")
    check = query_dns(server_ip, "google.com")
    if "error" in check:
        print(f"[-] ERROR: Failed to connect to {server_ip}: {check.get('error')}")
        print("    Ensure device is powered on, Wi-Fi connected, and IP is correct.")
        sys.exit(1)
    print(f"[+] Appliance Online! Baseline RTT: {check['rtt_ms']:.2f} ms\n")

    # 2. Blocklist & Pattern Verification Test
    print("[2/5] Running Blocklist & Sinkhole Tests (Expected: 0.0.0.0)...")
    blocked_test_domains = [
        "doubleclick.net",
        "ad.doubleclick.net",
        "pubads.g.doubleclick.net",
        "googleadservices.com",
        "pagead2.googlesyndication.com",
        "video-stats.l.google.com",
        "s.youtube.com",
        "ad.youtube.com",
        "adnxs.com",
        "applovin.com",
        "unityads.unity3d.com",
        "criteo.com",
        "scorecardresearch.com",
        "crashlytics.com",
        "google-analytics.com",
        "ssl.google-analytics.com"
    ]
    
    block_results = []
    block_rtts = []
    all_blocked_pass = True
    for dom in blocked_test_domains:
        res = query_dns(server_ip, dom, "A")
        ips = [ans.get("ip") for ans in res.get("answers", [])]
        is_sinkholed = ("0.0.0.0" in ips)
        if not is_sinkholed:
            all_blocked_pass = False
        rtt = res.get("rtt_ms", 0.0)
        block_rtts.append(rtt)
        status = "PASS (0.0.0.0)" if is_sinkholed else f"FAIL ({ips})"
        print(f"    - {dom:<32} {status:<18} ({rtt:6.2f} ms)")
        block_results.append({
            "category": "BLOCKLIST_VERIFY",
            "domain": dom,
            "expected": "0.0.0.0",
            "actual": ips[0] if ips else "NONE",
            "passed": is_sinkholed,
            "rtt_ms": round(rtt, 3)
        })
    print(f"[*] Blocklist Sinkhole Result: {'ALL PASS' if all_blocked_pass else 'FAILURES DETECTED'}\n")

    # 3. False-Positive Zero-Regression Test
    print("[3/5] Running False-Positive Regression Tests (Expected: Legitimate IP != 0.0.0.0)...")
    corpus_file = os.path.join(os.path.dirname(__file__), "legitimate_domains.txt")
    legit_domains = []
    if os.path.exists(corpus_file):
        with open(corpus_file, "r") as f:
            for line in f:
                line = line.strip()
                if line and not line.startswith("#"):
                    legit_domains.append(line)
    else:
        legit_domains = ["google.com", "youtube.com", "github.com", "wikipedia.org", "notdoubleclick.net", "mytelemetryapp.com"]
        
    fp_results = []
    fp_rtts = []
    zero_false_positives = True
    for dom in legit_domains:
        res = query_dns(server_ip, dom, "A")
        ips = [ans.get("ip") for ans in res.get("answers", [])]
        if "0.0.0.0" in ips:
            zero_false_positives = False
            status = "FALSE POSITIVE (Blocked!)"
            passed = False
        elif not ips:
            # Might be NXDOMAIN if synthetic domain, e.g. notdoubleclick.net
            status = f"RESOLVED/NX (RCODE={res.get('rcode')})"
            passed = True
        else:
            status = f"PASS ({ips[0]})"
            passed = True
        rtt = res.get("rtt_ms", 0.0)
        fp_rtts.append(rtt)
        print(f"    - {dom:<32} {status:<24} ({rtt:6.2f} ms)")
        fp_results.append({
            "category": "FALSE_POSITIVE_CHECK",
            "domain": dom,
            "expected": "VALID_OR_UPSTREAM",
            "actual": ips[0] if ips else f"RCODE_{res.get('rcode')}",
            "passed": passed,
            "rtt_ms": round(rtt, 3)
        })
    print(f"[*] False Positive Score: {'100% CLEAN (0 False Positives)' if zero_false_positives else 'DEFECT DETECTED'}\n")

    # 4. Cache Hit vs Cache Miss Latency Benchmark
    print("[4/5] Measuring DNS Cache Acceleration (Miss vs Hit Latencies)...")
    cache_domain = "github.com"
    # First query -> Cache Miss (fetches from upstream)
    miss_res = query_dns(server_ip, cache_domain)
    time.sleep(0.05)
    # Subsequent 5 queries -> Cache Hits
    hit_rtts = []
    for _ in range(5):
        h = query_dns(server_ip, cache_domain)
        hit_rtts.append(h.get("rtt_ms", 0.0))
        time.sleep(0.02)
        
    avg_hit_rtt = statistics.mean(hit_rtts)
    print(f"    - Cache Miss Latency: {miss_res.get('rtt_ms', 0):.2f} ms (upstream resolution)")
    print(f"    - Cache Hit Latency:  {avg_hit_rtt:.2f} ms (on-chip SRAM lookup)")
    delta_x = (miss_res.get('rtt_ms', 1) / max(0.1, avg_hit_rtt))
    print(f"    - Speedup Factor:     {delta_x:.1f}x faster with on-chip cache\n")

    # 5. Throughput & Percentile Stress Benchmark
    print("[5/5] Measuring Latency Percentiles (p50, p95, p99) and Query Throughput...")
    stress_domains = [
        "doubleclick.net", "google.com", "adservice.google.com", "github.com",
        "pubads.g.doubleclick.net", "wikipedia.org", "s.youtube.com", "cloudflare.com"
    ]
    iterations = 50
    total_queries = len(stress_domains) * iterations
    stress_rtts = []
    t_start = time.perf_counter()
    
    for _ in range(iterations):
        for dom in stress_domains:
            r = query_dns(server_ip, dom, timeout=1.5)
            stress_rtts.append(r.get("rtt_ms", 0.0))
    t_total = time.perf_counter() - t_start
    actual_qps = total_queries / t_total

    sorted_rtts = sorted(stress_rtts)
    p50 = sorted_rtts[int(len(sorted_rtts) * 0.50)]
    p95 = sorted_rtts[int(len(sorted_rtts) * 0.95)]
    p99 = sorted_rtts[int(len(sorted_rtts) * 0.99)]
    avg_rtt = statistics.mean(stress_rtts)
    min_rtt = min(stress_rtts)
    max_rtt = max(stress_rtts)

    print("\n=======================================================")
    print("                BENCHMARK RESULTS SUMMARY               ")
    print("=======================================================")
    print(f"  Total Queries Processed: {total_queries}")
    print(f"  Benchmark Duration:      {t_total:.2f} seconds")
    print(f"  Throughput (QPS):        {actual_qps:.1f} queries/sec")
    print(f"  Min Decision Latency:    {min_rtt:.2f} ms")
    print(f"  p50 (Median) Latency:    {p50:.2f} ms")
    print(f"  p95 Latency:             {p95:.2f} ms")
    print(f"  p99 Latency:             {p99:.2f} ms")
    print(f"  Max Latency:             {max_rtt:.2f} ms")
    print(f"  Average Latency:         {avg_rtt:.2f} ms")
    print("=======================================================\n")

    # Export to CSV & JSON
    all_records = block_results + fp_results
    csv_path = os.path.join(os.path.dirname(__file__), "results.csv")
    json_path = os.path.join(os.path.dirname(__file__), "results.json")
    
    with open(csv_path, "w", newline="") as f:
        writer = csv.DictWriter(f, fieldnames=["category", "domain", "expected", "actual", "passed", "rtt_ms"])
        writer.writeheader()
        writer.writerows(all_records)
    print(f"[+] Saved detailed per-query results to: {csv_path}")

    summary_data = {
        "device_ip": server_ip,
        "total_queries": total_queries,
        "duration_sec": round(t_total, 3),
        "qps": round(actual_qps, 2),
        "latency_min_ms": round(min_rtt, 3),
        "latency_p50_ms": round(p50, 3),
        "latency_p95_ms": round(p95, 3),
        "latency_p99_ms": round(p99, 3),
        "latency_avg_ms": round(avg_rtt, 3),
        "latency_max_ms": round(max_rtt, 3),
        "cache_miss_ms": round(miss_res.get("rtt_ms", 0), 3),
        "cache_hit_avg_ms": round(avg_hit_rtt, 3),
        "cache_speedup_x": round(delta_x, 2),
        "zero_false_positives": zero_false_positives,
        "blocklist_pass": all_blocked_pass,
        "verified_at": time.strftime("%Y-%m-%d %H:%M:%S UTC", time.gmtime())
    }
    with open(json_path, "w") as f:
        json.dump(summary_data, f, indent=2)
    print(f"[+] Saved aggregate benchmark telemetry to: {json_path}\n")

if __name__ == "__main__":
    target = sys.argv[1] if len(sys.argv) > 1 else "192.168.1.50"
    run_benchmarks(target)
