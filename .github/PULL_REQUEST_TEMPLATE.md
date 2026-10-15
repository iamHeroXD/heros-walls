## Description of Changes
Please include a summary of the changes and the related issue.

## Hardware Targets Verified
- [ ] ESP32-C3 (Single-Core RISC-V, SRAM Arena)
- [ ] ESP32-S3 (Dual-Core Xtensa, 8MB PSRAM)

## Verification Checklist
- [ ] Code compiles without warnings on target boards
- [ ] Zero false positive regression tested against `benchmarks/legitimate_domains.txt`
- [ ] No heap allocations added to the fast-path DNS packet loop
- [ ] No personal credentials, SSIDs, or private IPs included in code or commit
- [ ] Documentation updated where relevant
