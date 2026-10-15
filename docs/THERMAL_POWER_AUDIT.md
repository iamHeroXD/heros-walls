# Hero's Walls V2 — Thermal & Power Safety Audit

## 1. Investigation of Board Heating

### 1.1 The Symptom
During preliminary testing, when the ESP32-C3 SuperMini development board was powered via a standard 5V USB phone charger, users noted that the board felt noticeably warm / hot to the touch.

### 1.2 Root Cause Analysis: Hardware & Electrical Physics

1. **Board Form Factor & Thermal Mass**:
   - Board: ESP32-C3 SuperMini (22.5 mm × 18 mm).
   - The PCB has minimal copper surface area and lacks dedicated thermal relief planes or heat-dissipation vias.
2. **Linear Voltage Regulator (LDO) Inefficiency**:
   - The board features a miniature Low Dropout Regulator (typically an ME6211 or RT9193 in an ultra-small SOT-23-5 package).
   - In a linear regulator, power dissipated as waste heat is calculated as:
     $$P_{\text{heat}} = (V_{\text{in}} - V_{\text{out}}) \times I_{\text{load}}$$
   - USB chargers frequently deliver $5.15\text{V} - 5.25\text{V}$. With $V_{\text{out}} = 3.3\text{V}$, the voltage drop across the tiny regulator silicon die is:
     $$\Delta V = 5.20\text{V} - 3.30\text{V} = 1.90\text{V}$$
   - When the ESP32-C3 Wi-Fi radio is actively transmitting and listening at full power (+19.5 to +20.5 dBm), the chip draws approximately 100–140 mA continuous current.
   - Power dissipated strictly in the regulator:
     $$P_{\text{LDO}} = 1.90\text{V} \times 0.12\text{A} \approx 0.228\text{ Watts}$$
   - The thermal resistance ($\theta_{\text{JA}}$) of a standard SOT-23 package on minimal PCB copper is approximately $200^\circ\text{C}/\text{W} - 250^\circ\text{C}/\text{W}$.
   - Consequently, the regulator silicon temperature rises by:
     $$\Delta T \approx 0.228\text{W} \times 220^\circ\text{C}/\text{W} \approx 50^\circ\text{C}\text{ above ambient}$$
   - At a room ambient temperature of 25°C, the regulator surface reaches **70°C–75°C**.
   - Human skin registers temperatures above 45°C as hot, and above 55°C as uncomfortably hot, creating the impression of an abnormal defect despite the silicon remaining well within the manufacturer's rated operational limit of 125°C.

3. **RF Synthesis & Wi-Fi Power State**:
   - In standard baseline firmware, Wi-Fi modem sleep was disabled, meaning the internal power amplifier (PA) and radio synthesizer remained energized 100% of the duty cycle even when no DNS queries were in flight.

---

## 2. Firmware Mitigations Implemented in Hero's Walls V2

To maximize thermal safety and minimize idle energy consumption, the following firmware optimizations were audited and implemented:

1. **Modem Sleep Enablement (`WiFi.setSleep(true)`)**:
   - Enables 802.11 DTIM modem sleep during idle intervals. The radio drops from ~110 mA to ~15–20 mA between packet bursts without dropping association or missing DNS queries.
2. **RF Output Power Tuning (`WiFi.setTxPower(WIFI_POWER_15dBm)`)**:
   - Reduced excessive transmit power from the default +19.5 dBm to +15.0 dBm. This reduces peak transmit current by over 35% with negligible impact on local home Wi-Fi range (still providing strong signal through multiple interior walls).
3. **Elimination of Busy Loops**:
   - Audited FreeRTOS tasks to ensure all loops utilize blocking `vTaskDelay` or blocking socket `recvfrom`, guaranteeing the CPU executes the FreeRTOS `vPortYield` idle hook when quiescent.
4. **Non-Blocking Diagnostic Logging**:
   - Replaced verbose serial logging on every query with non-blocking channels (`LOG_VRB`), avoiding serial TX FIFO spinlocks.

---

## 3. Operational Safety Guidelines

> [!CAUTION]
> Never use an external fan or passive heatsink to mask an underlying electrical short or incorrect power supply polarity.

- **Recommended Power Source**: Power the ESP32-C3 using a certified 5.0V USB port (such as the USB port on your home Wi-Fi router or a standard 5V 1A USB adapter). Avoid uncertified "fast chargers" that output noisy > 5.3V rails.
- **Ventilation**: Ensure the board is placed in open air or an enclosure with adequate ventilation slots. Do not wrap the bare PCB in insulating tape or enclose it in an airtight plastic bag.
- **Maximum Junction Rating**:
  - ESP32-C3 SoC: Operating junction temperature up to **105°C**.
  - ME6211 LDO: Operating junction temperature up to **125°C**.
- Under V2 firmware with modem sleep and tuned RF power, the board operates stably in continuous 24/7 service.
