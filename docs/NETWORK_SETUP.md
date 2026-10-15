# Hero's Walls V2 — Network Integration & Router Guide

This guide details how to deploy Hero's Walls across your home or office network so that every connected device (smartphones, PCs, smart TVs, IoT) automatically enjoys DNS ad and tracker blocking.

---

## 1. Network Deployment Models

### Option A: Whole-Network Blocking via Router DHCP (Recommended)
This is the most powerful setup. When devices join your Wi-Fi, your router automatically instructs them to use Hero's Walls for DNS resolution.

```
                           +-------------------+
                           |  Internet / ISP   |
                           +---------+---------+
                                     |
                                     v
                           +-------------------+
                           |    Home Router    |
                           |  (JioFiber / etc) |
                           | IP: 192.168.29.1  |
                           +---------+---------+
                                     |
                +--------------------+--------------------+
                |                                         |
                v                                         v
     +---------------------+                   +---------------------+
     |   Hero's Walls V2   |                   |    Client Devices   |
     |    (Static IP)      |                   | (DHCP assigns DNS   |
     |  IP: 192.168.29.50  |<------------------|  = 192.168.29.50)  |
     +---------------------+  DNS Queries      +---------------------+
```

#### Step-by-Step Router Configuration:
1. Open your router's admin portal (e.g. `http://192.168.29.1` or `http://192.168.1.1`).
2. Log in with your admin credentials.
3. Navigate to **Network Settings** $\to$ **LAN** / **DHCP Server Settings**.
4. In the **Primary DNS Server** field, enter the IP address of Hero's Walls (e.g. `192.168.1.50` or your assigned static IP).
5. Leave the **Secondary DNS Server** blank or enter Hero's Walls IP again.
   > [!IMPORTANT]
   > Do NOT enter `8.8.8.8` or `1.1.1.1` as Secondary DNS! If you provide a secondary non-filtering DNS, devices and operating systems will load-balance queries across both servers, causing ads to bypass the blocker.
6. Save and apply settings.
7. Disconnect and reconnect your client devices (or toggle Wi-Fi on/off) to acquire the new DHCP DNS lease.

---

## 2. Per-Device Manual Configuration (No Router Changes Needed)

If your ISP router has locked DHCP settings (such as some ISP fiber gateways), you can configure devices individually:

### Windows 10 / 11
1. Open **Settings** $\to$ **Network & internet** $\to$ **Wi-Fi** (or **Ethernet**).
2. Click **Hardware properties**.
3. Under **DNS server assignment**, click **Edit**.
4. Change from *Automatic (DHCP)* to *Manual*, toggle **IPv4** ON.
5. In **Preferred DNS**, enter Hero's Walls IP (e.g. `192.168.1.50`).
6. Click **Save**.

### Android 9+ (Private DNS Consideration)
1. Go to **Settings** $\to$ **Wi-Fi** $\to$ Tap your connected network's Gear/Settings icon.
2. Select **Advanced** $\to$ **IP settings** $\to$ Change from *DHCP* to *Static*.
3. In **DNS 1**, enter Hero's Walls IP.
4. **Note on Private DNS (DoH)**: If your phone has "Private DNS" enabled under *Network & internet*, Android will bypass local port 53 DNS and query Google/Cloudflare over HTTPS. Set *Private DNS* to **Off** or **Automatic** when using home DNS filtering appliances.

### iOS / iPadOS
1. Open **Settings** $\to$ **Wi-Fi**.
2. Tap the blue `(i)` next to your connected Wi-Fi network.
3. Scroll down to **Configure DNS** $\to$ select **Manual**.
4. Delete existing servers and add a server pointing to Hero's Walls IP.
5. Tap **Save**.

---

## 3. Verification & DNS Leak Testing

Once configured, verify that DNS requests are actively filtered:
1. Open a terminal and run `nslookup doubleclick.net`.
   - Expected Output: `Address: 0.0.0.0`
2. Run `nslookup google.com`.
   - Expected Output: Valid public Google IP address.
3. Visit the Hero's Walls web dashboard (`http://<device-ip>/`) to view the real-time query count and blocked ads counter increment.
