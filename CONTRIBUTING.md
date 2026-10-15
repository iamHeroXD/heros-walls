# Contributing to Hero's Walls

We welcome contributions from embedded systems engineers, network developers, security researchers, and privacy enthusiasts!

---

## 🛠️ Development Workflow

1. **Fork & Branch**:
   - Fork the repository and create a feature branch (`feature/your-feature-name` or `fix/issue-description`).
2. **Coding Standards**:
   - Write clean, modern C++ compatible with the Espressif `esp32` Arduino core (v3.x).
   - Maintain documentation integrity and preserve upstream license notices.
   - Do NOT introduce dynamic heap allocations (`new` / `malloc`) in the fast-path DNS packet loop (`dnsTask`). All packet buffers must be static or stack-allocated.
   - Any new domain rule must use strict label-boundary matching (`matchDomainSuffix`). Do NOT use raw `strstr()` for domain checks.
3. **Automated Verification**:
   - Verify that your changes compile cleanly on both targets:
     ```bash
     # Compile C3 Target
     arduino-cli compile --fqbn esp32:esp32:esp32c3:PartitionScheme=min_spiffs,FlashMode=dio,FlashFreq=80,CDCOnBoot=default .

     # Compile S3 Target
     arduino-cli compile --fqbn esp32:esp32:esp32s3:PartitionScheme=huge_app,FlashMode=qio,FlashFreq=80,PSRAM=opi,CDCOnBoot=default .
     ```
   - If you have physical hardware, execute the test suite:
     ```bash
     python benchmarks/run_benchmarks.py <device-ip>
     ```
   - Assert **0% false positive regression** across `benchmarks/legitimate_domains.txt`.
4. **Submitting a Pull Request**:
   - Open a PR against the `main` branch with a clear description of the problem solved, code changes, and test results.
