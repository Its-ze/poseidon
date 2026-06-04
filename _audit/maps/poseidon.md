# POSEIDON v0.6.1 Inventory Map

Generated 2026-06-01 from commit f5bfc66.

## Top-level groupings

| Group | File count | LOC total | Notes |
|-------|------------|-----------|-------|
| System core (main/app/menu/ui/theme/screensaver/splash/sfx/input/argus/sd) | 36 | ~7,500 | Includes all headers; screensaver.cpp alone is 1,090 LOC |
| WiFi (scan/deauth/beacon/portal/karma/wardrive/pmkid/spectrum/clients) | 18 | ~5,700 | Includes wifi_deauth_frame.h inline logic |
| BLE (scan/spam/hid/gatt/extras/finder/flood/findmy/sourapple/toys/whisperpair/blueducky) | 17 | ~5,200 | ble_db.cpp is a large OUI+FastPair lookup table |
| Sub-GHz + RF radios (CC1101/nRF24/nRF52/LoRa) | 24 | ~6,400 | Radio drivers + 10 sub-GHz feature modules + 5 nRF52 modules |
| Net attacks + comms (net_*/dhcp/mesh/satcom/c5/trident/mimir/triton) | 26 | ~9,600 | Largest group; triton.cpp alone is 1,457 LOC |
| SaltyJack (LAN pentest submenu) | 7 | ~2,850 | saltyjack/ subdir; port of Evil-M5Project RaspyJack |
| IR / GPS / specials (ir/badusb/drone/surveillance/defensive/tools/system) | 12 | ~4,600 | GPS-off tag lives here |
| c5_node (ESP32-C5 satellite firmware) | 9 | ~1,650 | Pure C ESP-IDF; separate from Arduino main app |

## Per-file inventory

### src/ (top-level .cpp)

- **main.cpp** — 192 LOC, boot sequence: IR park, LoRa RST hold, M5Cardputer.begin, TCA8418 keyboard, GPS + IR watchdog tasks, serial test, splash. Tags: [IR-active-low]
- **ui.cpp** — 933 LOC, all drawing primitives: status bar with C5 badge, footer, toast, animations (slide, spinner, notify, ripple, waves, radar, hexstream, glitch, eq_bars, dashboard_chrome, action_overlay, matrix_rain). Framebuffers heap-alloc'd to avoid PSRAM. Tags: [TX-cache]
- **menu.cpp** — 1,379 LOC, compile-time static menu tree with all feature forward-decls; terminal + carousel runtime; screensaver idle hook; NVS-backed style persistence.
- **theme.cpp** — 191 LOC, six curated 16-bit 565 palettes (POSEIDON/MATRIX/E-INK/SYNTHWAVE/PHANTOM/BLOOD), NVS persistence, live-preview without NVS write.
- **input.cpp** — 144 LOC, TCA8418 keyboard poller, modal line editor, injected-key ring for TRIDENT bridge, idle tracking for screensaver.
- **screensaver.cpp** — 1,090 LOC, 10 full-screen idle painters (wardrive sim, matrix rain, breathing, deep scan, port scan, hex cascade, terminal crack, neural arc, glitch BSOD, tide waves); NVS-backed enable/timeout/pick.
- **splash.cpp** — 148 LOC, animated boot splash: Hokusai Great Wave full-screen image with magenta scanline materialization sweep + fade-in + tag-line crawl. (sys-022 errata fix — earlier draft said "metallic trident sprite, title bloom, scanline sweep".)
- **sfx.cpp** — 238 LOC, tone-based SFX library (click, select, back, error, scan_hit, capture, cracked, boot jingle, alert, glitch); NVS-backed volume + mute.
- **argus.cpp** — 175 LOC, 48×48 RGB565 sprite renderer for Triton gotchi mood portraits; 10 moods, procedural overlays. Tags: [TX-cache]
- **ui_ambient.cpp** — 153 LOC, per-theme ambient layer renderer (matrix rain for MATRIX theme, no-op for E-INK, subtle particle drift for others).
- **ui_subghz.cpp** — 315 LOC, shared sub-GHz UI widgets: protocol label, pulse-train bar chart, frequency display; used by scan + replay.
- **radio.cpp** — 179 LOC, lazy domain switcher: tears down old radio domain before bringing up new one; `wifi_lean_sta_init` with reduced DMA buffers; raw IDF AP pattern reference. Tags: [WiFi-AP-IDF, BLE-coop, HSPI-park]
- **menu_carousel.cpp** — 417 LOC, big-card carousel renderer with corner brackets, large hotkey badge, slide animation, shared keyboard semantics with terminal style. Tags: [IR-active-low]
- **menu_icons.cpp** — 50 LOC, icon-drawing helpers (procedural pixel art, no bitmaps) used by carousel and SaltyJack views.
- ~~**menu_registry.cpp**~~ — Removed (Phase 0 / POS-AUDIT-004): was 201 LOC of dead self-registration with zero call sites.
- ~~**hat_manager.cpp**~~ — Removed (Phase 0 / POS-AUDIT sys-013): was 64 LOC stub that always returned `HatType::NONE`; no callers in the codebase.
- **sd_helper.cpp** — 142 LOC, idempotent SD mount, shared HSPI bus accessor `sd_get_spi()`, `sdlog_open()` canonical CSV logger bootstrap, FAT format. Tags: [HSPI-park]
- **gps.cpp** — 172 LOC, minimal NMEA GGA+RMC parser on UART1 (GPIO 15 RX, 13 TX), baud-cycle support, background snapshot API. Tags: [GPS-off]
- **lora_hw.cpp** — 252 LOC, SX1262 RadioLib wrapper via shared HSPI; PI4IOE5V6408 antenna switch on I2C; preset configs for 433/868/915/Meshtastic bands. Tags: [HSPI-park]
- **cc1101_hw.cpp** — 109 LOC, CC1101 ELECHOUSE wrapper; `cc1101_park_others()` forces SD CS high before SPI ops; GPS pin 13 conflicts with CC1101 CS. Tags: [HSPI-park]
- **cc1101_rmt.cpp** — 187 LOC, IDF 5.5 new-API RMT driver for CC1101 OOK TX/RX; pulse arrays in Flipper .sub format; owns channel + encoder lifecycle per call.
- **nrf24_hw.cpp** — 91 LOC, RF24 wrapper for nRF24L01+ on shared HSPI (CS=G6, CE=G4); parks SD/LoRa CS before ops. Tags: [HSPI-park]
- **nrf52_hw.cpp** — 119 LOC, UART1 (G3 TX, G4 RX) driver for Adafruit Feather nRF52840; send_command + streaming line reader; PING/PONG detection.
- **nrf52_led.cpp** — 164 LOC, cyberpunk NeoPixel animation controller for nRF52 companion; sends LED mode commands over UART; 24 defined modes.
- **satcom.cpp** — 251 LOC, SGP4 TLE fetch from Celestrak + SD cache; pass prediction with 30s step; 8 built-in NORAD favorites (ISS, Tiangong, NOAA, Hubble, etc).
- **mesh.cpp** — 180 LOC, ESP-NOW broadcast presence layer (KRAKEN PigSync); HELLO frames every 5s; 8-peer table with 30s eviction; GPS coordinates included if fix available.
- **c5_cmd.cpp** — 573 LOC, POSEIDON-side C5 satellite client; ESP-NOW peer table; sends v3 protocol commands (scan/deauth/pmkid/hs/clients/karma/spectrum/ciw); collects streamed responses.
- **net_helpers.cpp** — 65 LOC, shared utility: ARP sender, TCP port probe, OUI lookup helper.
- **dhcp_cache.cpp** — 125 LOC, transient DHCP lease cache for rogue-DHCP attacks; tracks offered IPs to avoid duplicate assignment.
- **subghz_decode.cpp** — 196 LOC, OOK protocol decoder: NEC, Samsung, RC5, RC6, Came, Nice — decodes raw pulse arrays into structured frames.
- **deauth_autotest.cpp** — 156 LOC, automated deauth self-test (3 stages: basic/burst/triton-hop); gated by `POSEIDON_AUTO_DEAUTH_TEST` define; NVS progress trace for reboot-visible results.
- **serial_test.cpp** — 71 LOC, USB serial test harness (K/S/R/? commands) for `scripts/test_all_features.py`; no-op for normal users.

### src/ (top-level .h)

- **app.h** — 32 LOC, shared palette macros (COL_*), display geometry constants (SCR_W/H, STATUS_H, BODY_Y, etc); M5Cardputer include.
- **theme.h** — 69 LOC, `poseidon_theme_t` struct, `theme_id_t` enum (6 themes), API declarations, T_* convenience macros replacing COL_* macros.
- **ui.h** — 121 LOC, all UI drawing function declarations including animations; `action_anim_t` enum.
- **input.h** — 44 LOC, `PK_*` key code enum, `input_poll`, `input_line`, `input_inject` declarations.
- **menu.h** — 59 LOC, `menu_node_t` struct, `menu_style_t` enum, `menu_run`, `g_current_feature_item` extern.
- **radio.h** — 46 LOC, `radio_domain_t` enum, `radio_switch`, `wifi_lean_sta_init`, `wifi_force_clean_sta` declarations.
- **gps.h** — 59 LOC, `gps_fix_t` struct (lat/lon/alt/sats/hdop/UTC), `gps_diag_t` struct, full GPS API.
- **wifi_types.h** — 27 LOC, `ap_t` struct, `g_last_selected_ap` / `g_last_selected_valid` externs.
- **ble_types.h** — 20 LOC, `ble_target_t` struct, `g_ble_target` / `g_ble_target_valid` externs.
- **ble_db.h** — 32 LOC, OUI/Apple/FastPair/UUID lookup declarations, `ble_db_identify` convenience wrapper.
- **wifi_wardrive.h** — 30 LOC, `wdr_ap_t` struct, `g_wdr_aps[]` / `g_wdr_ap_count` externs; wardrive data shared across Triton/scan.
- **wifi_deauth_frame.h** — 253 LOC, inline deauth/disassoc frame builder + `wifi_deauth_pair` + `wifi_deauth_broadcast` + `wifi_silent_ap_begin`; Porkchop-style WIFI_IF_STA TX. Tags: [WiFi-AP-IDF]
- **c5_cmd.h** — 272 LOC, v3 wire-protocol structs (packed), full command/response enum, complete C5 API surface.
- **mesh.h** — 46 LOC, ESP-NOW mesh API; `mesh_peer_t` struct; begin/stop/peers/tx_count/rx_count.
- **satcom.h** — 78 LOC, `satcom_tle_t`, `satcom_pos_t`, `satcom_pass_t`, `satcom_favorite_t`; fetch/compute/predict API.
- **lora_hw.h** — 46 LOC, `lora_band_t` enum, `lora_config_t`, RadioLib SX1262 begin/end/radio accessor.
- **cc1101_hw.h** — 25 LOC, pin defines (CS=13, GDO0=5), begin/end/set_freq/set_rx/set_tx/get_rssi/park_others.
- **cc1101_rmt.h** — 34 LOC, `cc1101_rmt_tx` + `cc1101_rmt_rx` for Flipper-format pulse arrays via RMT.
- **nrf24_hw.h** — 20 LOC, pin defines (CS=6, CE=4), begin/end/is_up/nrf24_radio accessor.
- **nrf52_hw.h** — 44 LOC, `NRF52Hardware` class; begin/end/is_up/send_command/available/read_line.
- **nrf52_led.h** — 78 LOC, `nrf52_led_mode_t` enum (24 modes), nrf52_led_set/oneshot.
- **sd_helper.h** — 53 LOC, sd_mount/is_mounted/format, sd_get_spi, sd_remount, sdlog_open.
- ~~**hat_manager.h**~~ — Removed alongside hat_manager.cpp (Phase 0 / sys-013).
- ~~**menu_registry.h**~~ — Removed alongside menu_registry.cpp (Phase 0 / POS-AUDIT-004).
- **subghz_types.h** — 26 LOC, `subghz_capture_t` (512 pulses, freq, ts), `g_subghz_last_cap` / `g_subghz_last_valid` externs.
- **nrf24_types.h** — 24 LOC, `nrf24_target_t` (addr/channel/type/packet_count), `g_nrf24_last_device` / `g_nrf24_last_valid` externs.
- **version.h** — 21 LOC, `poseidon_version()` + `poseidon_build_date()` inline wrappers; `-D` flag injection from platformio.ini.
- **screensaver.h** — 59 LOC, screensaver API + pool introspection; SCREENSAVER_PICK_SHUFFLE constant.
- **sfx.h** — 49 LOC, volume/mute API + full SFX function declarations.
- **ui_ambient.h** — ~30 LOC, ambient layer tick declaration.
- **ui_subghz.h** — ~30 LOC, sub-GHz UI widget declarations.
- **menu_carousel.h** — ~20 LOC, carousel entry point declaration.
- **menu_icons.h** — ~20 LOC, procedural icon drawing declarations.
- **argus.h** — 16 LOC, `argus_draw` + `argus_flash` declarations; references `argus_data.h`.
- **argus_data.h** — ~200 LOC, 10 × 48×48 RGB565 sprite arrays for Triton gotchi moods.
- **trident.h** — 3 LOC, `feat_trident` + `g_trident_cdc_active` extern.
- **mimir.h** — 14 LOC, `feat_mimir` + `g_mimir_cdc_active` extern.
- **dhcp_cache.h** — ~20 LOC, DHCP lease cache API.
- **subghz_decode.h** — ~20 LOC, `subghz_decode()` declaration.
- **net_helpers.h** — ~30 LOC, ARP/TCP probe helpers.
- **sigdb_bt.h** — ~30 LOC, BLE surveillance signature DB (manufacturer IDs, GATT UUIDs for hostile devices).
- **sigdb_surveillance.h** — ~30 LOC, WiFi BSSID/SSID signatures for Flock ALPR + ShotSpotter Raven.
- **wifi_portal_extras.h** — ~20 LOC, portal HTML helpers or extra template data.
- **badusb_extras.h** — ~50 LOC, Android-targeted DuckyScript payloads.
- **badusb_pranks_data.h** — ~50 LOC, prank DuckyScript payload list.
- **ble_blueducky.h** — ~10 LOC, BlueDucky feature entry point.
- **evil_twin.h** — ~10 LOC, evil_twin feature entry point.
- **ir_extras_data.h** — ~50 LOC, IR prank code database.
- **satcom_tle_baked.h** — ~30 LOC, fallback baked TLE data for offline operation.
- **serial_test.h** — ~10 LOC, `serial_test_init` declaration.
- **ap_signal_test.h** — ~10 LOC, `feat_ap_signal_test` declaration.
- **subghz_signals_data.h** — ~200 LOC, baked-in .sub signal payloads for broadcast feature.
- **splash_sprite.h** — ~100 LOC, trident splash sprite data for boot animation.
- **menu_icons_data.h** — ~100 LOC, icon pixel data arrays.

### src/features/ (WiFi)

- **wifi_scan.cpp** — 591 LOC, reference feature pattern: async scan → sorted results → hotkeys (D/C/I/P/O/filter); seeds `g_last_selected_ap`; C5 satellite status badge. Tags: [WiFi-AP-IDF]
- **wifi_deauth.cpp** — 316 LOC, targeted deauth: promisc client harvest + broadcast+unicast deauth pairs; 4-frame burst with 2ms spacing; PMF warning; FreeRTOS task for deauth loop. Tags: [WiFi-AP-IDF]
- **wifi_deauth_extras.cpp** — 355 LOC, broadcast deauth (nuke all APs in range) + passive deauth detector; C5 satellite deauth routing. Tags: [WiFi-AP-IDF]
- **wifi_beacon_spam.cpp** — 190 LOC, fake AP beacon flood: meme/rickroll/FBI/custom SSID lists; raw 802.11 beacon frame with rotating BSSIDs at 10 Hz. Tags: [WiFi-AP-IDF]
- **wifi_portal.cpp** — 664 LOC, evil captive portal: raw-IDF AP (NOT WiFi.softAP), DNS wildcard, HTTP server, 4 phishing templates (Google/Facebook/Microsoft/FreeWiFi), creds → SD. Tags: [WiFi-AP-IDF]
- **evil_twin.cpp** — 425 LOC, chained AP-clone + portal + periodic deauth; time-slices STA↔AP phases; same raw-IDF recipe; creds → SD. Tags: [WiFi-AP-IDF]
- **wifi_wardrive.cpp** — 279 LOC, channel-hop beacon logger → WiGLE v1.6 CSV; GPS-tagged rows; dedup by BSSID (best RSSI wins); seeds `g_wdr_aps` for Triton. Tags: [GPS-off]
- **wifi_pmkid.cpp** — 557 LOC, EAPOL capture → hashcat 22000 (WPA*01* PMKID + WPA*02* full 4-way); SSID cache from beacons; triggers deauth to force client reassociation. Tags: [WiFi-AP-IDF]
- **wifi_probe.cpp** — 517 LOC, probe sniffer + Karma attack; probe-response injection within 30ms window; background beacon spam for passive clients. Tags: [WiFi-AP-IDF]
- **wifi_clients.cpp** — 230 LOC, live client table for a single target AP; unicast deauth on selected client.
- **wifi_clients_all.cpp** — 263 LOC, global client hunter; channel-hop all channels; D/X/L/H hotkeys.
- **wifi_spectrum.cpp** — 435 LOC, 2.4 GHz 14-channel activity bar graph; 80ms dwell per channel; packet count + peak RSSI.
- **wifi_ciw.cpp** — 297 LOC, CIW zeroclick SSID injection: 14 payload categories (command injection, overflow, Log4Shell, etc.), 157 payloads; monitors disconnects as crash indicators. Tags: [WiFi-AP-IDF]
- **wifi_sanity_override.cpp** — 50 LOC, linker-override of `ieee80211_raw_frame_sanity_check` to always return 0 (allows deauth/disassoc TX); `wifi_deauth_last_rc` sentinel for UI display.
- **ap_signal_test.cpp** — 225 LOC, diagnostic raw-IDF AP ("POSEIDON-SIGTEST") to verify AP path; same recipe as portal; live STA count + TX power display. Tags: [WiFi-AP-IDF]

### src/features/ (BLE)

- **ble_scan.cpp** — 547 LOC, async NimBLE scan; type/vendor ID from manufacturer data + service UUIDs; seeds `g_ble_target`; hotkeys to child BLE features. Tags: [BLE-coop]
- **ble_spam.cpp** — 223 LOC, BLE advertisement flood: Apple/Samsung/Google/Windows/All modes; random MAC per broadcast; cooperative tick loop. Tags: [BLE-coop]
- **ble_hid.cpp** — 254 LOC, BLE HID keyboard via NimBLE HIDDevice; standard 8-byte input report descriptor; DuckyScript payload runner for BLE-paired hosts. Tags: [BLE-coop]
- **ble_whisperpair.cpp** — 715 LOC, CVE-2025-36911 detector: passive scan → connect → KBP write → 3s notify window → VULNERABLE/PATCHED verdict; logs to SD CSV. Tags: [BLE-coop]
- **ble_gatt.cpp** — 286 LOC, GATT explorer: connect → discover services/characteristics → read/write values; pocket nRF-Connect. Tags: [BLE-coop]
- **ble_extras.cpp** — 285 LOC, tracker detector + BLE sniffer CSV logger + iBeacon broadcaster.
- **ble_finder.cpp** — 457 LOC, physical tracker locator; Geiger-counter RSSI meter for a selected MAC; audio beep rate scales with signal.
- **ble_flood.cpp** — 135 LOC, connection flood against target MAC; random MAC cycling; defeats limited-connection peripherals (smart locks). Tags: [BLE-coop]
- **ble_karma.cpp** — 146 LOC, BLE Karma: responds to active scan requests by cycling popular device names in advertisement. Tags: [BLE-coop]
- **ble_clone.cpp** — 137 LOC, rebroadcast target device identity; sets random MAC to target MAC; causes pairing dedup collisions. Tags: [BLE-coop]
- **ble_findmy.cpp** — 180 LOC, Apple Find My / AirTag emulator; single or flock mode; rotating random public keys in offline-finding advertisement format. Tags: [BLE-coop]
- **ble_sourapple.cpp** — 461 LOC, multi-vendor BLE advertisement spam (Apple ProximityPair/NearbyAction/AirTag, Samsung SmartTag, Windows SwiftPair); cross-referenced attack variants. Tags: [BLE-coop]
- **ble_toys.cpp** — 284 LOC, "The Salty Deep": wireless sex toy scanner + intensity controller; Lovense ASCII protocol (Vibrate/Battery/DeviceType); connects via GATT. Tags: [BLE-coop]
- **ble_blueducky.cpp** — 610 LOC, BlueDucky BLE HID injection (CVE-2023-45866); auto-binds unpatched Android; DuckyScript-lite payload from BADUSB_ANDROID_PAYLOADS; 30-50 keys/sec cadence. Tags: [BLE-coop]
- **ble_db.cpp** — 353 LOC, OUI → vendor + Apple Continuity subtype + Google Fast Pair 24-bit model ID + BT SIG UUID lookup tables; `ble_db_identify` heuristic.

### src/features/ (Sub-GHz + RF Radios)

- **subghz_scan.cpp** — 304 LOC, CC1101 raw signal detector via GDO0 edge counting + tight poll loop; pulse validation (repeated-duration heuristic); saves to SD. Tags: [HSPI-park]
- **subghz_spectrum.cpp** — 420 LOC, professional spectrum analyzer: bar + waterfall + waveform modes; peak hold; dBm scale; sweeps 300–464 and 779–928 MHz bands. Tags: [HSPI-park]
- **subghz_replay.cpp** — 189 LOC, Flipper-compatible .sub file replay via CC1101 + RMT precise timing; reads RAW_Data pulse arrays from SD. Tags: [HSPI-park]
- **subghz_record.cpp** — 232 LOC, RAW signal capture via RMT on GDO0; saves Flipper-compatible .sub file to SD. Tags: [HSPI-park]
- **subghz_bruteforce.cpp** — 113 LOC, fixed-code protocol brute force: Came 12bit, Nice 12bit, Linear 10bit, Chamberlain 9bit, Holtek 12bit, Ansonic 12bit via RCSwitch. Tags: [HSPI-park]
- **subghz_broadcast.cpp** — 362 LOC, categorized .sub file browser + transmitter; browses /poseidon/signals/ (cars/pranks/tesla/custom); transmits via CC1101+RMT. Tags: [HSPI-park]
- **subghz_jammer.cpp** — 98 LOC, intermittent + continuous carrier jammer via CC1101 TX; 20s safety cap on full-carrier mode. Tags: [HSPI-park]
- **subghz_jam_detect.cpp** — 205 LOC, RSSI anomaly monitor on CC1101; 10s warmup baseline; sustained +15 dBm spike detection; siren audio alert + SD log. Tags: [HSPI-park]
- **nrf24_suite.cpp** — 771 LOC, comprehensive nRF24L01+ toolkit: promiscuous sniffer (Goodspeed SETUP_AW=0), MouseJack HID injection, BLE spam via nRF24 (ADV_IND + CRC24 + whitening), CW jammer (10 presets), 2.4 GHz scanner, ESB replay. Uses `sd_get_spi()` for raw register writes to avoid FSPI/M5GFX conflict. Tags: [HSPI-park]
- **lora_spectrum.cpp** — 389 LOC, LoRa band activity monitor; single-freq RSSI time-series (bars/waterfall/scope modes); detects incoming LoRa packets with RSSI/SNR/size/hex. Tags: [HSPI-park]
- **radio_lora.cpp** — 365 LOC, LoRa UI features (band picker shared by all): passive RX scan → SD, POSEIDON beacon TX with GPS coords, Meshtastic LongFast listener, GPS fix live view. Tags: [HSPI-park, GPS-off]
- **nrf52_suite.cpp** — 415 LOC, nRF52840 Feather hat menu: BLE 5.0 Full Scan, Long-Range BLE (Coded PHY S=8), BLE Advertisement Sniffer (pcap), 802.15.4 Zigbee Sniffer, BLE Connection MITM sub-feature dispatch.
- **nrf52_ble_mitm_relay.cpp** — 317 LOC, BLE MITM transparent relay: ESP32 NimBLE advertises clone → nRF52 connects real device → full L2CAP/ATT relay with logging and optional modification. Tags: [BLE-coop]
- **nrf52_scout_strike.cpp** — 284 LOC, dual-radio Scout & Strike: ESP32 NimBLE smart scan (Phase 1) → nRF52 raw sniff/flood/disconnect on target (Phase 2).
- **nrf52_wifi_ble_combo.cpp** — 213 LOC, coordinated WiFi+BLE attack: ESP32 deauths smart home device off WiFi → nRF52 captures BLE provisioning fallback handshake. Tags: [WiFi-AP-IDF, BLE-coop]
- **rf_finder.cpp** — 227 LOC, hot/cold signal locator for nRF24 (channel/ESB device tracking) and CC1101 (sub-GHz RSSI); thermometer bar + audio beep rate scaling. Tags: [HSPI-park]

### src/features/ (Net Attacks + Comms)

- **net_attacks.cpp** — 906 LOC, 6 offensive network features: UART Shell (serial bridge), Reverse TCP (connect-back relay), Telnet Honeypot (fake login → SD log), WiFi Dead Drop (hidden AP + portal), Printer Detect (ARP + port 9100), SSDP Poisoner (fake UPnP NOTIFY). Tags: [WiFi-AP-IDF]
- **net_dhcp.cpp** — 509 LOC, DHCP starvation + rogue DHCP (STA mode) + rogue DHCP (AP mode) + Network Hijack chain; ported from Evil-M5Project. Tags: [WiFi-AP-IDF]
- **net_wpad.cpp** — 556 LOC, WPAD abuse + Exchange Autodiscover credential capture; SoftAP + DNS wildcard + wpad.dat PAC + 407 NTLM harvest; ported from Evil-M5Project. Tags: [WiFi-AP-IDF]
- **net_responder.cpp** — 246 LOC, LLMNR + NBT-NS + mDNS poisoner; TCP 445 SMB stub for NTLMv2 capture → hashcat mode 5600.
- **net_lanrecon.cpp** — 406 LOC, RaspyJack-style LAN recon chain: ARP sweep → port scan → banner grab → OUI lookup → default-cred probe → CSV export.
- **net_cctv.cpp** — 588 LOC, CCTV/IP camera recon: port probe (80/443/554/8080-8083/8443/8554) → HTTP fingerprint → default-cred spray → RTSP stream probe; CSV to SD. Credit: @7h30th3r0n3.
- **net_tools.cpp** — 151 LOC, basic network utilities: TCP port scan, ICMP ping, DNS lookup; requires STA association.
- **net_ssdp.cpp** — 207 LOC, UPnP/SSDP discovery scanner; M-SEARCH to multicast 239.255.255.250:1900; XML description fetch; friendlyName/modelName/serial extraction.
- **c5_scan.cpp** — 967 LOC, dual-band WiFi scan via C5 satellite + Zigbee sniffer display; shows C5 connection status; integrates with `g_last_selected_ap` for chain-to-deauth.
- **triton.cpp** — 1,457 LOC, autonomous handshake hunter: FreeRTOS background task with channel-hop promisc + EAPOL parse + deauth every 3s; 4 modes (HUNT/STEALTH/SURGICAL/STORM); Argus gotchi mood state machine; parallel wardrive CSV. Tags: [GPS-off, WiFi-AP-IDF]
- **trident.cpp** — 279 LOC, PC Bridge for TRIDENT desktop app; scanline-chunk framebuffer streaming over USB-CDC (JSON header + raw RGB565); remote keypress injection via `input_inject`. Tags: [TX-cache]
- **mimir.cpp** — 745 LOC, MIMIR pentest drop-box C2 controller; newline-delimited JSON over USB-CDC; manages `g_mimir_cdc_active` exclusivity flag.
- **feat_satcom.cpp** — 263 LOC, satellite tracker UI: pick favorite → fetch TLE (HTTPS/cache) → live polar skyplot + az/el/lat/lon → pass-predict list. Tags: [GPS-off]
- **mesh_chat.cpp** — 133 LOC, live Meshtastic text chat: split-screen log + input line; ENTER broadcasts text.
- **mesh_nodes.cpp** — 122 LOC, Meshtastic node roster: SNR/RSSI/hops/last-seen/GPS; ENTER opens page feature.
- **mesh_page.cpp** — 116 LOC, unicast Meshtastic text message composer; two entry paths (pick node or pre-selected).
- **mesh_position.cpp** — 78 LOC, toggle Meshtastic background position broadcast; shows current GPS fix state.
- **mesh_status.cpp** — 59 LOC, live ESP-NOW PigSync mesh status display; peer table, HELLO counts.
- **mesh/meshtastic_node.cpp** — 591 LOC, full Meshtastic leaf-node stack: background RX FreeRTOS task, AES-CTR decryption, protobuf decode, node roster + message ring, TX (broadcast + unicast + NodeInfo + Position).
- **mesh/meshtastic_crypto.cpp** — 60 LOC, AES-CTR encrypt/decrypt for Meshtastic default channel PSK (LongFast key).
- **mesh/meshtastic_pb.cpp** — 353 LOC, minimal hand-written protobuf decoder for Data/Position/User/MeshPacket; no generated code dependency.

### src/features/ (IR / GPS / Specials)

- **ir_remote.cpp** — 274 LOC, virtual Samsung TV remote mapped to Cardputer keys; LEDC 38 kHz carrier with `output_invert=1`; active-LOW GPIO 44. Tags: [IR-active-low]
- **ir_tvbgone.cpp** — 187 LOC, TV-B-Gone: fires condensed subset of NEC/Sony/RC5/RC6 power-off codes; bit-banged carrier on GPIO 44 (active-LOW, HIGH=off). Tags: [IR-active-low]
- **ir_clone.cpp** — 479 LOC, multi-profile IR remote with 3 built-in TV profiles (Samsung/LG/Sony); LEDC carrier on GPIO 44; v2 will add RX capture + .ir file storage. Tags: [IR-active-low]
- **badusb.cpp** — 413 LOC, USB HID keyboard payload runner via ESP32-S3 native USB; DuckyScript-lite (REM/DELAY/STRING/GUI/CTRL/ALT/SHIFT/COMBO); loads from SD /poseidon/ducky/*.txt or built-in library.
- **drone_remoteid.cpp** — 314 LOC, passive FAA Part 89 Remote ID detector (BLE UUID 0xFFFA); decodes BasicID + Location + System messages; JSONL log to SD. Tags: [BLE-coop, GPS-off]
- **surveillance_hunter.cpp** — 355 LOC, passive Flock Safety ALPR + ShotSpotter Raven detector; 2.4 GHz promisc beacon/probe scorer against sigdb_surveillance.h; CSV + JSONL SD output. Tags: [GPS-off]
- **defensive_monitor.cpp** — 769 LOC, multi-class counter-surveillance: deauth flood detector, evil twin detector (same SSID / dual BSSID), beacon spam rate detector; JSONL SD log + audio alert. Tags: [GPS-off]
- **system_tools.cpp** — 360 LOC, saved WiFi connect, SD file browser, NTP clock, settings menu (big-text toggle, menu style).
- **tools.cpp** — 341 LOC, utility collection: SD format, flashlight, screen test, stopwatch, dice, morse code, MAC randomize, RPN calculator.
- **theme_picker.cpp** — 72 LOC, visual theme selector with live in-RAM preview + NVS commit on ENTER; ESC restores original.
- **sfx_settings.cpp** — 67 LOC, volume +/- and mute toggle UI; both persist to NVS immediately.
- **screensaver_picker.cpp** — 124 LOC, pick specific screensaver or SHUFFLE mode; P previews without commit; ENTER commits to NVS.
- **stubs.cpp** — 32 LOC, About screen only (version + build date).

### src/features/saltyjack/

- **saltyjack_menu.cpp** — 495 LOC, custom RaspyJack-faithful submenu renderer: solid-highlight list + info blurb + `i`=info page + `v`=cycle LIST/GRID/CAROUSEL; 30s screensaver takeover.
- **saltyjack_info.cpp** — 52 LOC, SaltyJack homage page: credits @7h30th3r0n3, links to Evil-M5Project + RaspyJack.
- **saltyjack_dhcp_starve.cpp** — 320 LOC, DHCP starvation: requires STA association; random-MAC Discover→Offer→Request→ACK loop; pool exhaustion detection via NAK. Ported from @7h30th3r0n3.
- **saltyjack_dhcp_rogue.cpp** — 425 LOC, rogue DHCP server in STA mode (races real server) and AP mode (serves own clients); attacker-controlled gateway + DNS injection. Tags: [WiFi-AP-IDF]
- **saltyjack_responder.cpp** — 619 LOC, LLMNR + NBT-NS poisoner + SMB2 NTLMv2 harvester; full NTLMSSP Type-1→2→3 state machine; hashcat-ready hash output to SD. Ported from @7h30th3r0n3.
- **saltyjack_ntlm_crack.cpp** — 496 LOC, on-device NTLMv2 wordlist cracker: HMAC-MD5 implementation; reads /poseidon/saltyjack/wordlist.txt; displays cracked credentials.
- **saltyjack_wpad.cpp** — 437 LOC, WPAD abuse + Exchange Autodiscover 407 NTLM harvest; SoftAP + DNS wildcard + PAC server + HTTP 407 proxy challenge. Ported from @7h30th3r0n3. Tags: [WiFi-AP-IDF]

### src/features/mesh/ (Meshtastic protocol stack)

- **meshtastic_node.cpp** — 591 LOC, Meshtastic leaf node: background RX task (FreeRTOS), AES-CTR decrypt, protobuf decode, node roster (max 32), message ring (max 40), TX path for broadcast/unicast/NodeInfo/Position; mesh_tick for periodic beacons.
- **meshtastic_crypto.cpp** — 60 LOC, AES-CTR encrypt/decrypt using mbedTLS; hardcoded default LongFast PSK.
- **meshtastic_pb.cpp** — 353 LOC, hand-written varint + length-delimited protobuf decoder; handles MeshPacket/Data/Position/User messages without generated code.

### c5_node/ (ESP32-C5 satellite firmware — pure C + IDF)

- **main/main.c** — 304 LOC, C5 node boot: NVS + WiFi init in STA+Null (for ESP-NOW), ESP-NOW callbacks, HELLO broadcast task, command dispatch loop for all CMD_* types from S3.
- **main/proto.h** — 242 LOC, wire protocol v3 shared with S3 side (`c5_cmd.h` mirror); all packed structs for request/response messages; POSEI_MAGIC = "POSE".
- **main/proto.c** — 35 LOC, protocol version helper; compile-time assert that struct sizes are correct.
- **main/wifi_attacker.c** — 200 LOC, 802.11 deauth on ESP32-C5: 5 GHz channel support (C5's headline feature vs S3 2.4 GHz only); targeted + broadcast modes; RESP_STATUS streaming.
- **main/pmkid_capture.c** — 234 LOC, PMKID capture on C5: promiscuous mode EAPOL-Key M1 PMKID KDE parser; streams WPA*01* records back via ESP-NOW RESP_PMKID.
- **main/hs_capture.c** — 276 LOC, full 4-way handshake capture on C5; ANonce cache + M2 MIC extraction → hashcat 22000 WPA*02* records; streams via RESP_HS.
- **main/zb_sniffer.c** — 127 LOC, IEEE 802.15.4 Zigbee frame sniffer using C5's native 802.15.4 radio; streams channel/RSSI/frame_type/PAN/addrs via RESP_ZB.
- **main/led_fx.c** — 216 LOC, C5 RGB LED effects driver; boot animation, scanning pulse, capture burst, TX strobe, idle breathing.
- **main/led_fx.h** — 13 LOC, LED mode enum and `led_fx_set` declaration.

---

## Public API surface

Headers included by 3 or more other files (based on grep include counts):

- **app.h** → `COL_*` palette macros, `SCR_W/H`, `STATUS_H`, `BODY_Y/H`, `FOOTER_Y/H` — included by 61+ files
- **ui.h** → `ui_init`, `ui_clear_body`, `ui_force_clear_body`, `ui_draw_status`, `ui_status_invalidate`, `ui_draw_footer`, `ui_toast`, `ui_text`, `ui_text_w`, `ui_splash`, `ui_body_println`, `ui_slide_transition`, `ui_spinner`, `ui_notify_slide`, `ui_ripple`, `ui_matrix_rain`, `ui_waves`, `ui_radar`, `ui_hexstream`, `ui_glitch`, `ui_eq_bars`, `ui_action_overlay`, `ui_dashboard_chrome`, `ui_freq_bars`, `ui_big_text`, `ui_big_text_set`, `action_anim_t` — included by 59+ files
- **input.h** → `PK_NONE`, `PK_ENTER`, `PK_ESC`, `PK_BKSP`, `PK_TAB`, `PK_SPACE`, `PK_UP`, `PK_DOWN`, `PK_LEFT`, `PK_RIGHT`, `PK_FN`, `input_poll`, `input_last_key`, `input_last_input_ms`, `input_line`, `input_inject` — included by 59+ files
- **radio.h** → `radio_domain_t`, `radio_switch`, `radio_current`, `radio_name`, `wifi_force_clean_sta`, `wifi_lean_sta_init` — included by 49+ files
- **theme.h** → `poseidon_theme_t`, `theme_id_t`, `theme_init`, `theme_set`, `theme_preview`, `theme_current_id`, `theme`, `T_BG`, `T_FG`, `T_ACCENT`, `T_ACCENT2`, `T_WARN`, `T_BAD`, `T_GOOD`, `T_DIM`, `T_SEL_BG`, `T_SEL_BD` — included by 10 files directly (all others via app.h transitively)
- **c5_cmd.h** → `c5_begin`, `c5_stop`, `c5_any_online`, `c5_peer_count`, `c5_last_seen_ms`, `c5_peer_name`, `c5_cmd_scan_5g`, `c5_cmd_scan_zb`, `c5_cmd_stop`, `c5_cmd_ping`, `c5_cmd_deauth`, `c5_cmd_pmkid`, `c5_cmd_hs`, `c5_cmd_clients_hunt`, `c5_cmd_clients_ap`, `c5_cmd_beacon_spam`, `c5_cmd_probe_sniff`, `c5_cmd_deauth_detect`, `c5_cmd_karma`, `c5_cmd_apclone`, `c5_cmd_spectrum`, `c5_cmd_ciw`, `c5_deauth_dashboard`, `c5_aps`, `c5_zbs`, `c5_pmkids`, `c5_hss`, `c5_stas`, `c5_probes`, `c5_deauth_hits`, `c5_spectrum_get`, `c5_clear_results`, plus all packed struct types — included by 8 files
- **sd_helper.h** → `sd_mount`, `sd_is_mounted`, `sd_format`, `sd_get_spi`, `sd_remount`, `sdlog_open` — included by 14+ files
- **gps.h** → `gps_fix_t`, `gps_diag_t`, `gps_begin`, `gps_end`, `gps_poll`, `gps_cycle_baud`, `gps_current_baud`, `gps_get`, `gps_snapshot`, `gps_diag`, `GPS_UART_RX_PIN`, `GPS_UART_TX_PIN` — included by 15 files
- **wifi_deauth_frame.h** → `wifi_silent_ap_begin`, `wifi_silent_ap_end`, `wifi_deauth_pair`, `wifi_deauth_broadcast`, `wifi_auth_has_pmf`, `wifi_silent_ap_set_source_mac` (inline statics) — included implicitly by deauth users
- **lora_hw.h** → `lora_band_t`, `lora_config_t`, `lora_preset`, `lora_band_name`, `lora_begin`, `lora_end`, `lora_is_up`, `lora_radio` — included by LoRa + mesh stack
- **mesh/meshtastic.h** → `mesh_node_t`, `mesh_message_t`, `MESH_FREQ_MHZ`, `MESH_MAX_PAYLOAD`, `mesh_begin`, `mesh_end`, `mesh_is_up`, `mesh_own_node_id`, `mesh_own_long_name`, `mesh_own_short_name`, `mesh_send_broadcast_text`, `mesh_send_direct_text`, `mesh_send_nodeinfo`, `mesh_send_position`, `mesh_nodes`, `mesh_messages`, `mesh_snapshot_messages`, `mesh_drain_new_message`, `mesh_clear_messages`, `mesh_set_position_reporting`, `mesh_position_reporting`, `mesh_tick` — included by 5 mesh feature files
- **nrf52_hw.h** → `NRF52Hardware::begin`, `end`, `is_up`, `send_command`, `available`, `read_line` — included by 5 nRF52 feature files
- **sfx.h** → `sfx_init`, `sfx_set_volume`, `sfx_get_volume`, `sfx_set_mute`, `sfx_is_muted`, `sfx_click`, `sfx_select`, `sfx_back`, `sfx_error`, `sfx_toast`, `sfx_scan_start`, `sfx_scan_hit`, `sfx_deauth_burst`, `sfx_capture`, `sfx_cracked`, `sfx_boot`, `sfx_alert`, `sfx_glitch` — included by 10+ files

---

## Gotcha tag distribution

| Tag | Files | Notes |
|-----|-------|-------|
| TX-cache | ui.cpp, argus.cpp, trident.cpp | ui.cpp heap-allocs slide/notify framebuffers (fixed, no longer static BSS). argus.cpp calls pushImage during WiFi feature redraws — benign because 4.6 KB/frame is below DMA stall threshold. trident.cpp uses readRect per scanline during active streaming. |
| WiFi-AP-IDF (GOOD — raw IDF) | wifi_portal.cpp, evil_twin.cpp, ap_signal_test.cpp, wifi_probe.cpp (AP mode path), net_attacks.cpp, net_dhcp.cpp (AP mode), net_wpad.cpp, saltyjack_dhcp_rogue.cpp (AP mode), saltyjack_wpad.cpp | All use `esp_wifi_set_config(WIFI_IF_AP, ...)` + `esp_wifi_start()` raw IDF recipe. |
| WiFi-AP-IDF (BAD — Arduino softAP) | wifi_ciw.cpp (uses WiFi.mode(WIFI_AP) path — RISKY) | wifi_ciw.cpp is the only active user of Arduino's WiFi.mode(WIFI_AP) — potential instability under Bruce libs. |
| BLE-coop | ble_scan.cpp, ble_spam.cpp, ble_hid.cpp, ble_whisperpair.cpp, ble_gatt.cpp, ble_flood.cpp, ble_karma.cpp, ble_clone.cpp, ble_findmy.cpp, ble_sourapple.cpp, ble_toys.cpp, ble_blueducky.cpp, ble_extras.cpp, drone_remoteid.cpp, nrf52_ble_mitm_relay.cpp, nrf52_wifi_ble_combo.cpp | All BLE features use cooperative tick; none create xTaskCreate around NimBLE work. ble_spam.cpp and ble_sourapple.cpp internally use xTaskCreate for a separate spam_task — these are NOT around NimBLE init, they run after NimBLE is already initialized, so they are safe. |
| IR-active-low | main.cpp, ir_remote.cpp, ir_tvbgone.cpp, ir_clone.cpp, menu_carousel.cpp (IR state check) | main.cpp parks GPIO 44 HIGH at boot + watchdog task. ir_remote.cpp correctly uses LEDC `output_invert=1`. ir_tvbgone.cpp and ir_clone.cpp use bit-banged carrier (active-LOW: HIGH=off) — correct behavior, no LEDC invert needed for bit-bang. |
| GPS-off | wifi_wardrive.cpp, triton.cpp, surveillance_hunter.cpp, drone_remoteid.cpp, defensive_monitor.cpp, feat_satcom.cpp, radio_lora.cpp, mesh/meshtastic_node.cpp, mesh.cpp, mesh_position.cpp, gps.cpp | All GPS-tagged captures are gated: wardrive writes GPS fields only when `gps_snapshot()` returns true AND `g.valid`. triton.cpp's wardrive CSV file is only opened when a GPS fix is present during the session. No feature writes GPS coordinates unconditionally. |
| HSPI-park | cc1101_hw.cpp, nrf24_hw.cpp, nrf24_suite.cpp, lora_hw.cpp, sd_helper.cpp, subghz_scan.cpp, subghz_replay.cpp, subghz_record.cpp, subghz_broadcast.cpp, subghz_jammer.cpp, subghz_spectrum.cpp, subghz_jam_detect.cpp, subghz_bruteforce.cpp, lora_spectrum.cpp, radio_lora.cpp, rf_finder.cpp | All share the HSPI bus (SCK=40, MISO=39, MOSI=14). SD uses CS=12 (`sd_helper.cpp:24 #define SD_CS 12`), LoRa uses CS=G5, CC1101 uses CS=13, nRF24 uses CS=G6. cc1101_hw.cpp calls `cc1101_park_others()` to pull other CS lines HIGH before SPI ops. nrf24_suite.cpp explicitly uses `sd_get_spi()` for raw register writes to avoid FSPI/M5GFX bus contention. radio_switch(RADIO_SUBGHZ) calls `gps_end()` because CC1101 CS=13 conflicts with GPS UART TX=13. (Errata: earlier draft said SD CS=10 and listed `hat_manager.cpp` which was dead code deleted under Phase 0 hygiene.) |
