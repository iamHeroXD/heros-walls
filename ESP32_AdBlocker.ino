/*
   ESP32_AdBlocker acts as a DNS Sinkhole by returning 0.0.0.0 for any domain names in its blocked list, 
   else forwards to an external DNS server to resolve IP addresses. This prevents content being retrieved 
   from or sent to blocked domains. Checks generally take <100us.

   To use ESP32_AdBlocker, enter its IP address in place of the DNS server IPs in your router/devices.
   Currently it does not have an IPv6 address and some devices use IPv6 by default, so disable IPv6 DNS on 
   your device / router to force it to use IPv4 DNS.

   Blocklist files can be downloaded from hosting sites and should either be in HOSTS format 
   or Adblock format (only domain name entries processed)

   Based on idea from https://github.com/rubfi/esphole

   s60sc 2020, 2023
*/

#include "appGlobals.h"

void setup() { 
  bool res = utilsStartup();
  startNetwork();
  resetCrashLoop();
  startupFailure[0] = 0;
  appSetup();
  checkMemory();
}

void loop() {
  // Serial interactive Wi-Fi configuration
  if (Serial.available()) {
    String cmd = Serial.readStringUntil('\n');
    cmd.trim();
    if (cmd.startsWith("WIFI:")) {
      int sep = cmd.indexOf(':', 5);
      if (sep != -1) {
        String ssid = cmd.substring(5, sep);
        String pass = cmd.substring(sep + 1);
        LOG_INF("Setting Wi-Fi credentials via Serial: SSID '%s'", ssid.c_str());
        Preferences p;
        if (p.begin(APP_NAME, false)) {
          p.putString("ST_SSID", ssid.c_str());
          p.putString("ST_Pass", pass.c_str());
          p.end();
          LOG_INF("Saved to NVS. Restarting ESP32...");
          delay(1000);
          ESP.restart();
        }
      } else {
        LOG_WRN("Usage: WIFI:<SSID>:<PASS>");
      }
    } else if (cmd == "STATUS") {
      LOG_INF("Status: IP=%s, STA status=%d, FreeHeap=%u",
              WiFi.STA.localIP().toString().c_str(), WiFi.STA.status(), ESP.getFreeHeap());
    } else if (cmd == "RESTART") {
      ESP.restart();
    }
  }
  delay(100);
}
