# Evil-M5Project Reference Map

Commit: a23525b
Cloned: 2026-06-01
Repo: https://github.com/7h30th3r0n3/Evil-M5Project

Single-sketch architecture — all features live in one giant `.ino`
per board. Cardputer-Advance build (the relevant one for POSEIDON) is
`Evil-Cardputer-v1-5-2.ino` (~38k lines). Menu is one flat `menuItems[]`
array (line 233) dispatched by a giant `switch(case)` in
`executeMenuItem()` around line 2622.

## LAN attack feature index (PRIMARY)

| Feature | File:line | Notes |
|---------|-----------|-------|
| DHCP starvation | Evil-Cardputer-v1-5-2.ino:17296+, entry 17509 `startDHCPStarvation` | Floods random-MAC `DHCPDISCOVER` via UDP/67 (`sendDHCPDiscover` :17691). Detects target server via `detectDHCPServer` :17432. Counters: discover/offer/request/ack/nak. |
| DHCP server (rogue) | :16888 “Rogue DHCP”, :16926 `rogueDHCP(RogueDhcpMode)` | Two modes: `ROGUE_DHCP_STA` (race real DHCP on joined LAN) and `ROGUE_DHCP_AP` (we are DHCP for SoftAP clients, stops built-in `esp_netif_dhcps_stop` :16947 then restores). Hand-rolled DHCP packet build (`prepareDHCPResponse` :17062, `sendDHCPResponse` :17232). Offer suffix starts at .101. WPAD option 252 :17205, PAC URL `http://evilm5.lan/wpad.dat` :17206. |
| Auto-pick (DHCP) | :2673 case 51 `DHCPAttackAuto()` | Chooses starvation vs rogue based on detected server. |
| Responder LLMNR | :20876+ block, listener bound :21585 `llmnrUDP.beginMulticast(224.0.0.252, 5355)` | Answers any LLMNR A query with attacker IP. Hand-built 16/28-byte response (:21736, :21749). |
| Responder NBT-NS | :21581 `nbnsUDP.begin(137)`, handler :21608+ | Accepts any NBNS name query (`0x20` type). Builds 62-byte reply :21659. Logs name + protocol (`lastQueryName`/`lastQueryProtocol`). |
| NTLMv2 capture | NTLM block starts ~:4656, type-2 builder :4719, hash save :4704 | Server challenge generated via `esp_random` :4769. Detects Type-1/Type-3 messages. Writes `username::domain:serverChal:NTproof:blob` to `/evil/NTLM/ntlm_hashes.txt`. Tracks `ntlmHashCount`, `ntlmLastUser/Domain/Client`. |
| NTLMv2 cracking (on-device) | :2683 case 61, impl `crackNTLMv2` :26615 | Reads `/evil/NTLM/ntlm_wordlist.txt`, HMAC-MD5 over NTLM hash, writes plaintext to `/evil/NTLM/ntlm_found.txt`. Auto-creates wordlist if missing :26563. |
| NTLM hash de-dup | :2684 case 62, `CleanNTLMHashes` :27025 | Removes duplicate hash lines from `/evil/NTLM/ntlm_hashes.txt`. |
| WPAD abuse | :2682 (case 60 “WPAD Abuse”), main path :3897 `server.on("/wpad.dat", ...)`, NTLM client :4880 `handleNTLMClient` | Two WPAD paths: (a) PAC inside captive portal sends `PROXY <apIP>:80` :3898; (b) standalone WPAD-only server. Forces 407 Proxy-Authenticate negotiation :4857, sends NTLM Type-2 (b64) :4869, parses Type-3 :4956. Also serves a "DIRECT" PAC trap to silence Windows NCSI loops :36674. |
| SSDP fake/poison | :2693 case 71 `fakeSSDP()`, impl :27437 + server defs :27094-27275 | Spawns up to `MAX_SSDP_DEVICES=300` fake devices, replies to `ssdp:all` / `upnp:rootdevice` / specific `urn:schemas-upnp-org:device:*` M-SEARCHes. Serves XML SCPD on port 80 (`ssdpServer.onNotFound` :27238). Device names loaded from `/evil/config/SSDPName.txt`. |
| UPnP discovery / NAT | :2699 case 77 `listUPnPMappings`, :2700 case 78 `upnpTargetNATWorkflow` | Lists existing UPnP IGD mappings and crafts NAT-add requests against discovered IGDs. |
| mDNS poison | NOT IMPLEMENTED | No `MDNS_PORT` / `5353`-bound handler found. Responder covers NBNS/LLMNR only. |
| DNS spoof / captive DNS | DNSServer (Arduino) on :225, `dnsServer.start(53,"*",ipAP)` at :3892 / :5260 / :28449 / :28741 / :37165 | Pure captive wildcard `*`; "Switch DNS" (case 49) toggles AP↔STA bind :17985. No per-name spoof table — wildcard only. |
| Network Hijacking | :285 menu, dispatch :2671 case 50 (`networkHijacking`) | Combines DNS hijack + rogue-DHCP-style coercion. Display string :18095. |
| ARP table scan | :13509 `read_arp_table`, scan helper :13564 | Uses `etharp` (lwIP). Read-only — no ARP-spoof / ARP-poison frames sent. Used by `Scan Network Hosts` (case 37). |
| ARP spoof | NOT IMPLEMENTED | Only ARP scan / table read via `etharp`. No gratuitous-ARP / poison frame sender. |
| PCAP logging | Header structs :10696 `pcap_hdr_t`, :10704 `pcaprec_hdr_t`, writer :10717 | Promisc mode capture, `.pcap` global standard format (linktype 105 802.11). PCAP browser list `pcapFiles` :11607. Files live under `/` SD root with `.pcap` extension. Multi-handshake parser limit `MAX_HS_PER_PCAP=12` :113. |
| Captive cookie siphon | :3895 `/siphon`, `/logcookie`, writer `/evil/cookies.log` :3708 | JS-side payload posts captured cookies back to portal. |
| Reverse TCP tunnel | :2668 case 46, impl :16656 `reverseTCPTunnel` | Connects out to `tcp_host:tcp_port` from SD config. |

## WiFi / portal features

| Feature | File:line | Notes |
|---------|-----------|-------|
| Captive portal (Evil Twin core) | :2628 case 6 `createCaptivePortal`, AP-bringup :3861-3892 | SoftAP + DNS wildcard + `WebServer(80)`. Up to 10 clients (`max_clients=10` :3870). Optional WPA2 password. |
| Portal template loading | :3918 `SD.open(selectedPortalFile)` then `server.streamFile` | Static HTML served direct from SD. IP-rewrite variant :3927 `servePortalFileWithReplace`. |
| Evil Twin combined | :2647 case 25 `startEvilTwin(currentListIndex)`, impl :20154 | Clones the targeted SSID + spawns captive portal. |
| Credential capture | `saveCredentials` :5719, store `/evil/credentials.txt` | Plain-text format: `-- Email --\n... -- Password --\n... -- Portal --\n... -- SSID --\n... ------------------`. LED flash blue x2 on capture. |
| Credential webview | :2631 case 9 `checkCredentials`, :2632 case 10 wipe | Browsable from on-device LCD and from the web admin panel. |
| Portal change/select | :2630 case 8 `changePortal`, dir scan `/evil/sites` :3138 | Cycles through HTML files in `/evil/sites`. |
| Karma attack | :248-250 menu, FSM :2373 (StartScanKarma…SelectSSIDKarma), `karmaAttack` (case 14), `startAutoKarma` (15), `karmaSpear` (16) | Listens for client probe-requests, then beacons matching SSIDs. AutoKarma rotates channels 1,6,11 (see `karmaChannels` :737). Whitelist via SD config `KarmaAutoWhitelist`. |
| Probe sniffing/replay | :2634 case 12 probeAttack, :2635 case 13 probeSniffing | Saves probes to `/evil/probes.txt` (+ timestamped variant). |
| Deauth | :2645 case 23 `deauthAttack`, impl :10829 | Standard ESP32 raw 802.11 deauth (`esp_wifi_set_mode(WIFI_MODE_STA)` then `esp_wifi_80211_tx`). |
| Auto Deauth + Pwnagotchi detect | :2646 case 24, related vars :796-811 | Periodic scan + deauth all-or-targeted. |
| Sniff + deauth clients | :2652 case 30 `deauthClients`, :2653 case 31 `deauthDetect` | Promisc client sniff, EAPOL window after deauth (`deauthWaitingTime=5000` :829). |
| Handshake / PMKID capture | EAPOL counter :809, PCAP writer above, `Check Handshakes` case 32 | Pcap-based; multi-handshake parser. |
| Beacon spam | :2644 case 22 (`Beacon Spam`) | Spam from `CustomBeacons` config list. |
| Wardriving | :2642-43 cases 20/21, helpers in `utilities/wardriving/` | GPS-tagged `.csv` to SD. |

## Other / system

| Feature | File:line | Notes |
|---------|-----------|-------|
| Menu / UI | `menuItems[]` :233 (85 entries), zoom 1.5/2.0/2.5 :2539, dispatcher `executeMenuItem` ~:2620 | One flat list. No submenus — each line is a leaf. |
| SD config | `/evil/config/config.txt` (INI-ish), reader `restoreConfigParameter` (see :1978) | Key=value pairs: brightness, soundOn, ledOn, volume, theme path, custom probe/beacon SSID lists, llm_*, cloned_ssid, portal_file, portal_password, portal_ip_sel, startatboot/casetostartatboot/boot_countdown, GPS pin mode, CPU freq. |
| Boot-direct-to-feature | `caseToStartAtBoot` :749, settings helpers :165-167 | Auto-launch any menu case on boot, optional countdown. |
| Themes | INI via `IniFile` :180, themes `/evil/theme.ini` + `/evil/theme_laika.ini` | Selectable from settings. |
| Web admin panel | `WebServer server(80)` :224, HTML dashboard inline :535-621 | Tabs: Dashboard / Files / Upload / Portal / Monitor / Scan / Credentials / BadUSB. Pass `accessWebPassword` :352 (default `7h30th3r0n3`). |
| BLE Name Flood | :308 menu, case 73-ish via `BLEDevice::init` :28939 | Spams advertising names. |
| AirTag / FindMyEvil | :309-310 menu, TX :29927 `FindMyEvilTx`, keys SD `/evil/FindMyEvil_keys.txt` | Apple OF / AirTag spoof advertisements. |
| Skyjack (Parrot drone) | :2694 case 72 `skyjackDroneMode` :27718 | Hunts Parrot AR.Drone-style SSIDs. |
| LDAPDump | :313, :30988+ | Anonymous LDAP enum against target DC. |
| IMSI Catcher | :314, log `/evil/IMSI-catched.txt` | Limited — opportunistic. |
| EvilChat / LLM | menu :290-291, llm_* config keys | Talks to external Ollama via `llm_host` / `llm_api_path`. |
| SSH client / shell | libssh-esp32 :188 | Outbound shell from device. |
| UART shell | menu :298 | |
| SD-on-USB | menu :292 | Exposes SD over MSC. |

## Portal template system (deep-dive)

- **Template directory**: `SD-Card-File/sites/` (deployed to SD at `/evil/sites/`).
- **Format**: bare static HTML, one file per template. POST to `/` with form
  fields `email` + `password`; capture done in inline `server.on("/", HTTP_GET, ...)`
  at :3903 (and a matching POST handler nearby). The HTML may include JS to
  also `fetch('/siphon', ...)` cookies and `/logcookie`.
- **Brand coverage**: core set (top of `/sites/`) — Amazon, Facebook, Instagram,
  Microsoft, Netflix, Google ("FR-New-google"), Twitch, X, WhatsApp, YouTube,
  Starbucks, Holy-Portal (generic), AutoInfoStealer, WebSiphonCookie. Plus
  "Dropper" payloads (ClickFix, FileFix, ClipboardS, SCStealer, apk, ps1-lolbas,
  CVE-2025-24054), `snake.html`, `WebForkBomb.html`, RickRoll. **31 base
  templates** + **27 community templates** under `sites/community-sites/`
  (DE/EN/ES/FR/NL/PT-BR localizations).
- **Credential POST endpoint**: `POST /` on AP web server (port 80). Saved by
  `saveCredentials(email, password, portalName, clonedSSID)` :5719.
- **IP families** (`portal_ip_sel` :1981 / config): `0` = 192.168.4.1 (default
  ESP AP), `1` = 172.0.0.1 (forces replace in HTML). Switchable from menu.
- **Adding new templates**: drop `*.html` into `/evil/sites/`. `Change Portal`
  menu (case 8) re-scans dir. No manifest, no metadata.

## Hardware bring-up patterns relevant to LAN attacks

- **WiFi AP setup**: 99% Arduino `WiFi.softAP(ssid, pass, channel, hidden,
  max_clients=10)` :3874/3880. Falls back to raw IDF only in a few feature
  paths (`esp_wifi_init(&cfg)` at :8395, :9188, :9213, :10557, :10645, :10852;
  `esp_wifi_set_mode(WIFI_MODE_AP)` :14090). DHCPS owned by ESP-NETIF
  (`esp_netif_get_handle_from_ifkey("WIFI_AP_DEF")` :16946), stopped before
  rogue-DHCP-AP runs and restarted after.
- **DHCP**: roll-your-own packet build (`prepareDHCPResponse` :17062,
  `sendDHCPResponse` :17232). Sends/receives via `WiFiUDP udp` on port 67.
  Magic cookie + option 53 / 252 (WPAD) etc. encoded by hand.
- **DNS**: Arduino `DNSServer` wildcard only — no per-name spoof map; for
  precision DNS hijack POSEIDON would need its own resolver.
- **LWIP raw**: `etharp.h` :183 (ARP table reads), no raw socket beyond
  `WiFiUDP`/`WiFiClient` for net attacks.
- **RAM cost per client**: SoftAP `max_clients=10`; SSDP keeps `MAX_SSDP_DEVICES=300`
  device metadata in PSRAM (`evilAlloc` :197 prefers `MALLOC_CAP_SPIRAM`).
- **GPS**: `TinyGPSPlus` :135, baud configurable via `baudrate_gps`.

## Build / target boards

- Single `.ino` per board. **No PlatformIO** — Arduino-IDE only.
  Compile prerequisites in `utilities/compilation_prerequisites/`.
- Sketches present: Cardputer-Adv (`Evil-Cardputer-v1-5-2.ino`), M5Core2
  (`Evil-M5Core2-1-5-1.ino`), M5Core3 (`Evil-M5Core3-1-1-9.ino`), AtomS3, NanoC6,
  StickC (Beta), M5Dial (Semi-Evil), Cheap Yellow Display (CYD), Face / Face-Dial.
  ADV firmware in `binaries/Evil-ADV-V1.4.5.bin`.
- Includes (Cardputer build, :125-190): `M5Unified`, `M5Cardputer`, `WebServer`,
  `DNSServer`, `SD`, `TinyGPSPlus`, `Adafruit_NeoPixel`, `ArduinoJson`, `esp_now`,
  `esp_netif`, `BLEDevice`/`BLEScan`/`BLEAdvertisedDevice`, `esp_task_wdt`,
  `HTTPClient`, `WiFiClientSecure`, `IniFile`, `lwip/etharp.h`, `ESPping`,
  `libssh_esp32`.

## Anything Evil-M5 does that POSEIDON's saltyjack module does NOT appear to do

`saltyjack/` ports six attacks: `dhcp_starve`, `dhcp_rogue` (STA + AP),
`responder` (LLMNR/NBNS/SMB), `wpad` (407 + PAC), `ntlm_crack`. Gaps vs Evil-M5:

- **Network Hijacking combo** (Evil case 50) — orchestrated DHCP+DNS+WPAD attack chain. saltyjack has the pieces but no "run all" wrapper.
- **DHCPAttackAuto** (Evil case 51) — auto-pick starve vs rogue based on detected server.
- **Switch DNS** flip (Evil case 49 :17985) — toggle AP/STA bind for the captive DNS.
- **SSDP fake/poisoner** (Evil case 71 `fakeSSDP`) — 300-device fake UPnP world.
- **UPnP NAT abuse** (Evil cases 77/78) — `listUPnPMappings` + `upnpTargetNATWorkflow`.
- **NTLM hash de-dup** (Evil case 62 `CleanNTLMHashes`) — saltyjack has crack but no clean-up pass.
- **ARP table scan** integration (`read_arp_table` :13509) — recon prerequisite for selective LLMNR/relay targeting; saltyjack responder is broadcast-only.
- **PCAP/SD logging** for net attacks — saltyjack writes plain-text hash files only; no `.pcap` for DHCP/responder/WPAD flows.
- **mDNS poisoner (`udp/5353`)** — Evil-M5 also lacks this; both leave macOS / iOS clients unaffected. Worth flagging as a shared gap.
- **Cookie siphon endpoint** (`/siphon`, `/logcookie`) — Evil pairs WPAD with cookie capture, saltyjack does not.
- **Web admin / remote review** — Evil exposes captured creds + hashes via HTTP dashboard; saltyjack info page is on-device only (`saltyjack_info.cpp`).
