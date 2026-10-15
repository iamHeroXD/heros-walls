# Hero's Walls V2 — Power Consumption & Electrical Audit

## 1. Measurement Methodology & Transparency Declaration

> [!IMPORTANT]
> **Measurement Status Declaration**:
> In compliance with strict engineering integrity standards, values below are explicitly categorized into **[MEASURED - INSTRUMENTED]**, **[ESTIMATED - DATASHEET]**, or **[NOT TESTED - LABORATORY CALIBRATION REQUIRED]**.
> Uninstrumented figures previously reported in prototype drafts are marked **UNVERIFIED**.

---

## 2. Power Consumption Matrix

| Operating State | Measured Current (mA) | Nominal Supply Voltage (V) | Calculated Power (W) | Verification Method | Status |
| :--- | :--- | :--- | :--- | :--- | :--- |
| **Active Wi-Fi Burst (Full TX +19.5dBm)** | ~240 mA (Peak) | 5.0 V | 1.20 W | Espressif ESP32-C3 Datasheet Section 4.3 | **ESTIMATED (Datasheet)** |
| **Continuous Filtering (114 QPS load)** | ~95 mA (Avg) | 5.0 V | 0.475 W | Calculated from LDO + SoC load model | **UNVERIFIED (Requires USB Power Meter)** |
| **Idle Wi-Fi Connected (Modem Sleep ON)** | ~25 mA (Avg) | 5.0 V | 0.125 W | Espressif 802.11 DTIM3 reference curve | **ESTIMATED (Datasheet)** |
| **Bench Laboratory Shunt Measurement** | N/A | 5.0 V | N/A | Precision digital multimeter shunt | **NOT TESTED (Instrumentation Absent)** |
| **Annualized Electricity Cost (@ 0.45W)** | N/A | 5.0 V | ~3.94 kWh/year | Standard mathematical integration ($0.12/kWh) | **CALCULATED (~$0.47 / year)** |

---

## 3. Recommended Instrument Test Procedure

For third-party maintainers and independent evaluators wishing to measure electrical consumption:

1. **Required Equipment**:
   - Inline USB power meter (e.g. FNIRSI FNB48P / ChargerLAB Power-Z KM003C) OR inline high-precision current shunt resistor ($0.1\,\Omega,\,1\%$) with a calibrated digital multimeter.
   - Regulated 5.00V DC laboratory power supply.
2. **Measurement Steps**:
   - Insert the meter in series between the 5V power supply and the ESP32-C3 SuperMini USB-C port.
   - Allow the device to boot, connect to Wi-Fi, and settle into normal standby for 60 seconds. Record the quiescent idle current ($I_{\text{idle}}$).
   - Execute the automated benchmark stress test:
     ```bash
     python benchmarks/run_benchmarks.py <device-ip>
     ```
   - Record the root-mean-square current ($I_{\text{load}}$) and peak transient current ($I_{\text{peak}}$) across the 400-query benchmark run.
   - Update `benchmarks/power_results.csv` with verified instrument serial numbers, ambient temperatures, and multimeter readings.
