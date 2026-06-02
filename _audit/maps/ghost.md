# Ghost ESP Reference Map

Commit: 109ccde
Cloned: 2026-06-01
Repo: https://github.com/Spooks4576/Ghost_ESP

Stack: ESP-IDF (CMake, not PlatformIO/Arduino). LVGL/M5GFX UI. NimBLE. Cardputer is a first-class build target (`configs/sdkconfig.cardputer`).

## Feature index

| Feature | File:line | Notes |
|---------|-----------|-------|
| WiFi scan (APs) | main/managers/wifi_manager.c:944 `wifi_manager_start_scan` | command `scanap`; OUI-vendor tagging w/ Netgear/TPLink lookup (wifi_manager.c:175,187) |
| WiFi STA/clients sniff | main/managers/wifi_manager.c:331 `wifi_stations_sniffer_callback` + 1040 `wifi_manager_list_stations` | promiscuous mode; command `scansta` |
| WiFi LAN scan | main/managers/wifi_manager.c:1699 `wifi_manager_scan_subnet` + 2265 `wifi_manager_start_ip_lookup` | ICMP-style host probe; `scanlocal` |
| WiFi port scan | main/managers/wifi_manager.c:1633 `scan_ports_on_host` + 1866 `scan_ip_port_range` | command `scanports` |
| WiFi deauth | main/managers/wifi_manager.c:1099 `wifi_manager_broadcast_deauth` + 1196 `wifi_deauth_task` + 1250 `wifi_manager_start_deauth` | 26-byte deauth+disassoc templates, channel hop 1-11, FreeRTOS task. Sanity-check bypass: `main/main.c:23 ieee80211_raw_frame_sanity_check` returns 0 |
| WiFi auto-deauth | main/managers/wifi_manager.c:2006/2074 `wifi_auto_deauth_task` | flipper-style continuous on USB build (`USB_MODULE`) |
| WiFi beacon spam | main/managers/wifi_manager.c:2153 `wifi_manager_broadcast_ap` + 2396 `wifi_beacon_task` + 2430 `wifi_manager_start_beacon` | modes: `-r` random, `-rr` rickroll lyrics pool (cmdline.c:117), `-l` AP-list, or explicit SSID. Builds 802.11ax HE Capabilities IE (Wi-Fi 6 advertise) |
| WiFi probe capture | main/core/callbacks.c:587 `wifi_probe_scan_callback` + cmdline.c:654 `capture -probe` | pcap output |
| WiFi PMKID | (absent — no dedicated PMKID extraction) | only `capture -eapol` to PCAP for offline cracking (cmdline.c:704, callbacks.c:639) |
| WiFi EAPOL capture | main/core/callbacks.c:639 `wifi_eapol_scan_callback` | command `capture -eapol`, writes to pcap |
| WiFi pwnagotchi detect | main/core/callbacks.c:626 `wifi_pwn_scan_callback` + 484 `is_pwn_response` | command `capture -pwn` |
| WiFi WPS detect | main/core/callbacks.c:652 `wifi_wps_detection_callback` | command `capture -wps` |
| WiFi PineAP detect | main/core/callbacks.c:174/192/307 `start_pineap_detection`/`log_pineap_detection`/`wifi_pineap_detector_callback` + cmdline.c:1231 | command `pineap` (rogue-AP heuristic vs known BSSIDs) |
| WiFi evil portal | main/managers/wifi_manager.c:726 `wifi_manager_start_evil_portal` + 682 `start_portal_webserver` | AP+STA mode, IDF httpd, captive-portal redirects for Android `/generate_204`, Apple `/hotspot-detect.html`, MS `/connecttest.txt`; portal HTML pulled from a URL (PORTALURL) — see Captive Portal section |
| WiFi wardrive | main/core/callbacks.c:505 `wardriving_scan_callback` + main/managers/gps_manager.c:160 `gps_manager_log_wardriving_data` | CSV on SD, GPS-tagged. Toggled by `attack -w` from cmdline.c:809 |
| WiFi raw frame TX bypass | main/main.c:23 `ieee80211_raw_frame_sanity_check` returns 0 | unblocks `esp_wifi_80211_tx` for deauth/beacon (called wifi_manager.c:2223 etc.) |
| WiFi connect (STA) | main/managers/wifi_manager.c:2341 `wifi_manager_connect_wifi` + cmdline `connect` | for STA-side ops |
| Flipper deauth/portal stop hook | main/core/commandline.c:221 `handle_stop_flipper` | command `stop` |
| BLE init / stack | main/managers/ble_manager.c:530 `ble_init` + 50 `nimble_host_task` + 551 `nimble_port_init` | NimBLE only (not Bluedroid) |
| BLE scan generic | main/managers/ble_manager.c:465 `ble_start_scanning` + handler-registration pattern (493 `ble_register_handler`) | handlers: callbacks chained off `ble_gap_event` |
| BLE Find-the-Flippers | main/managers/ble_manager.c:222 `ble_findtheflippers_callback` + 574 `ble_start_find_flippers` | command `blescan -f`; targets Flipper Zero BLE advert |
| BLE spam DETECTOR | main/managers/ble_manager.c:348 `detect_ble_spam_callback` + 653 `ble_start_blespam_detector` | DETECTION only; **Ghost does NOT transmit BLE spam** |
| BLE AirTag SCANNER | main/managers/ble_manager.c:389 `airtag_scanner_callback` + 663 `ble_start_airtag_scanner` | observe-only, no spoof TX |
| BLE raw packet scan | main/managers/ble_manager.c:324 `ble_print_raw_packet_callback` + 658 `ble_start_raw_ble_packetscan` | bluetooth.pcap capable |
| BLE skimmer detect | main/managers/ble_manager.c:117/746 `ble_stop_skimmer_detection` / `ble_start_skimmer_detection` + callbacks.c:827 `ble_skimmer_scan_callback` | command `capture -skimmer`, writes BT pcap |
| BLE wardrive | main/core/callbacks.c:769 `ble_wardriving_callback` + cmdline.c:1192 `handle_ble_wardriving` | CSV on SD, GPS-tagged |
| BLE flood / sour apple / spam TX | absent | no BLE advert/spam transmitter at all |
| BLE GATT/HID/BlueDucky/MITM/clone/karma/whisperpair/findmy | absent | no GATT server, no HID stack — BLE is observe-only |
| Sub-GHz / CC1101 / nRF24 / IR / LoRa / Mesh | absent | not in scope |
| TV-B-Gone | absent | n/a |
| RemoteID / Drone | absent | n/a |
| Net DHCP/Responder/WPAD/SSDP/CCTV/LANRecon | absent (only LAN port scan) | wifi_manager.c subnet scan is the only LAN attack |
| DIAL / Chromecast / YouTube cast | main/managers/dial_manager.c (entire) + cmdline.c:236 `handle_dial_command` | command `dialconnect`, hits `youtube.com/api/lounge/bc/bind` (dial_manager.c:278) — "TV cast" attack |
| TP-Link smart-plug | main/core/commandline.c:518/526/534 `encrypt_tp_link_command`/`decrypt`/`handle_tp_link_test` | XOR scheme; command `tplinktest` |
| Power Printer | main/vendor/printer.c + cmdline.c:1066 / `powerprinter` | mass-send print jobs |
| System: main | main/main.c:25 `app_main` | minimal: serial → wifi_init → (optional ble_init commented) |
| System: command line | main/core/commandline.c:1448 `register_commands` | 33 commands total, table at file bottom |
| System: serial mgr | main/core/serial_manager.c | UART input dispatcher |
| System: system mgr | main/core/system_manager.c | tasks/state |
| System: DNS server (captive) | main/core/dns_server.c (301 lines) | wildcard DNS for captive portal |
| UI: LVGL display mgr | main/managers/display_manager.c | LVGL screen stack |
| UI: views | main/managers/views/{splash,main_menu,options,terminal,number_pad,music_visualizer,flappy_ghost,app_gallery,error_popup}_screen.c | 9 screens |
| UI: input (joystick + keyboard) | main/managers/joystick_manager.c + main/vendor/m5/m5stack_keyboard.c | Cardputer keyboard matrix at GPIOs 8/9/11 out, 13/15/3/4/5/6/7 in |
| UI: app gallery / flappy ghost / music visualizer | main/managers/views/*.c | mini-apps |
| Settings | main/managers/settings_manager.c + include/managers/settings_manager.h | NVS-backed; RGB mode/speed, AP SSID/PW, portal URL/SSID/PW/AP_SSID/domain/offline-mode, timezone, accent color, RTS, channel delay, broadcast speed, flappy name |
| SD card | main/managers/sd_card_manager.c | mount `/mnt/ghostesp/{pcaps,scans,gps,games,debug,wardriving}` |
| GPS | main/managers/gps_manager.c + main/vendor/GPS/{MicroNMEA,gps_logger}.c | NMEA parser, wardrive CSV (WiGLE format) |
| PCAP writer | main/vendor/pcap.c | `PCAP_CAPTURE_WIFI` / `PCAP_CAPTURE_BLUETOOTH` |
| RGB FX | main/managers/rgb_manager.c + main/vendor/led/* | modes: rainbow, police-siren, strobe (settings-driven speed); single-LED + matrix paths |
| BadUSB | absent | no HID gadget |
| Web UI (AP-mode REST) | main/managers/ap_manager.c:846 `ap_manager_start_services` + URI table 675-712 | REST surface — see Web UI section |
| Embedded portal HTML | include/managers/ghost_esp_site.h (3988-line `const char[]`) + scripts/site/ghost_site.html (2017 lines) | full LVGL-like control SPA served from ESP |
| Desktop control app | scripts/control app/esp_ghost_control.py (PyQt6) | UART-side companion GUI |

## Captive portal / evil portal template system

Ghost is **NOT** template-library based (no TP-Link/Netgear/Google HTML bundles on flash). It is **URL-driven**:

- Operator stores a `PORTALURL` (settings_manager NVS key) pointing at any external HTTPS page.
- `wifi_manager.c:585 portal_handler` does `stream_data_to_client(req, PORTALURL, "text/html")` — server proxies the remote portal HTML on-the-fly.
- `wifi_manager.c:562 file_handler` does the same for `.png/.jpg/.css/.js` by rebuilding the URL against the originating Host header.
- Credentials capture: `wifi_manager.c:627 get_info_handler` parses `email` + `password` from a GET query — caller's portal must POST/GET them to `/get`. Currently they are only `printf`'d (not persisted to SD in this commit).
- Captive-redirect endpoints registered (wifi_manager.c:686-712): `/login`, `/generate_204` (Android), `/hotspot-detect.html` (Apple), `/connecttest.txt` (MS), 404 fallback.
- Offline mode flag exists (`settings_get_portal_offline_mode` cmdline.c:427) — gates whether to also join an upstream STA first.
- POSEIDON gap: POSEIDON likely ships local SD-backed HTML templates per-brand; Ghost's model is "operator-hosted page".

## Web UI / REST control surface

`main/managers/ap_manager.c` runs a second httpd (separate from the evil-portal one) when device is in normal AP mode. Routes (lines 675-712 / dup at 882-919):

| Method | URI | Handler | Purpose |
|--------|-----|---------|---------|
| GET | `/` | `http_get_handler` | serves SPA from `include/managers/ghost_esp_site.h` (PROGMEM blob) |
| GET | `/api/settings` | get | dump current FSettings |
| POST | `/api/settings` | set | mutate settings |
| GET | `/api/sdcard` | list | SD directory listing |
| POST | `/api/sdcard/download` | download | pull file off SD |
| POST | `/api/sdcard/upload` | upload | push file onto SD |
| DELETE | `/api/sdcard` | `api_sd_card_delete_file_handler` (ap_manager.c:277) | delete file |
| POST | `/api/command` | exec | run a Ghost CLI command remotely |
| GET | `/api/logs` | log buffer | tail buffer populated via `ap_manager_add_log` (ap_manager.c:802) |

Auth model: **none** — open SoftAP, no token, no basic auth. Same exposure for both portal and control AP modes.

File serving: embedded `const char[]` (no SD dependency for the SPA itself); SD only used for pcaps/scans/gps/wardriving and portal assets in offline-mode if any.

## Hardware bring-up patterns

- WiFi init: `wifi_manager.c:884 wifi_manager_init` → `WIFI_INIT_CONFIG_DEFAULT` → `esp_wifi_init` → mode `WIFI_MODE_APSTA`. Monitor mode flips through `WIFI_MODE_NULL` + `esp_wifi_set_promiscuous(true)` (lines 866-878).
- Raw frame TX path: `esp_wifi_80211_tx(WIFI_IF_AP, packet, size, false)` — requires `ieee80211_raw_frame_sanity_check` stub returning 0 (main.c:23).
- BLE init: NimBLE only (`nimble_port_init` ble_manager.c:551, `nimble_host_task` :50). Bluedroid not used. Gated by `#ifndef CONFIG_IDF_TARGET_ESP32S2` because S2 has no BT.
- Beacon packet: hand-rolled 802.11 management frame in `wifi_manager_broadcast_ap` (wifi_manager.c:2153), 11-channel hop with 10 ms delay, randomized MAC per channel, adds 802.11ax HE Capabilities vendor IE for Wi-Fi 6 spoofing.
- Cardputer keyboard: bit-banged matrix scan at `main/vendor/m5/m5stack_keyboard.c` — outputs 8/9/11, inputs 13/15/3/4/5/6/7. Custom `Keyboard_t` driver, not the M5 Arduino lib.
- LED strip: bundled RMT + SPI WS2812 drivers under `main/vendor/led/`.

## Build / target boards

Single ESP-IDF project, switched at build time via `configs/sdkconfig.<board>`:

- **`sdkconfig.cardputer`** — `CONFIG_USE_CARDPUTER=y`, M5Stack Cardputer (original, not Advance)
- `sdkconfig.marauderv4`, `sdkconfig.marauderv6` — Marauder hardware
- `sdkconfig.ghostboard` — Ghost-branded board
- `sdkconfig.S3TWatch`, `sdkconfig.JC3248W535EN`, `sdkconfig.awokmini`
- `sdkconfig.CYDMicroUSB`, `sdkconfig.CYDDualUSB`, `sdkconfig.CYD2USB`, `sdkconfig.CYD2USB2.4Inch[_C_Varient]` — Cheap Yellow Display family
- `sdkconfig.sunton7inch`, `sdkconfig.crowtech7inch`, `sdkconfig.waveshare7inch` — large displays
- `sdkconfig.devkit.esp32{c3,c6}`, `sdkconfig.default.esp32{,s2,s3,c3,c6}`
- `sdkconfig.IRAMOptimization` — overlay
- Cardputer-**Advance** (POSEIDON's target) is **NOT** a Ghost build variant.

Notable build-time switches: `CONFIG_WITH_SCREEN`, `CONFIG_WITH_ETHERNET`, `USB_MODULE` (auto-deauth on boot), `DEBUG` (exposes `crash` cmd).

## Capability deltas — what Ghost does that POSEIDON likely does not

- Stream-from-URL evil portal (proxy any external HTML through the captive AP) — no local template engine required.
- Full HTTP REST + embedded SPA control surface (`/api/command`, `/api/settings`, `/api/sdcard/*`, `/api/logs`) served from a 3988-line embedded HTML blob.
- DIAL / Chromecast / YouTube-Lounge hijack (`dialconnect` — casts videos to TVs on the LAN via the YouTube lounge bind API).
- TP-Link Kasa smart-plug remote control with the XOR-key encryption (`tplinktest`).
- "Power Printer" mass print abuse (`powerprinter`).
- PineAP rogue-AP DETECTION callback (`pineap`).
- Pwnagotchi presence detection (`capture -pwn`, `is_pwn_response`).
- BLE skimmer detection callback specifically tuned for card-skimmer adverts.
- BLE detection-only model: ghost spam DETECTOR (not transmitter), AirTag SCANNER (not spoofer) — POSEIDON's BLE is presumably TX-side.
- Sanity-check bypass shim (`ieee80211_raw_frame_sanity_check` stub) — must verify POSEIDON uses the same trick or an equivalent Bruce-lib path.
- WiGLE-format wardrive CSV output (both WiFi and BLE), with GPS NMEA.
- PyQt6 desktop control panel (`scripts/control app/esp_ghost_control.py`) over UART.
- 802.11ax HE-Capabilities IE injection in beacon spam (advertises fake Wi-Fi 6 APs).
- AP-list beacon-spam mode that re-broadcasts the SSIDs of currently scanned APs.

## Capability deltas — what POSEIDON has that Ghost does not (high-level)

- Sub-GHz (CC1101), nRF24, IR (TV-B-Gone), LoRa, Mesh — entirely absent in Ghost.
- BLE TX-side: spam / sour-apple / flood / WhisperPair / Karma / clone / GATT / HID / BlueDucky / MITM / AirTag spoof / FindMy — all absent in Ghost.
- LAN attack toolkit (DHCP, Responder, WPAD, SSDP, CCTV, captive-portal templates per-brand) — Ghost has only port-scan + subnet ICMP.
- Drone RemoteID — absent in Ghost.
- BadUSB / HID stack — absent in Ghost (Cardputer KB is input-side only).
- Cardputer-Advance hardware support — Ghost only targets original Cardputer.
