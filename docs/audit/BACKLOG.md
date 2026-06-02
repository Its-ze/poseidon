# POSEIDON Audit Backlog

Source: 2026-06-01 v0.6.1 full audit (commit f5bfc66). Tickets sorted by severity, then phase, then module. Top 20 audit defects mapped to POS-AUDIT-001..020. Phase 3 cross-ref gaps mapped to POS-AUDIT-101..150. Module-level findings mapped to POS-AUDIT-200+.

## Index

| ID | Sev | Phase | Module | Title |
|----|-----|-------|--------|-------|
| POS-AUDIT-001 | CRIT | 1 | System/IR | IR LED parked LOW (= ON) at every park site (8+ files) |
| POS-AUDIT-002 | CRIT | 1 | SubGHz | subghz_jam_detect exit paths skip radio_switch(RADIO_NONE) |
| POS-AUDIT-003 | CRIT | 1 | WiFi | wifi_ciw.cpp Arduino softAP + per-rotate re-attach + APSTA teardown |
| POS-AUDIT-004 | CRIT | 0 | System | Dead menu_registry.cpp/.h ~280 LOC + 110 KB BSS |
| POS-AUDIT-005 | CRIT | 1 | System/c5_node | zb_sniffer ISR calls esp_now_send + led_fx_set |
| POS-AUDIT-006 | HIGH | 1 | BLE | feat_ble_scan bypasses radio_switch(RADIO_BLE) |
| POS-AUDIT-007 | HIGH | 1 | WiFi | Unconditional esp_bt_controller_mem_release in 3 sites |
| POS-AUDIT-008 | HIGH | 1 | System | radio.cpp WiFi.mode(WIFI_OFF) violates PORKCHOP |
| POS-AUDIT-009 | HIGH | 1 | WiFi | wifi_portal/evil_twin blocking overlay halts DNS+HTTP |
| POS-AUDIT-010 | HIGH | 1 | Net/SaltyJack | Arduino WiFi.softAP in 5 sites |
| POS-AUDIT-011 | HIGH | 1 | BLE | Indefinite scans without setMaxResults(0) in 7 sites |
| POS-AUDIT-012 | HIGH | 1 | SubGHz | cc1101_end doesn't restore GDO0/CS pins |
| POS-AUDIT-013 | HIGH | 1 | SubGHz | subghz_record 20s RX exceeds TWDT |
| POS-AUDIT-014 | HIGH | 1 | IR/specials | drone_remoteid SD I/O inside portENTER_CRITICAL |
| POS-AUDIT-015 | HIGH | 1 | IR/specials | defensive_monitor NimBLE init/deinit every 5s + alert MPSC race |
| POS-AUDIT-016 | HIGH | 1 | IR/specials | Triton 1457 LOC freeze regression hypothesis cluster |
| POS-AUDIT-017 | HIGH | 1 | System | sfx NVS handle leak + SFX blocking input loop |
| POS-AUDIT-018 | HIGH | 1 | System/c5_node | Shared s_frame[26] between two wifi_attacker tasks |
| POS-AUDIT-019 | HIGH | 1 | WiFi | wifi_pmkid blocking tone+delay while RX task fires |
| POS-AUDIT-020 | HIGH | 1 | Net | mesh.cpp WiFi.getMode() fails after raw-IDF init |
| POS-AUDIT-021 | HIGH | 1 | WiFi | wifi_scan rescan xTaskCreate 8KB stack vs canary risk |
| POS-AUDIT-022 | HIGH | 1 | WiFi | wifi_ciw 10KB CiwPayload BSS competes with WiFi driver |
| POS-AUDIT-023 | HIGH | 1 | BLE | ble_hid/blueducky double-deinit esp_wifi without gate |
| POS-AUDIT-024 | HIGH | 1 | SubGHz | nrf24_suite raw SPI register writes bypass RF24 lib shadow state |
| POS-AUDIT-025 | HIGH | 1 | SubGHz | subghz_replay picker misses /poseidon/signals/custom/ subdir |
| POS-AUDIT-026 | HIGH | 1 | SubGHz | lora_hw doesn't park CC1101/nRF24 CS on entry |
| POS-AUDIT-027 | HIGH | 1 | Net | satcom 8s blocking http.GET freezes UI |
| POS-AUDIT-028 | HIGH | 1 | IR | ir_remote/tvbgone/clone polarity bug already merged into POS-AUDIT-001 |
| POS-AUDIT-029 | HIGH | 1 | IR/specials | defensive_monitor alert MPSC merged into POS-AUDIT-015 |
| POS-AUDIT-030 | HIGH | 0 | WiFi | Verify -Wl,-zmuldefs in platformio.ini per memory invariant |
| POS-AUDIT-031 | HIGH | 1 | WiFi | Drop STA→AP reverse-pair in deauth_pair for broadcast (2x airtime) |
| POS-AUDIT-032 | HIGH | 0 | System | Adopt PORKCHOP heap_policy.h state machine [also in POS-AUDIT-118] |
| POS-AUDIT-101 | HIGH | 3 | BLE | Port Bruce HID Exploit Engine 9-tactic |
| POS-AUDIT-102 | HIGH | 3 | BLE | Port Bruce WhisperPair multi-variant exploits |
| POS-AUDIT-103 | MED | 3 | Net | Port Ghost DIAL/Chromecast hijack |
| POS-AUDIT-104 | MED | 3 | Net | Port Ghost TP-Link Kasa control |
| POS-AUDIT-105 | LOW | 3 | System | Port Bruce mquickjs JS interpreter (large effort) |
| POS-AUDIT-106 | LOW | 3 | Net | Port Bruce Wireguard/SSH/SOCKS4/Telnet |
| POS-AUDIT-107 | LOW | 3 | Specials | Bruce RFID/NFC stack (hardware eval first) |
| POS-AUDIT-108 | LOW | 3 | Net | Bruce W5500 wired LAN (hat eval first) |
| POS-AUDIT-109 | HIGH | 3 | Net/SaltyJack | Evil-M5 Network Hijack combo wrapper |
| POS-AUDIT-110 | MED | 3 | Net/SaltyJack | Evil-M5 DHCPAttackAuto |
| POS-AUDIT-111 | MED | 3 | Net | Evil-M5 Switch DNS AP↔STA bind toggle |
| POS-AUDIT-112 | MED | 3 | Net | Evil-M5 SSDP fake-300 poisoner |
| POS-AUDIT-113 | LOW | 3 | Net | Evil-M5 UPnP NAT abuse |
| POS-AUDIT-114 | HIGH | 3 | WiFi | Evil-M5 captive cookie siphon endpoint |
| POS-AUDIT-115 | MED | 3 | Net/SaltyJack | Evil-M5 NTLM hash de-dup pass |
| POS-AUDIT-116 | HIGH | 3 | WiFi | Evil-M5 portal SD template loader (31+27) |
| POS-AUDIT-117 | MED | 3 | System | Evil-M5 boot-to-feature (kiosk drop-box) |
| POS-AUDIT-118 | HIGH | 3 | System | PORKCHOP heap-pressure state machine |
| POS-AUDIT-119 | HIGH | 3 | System | PORKCHOP 80KB Reservation Fence pre-WiFi init |
| POS-AUDIT-120 | MED | 3 | System | PORKCHOP heap watermark persistence to SD |
| POS-AUDIT-121 | MED | 3 | System | PORKCHOP canGrow vector-growth gate |
| POS-AUDIT-122 | MED | 3 | WiFi | PORKCHOP BOAR BROS wardrive exclusion list |
| POS-AUDIT-123 | LOW | 3 | WiFi | PORKCHOP adaptive 4-state DNH channel hopper |
| POS-AUDIT-124 | LOW | 3 | BLE | PORKCHOP NimBLE internal-only allocation cflag |
| POS-AUDIT-125 | MED | 3 | System | PORKCHOP crash viewer (SD-backed) |
| POS-AUDIT-126 | MED | 3 | System | PORKCHOP first-class diagnostics menu |
| POS-AUDIT-127 | MED | 3 | WiFi | PORKCHOP/Bruce hidden-SSID reveal via broadcast deauth |
| POS-AUDIT-128 | MED | 3 | WiFi | Ghost 802.11ax HE Capabilities IE in beacons |
| POS-AUDIT-129 | MED | 3 | WiFi | Ghost AP-list beacon spam mode |
| POS-AUDIT-130 | LOW | 3 | Net | Ghost Power Printer mass-print |
| POS-AUDIT-131 | MED | 3 | IR/specials | Ghost PineAP rogue-AP detection |
| POS-AUDIT-132 | MED | 3 | IR/specials | Ghost Pwnagotchi presence detection |
| POS-AUDIT-133 | MED | 3 | IR/specials | Ghost BLE skimmer detection |
| POS-AUDIT-134 | MED | 3 | BLE | Ghost BLE wardrive (WiGLE CSV + GPS) |
| POS-AUDIT-135 | LOW | 3 | BLE | Ghost BLE pcap writer |
| POS-AUDIT-136 | LOW | 3 | System | Ghost REST + SPA control surface |
| POS-AUDIT-137 | MED | 3 | WiFi | Ghost EAPOL→PCAP alongside hashcat 22000 |
| POS-AUDIT-138 | MED | 3 | SubGHz | Bruce nRF24 drone FHSS jammer profile |
| POS-AUDIT-139 | LOW | 3 | IR | Bruce IR jammer (multi-protocol noise) |
| POS-AUDIT-140 | HIGH | 3 | IR | Bruce IR clone with RX capture (TODO already in code) |
| POS-AUDIT-141 | LOW | 3 | SubGHz | Bruce sub-GHz listen (pulse-decode passthrough audio) |
| POS-AUDIT-142 | HIGH | 3 | Net/SaltyJack | mDNS poisoner reply path (gap in both POSEIDON+Evil-M5) |
| POS-AUDIT-143 | LOW | 3 | BLE | Bruce Ninebot scooter unlock |
| POS-AUDIT-144 | LOW | 3 | Specials | Evil-M5 Skyjack Parrot drone hunter |
| POS-AUDIT-145 | INFO | 4 | Specials | Evil-M5 IMSI catcher (out of scope, no LTE) |
| POS-AUDIT-146 | LOW | 3 | Net | Evil-M5 LDAP anonymous bind enum |
| POS-AUDIT-147 | LOW | 3 | System | Evil-M5 web admin dashboard (overlaps 136) |
| POS-AUDIT-148 | MED | 3 | WiFi | Bruce WiGLE.net upload |
| POS-AUDIT-149 | MED | 3 | WiFi | PORKCHOP wpasec upload |
| POS-AUDIT-150 | HIGH | 3 | Specials | BadUSB SD payload loader (promised, unimplemented) |
| POS-AUDIT-200 | MED | 1 | WiFi | wifi_beacon_spam s_beacon race main↔task |
| POS-AUDIT-201 | MED | 2 | WiFi | wifi_deauth_extras silent_ap_begin scope inconsistency |
| POS-AUDIT-202 | MED | 1 | WiFi | wifi_clients sync deauth burst from UI task (~1s freeze) |
| POS-AUDIT-203 | MED | 1 | WiFi | wifi_probe TX from RX cb context |
| POS-AUDIT-204 | MED | 2 | WiFi | wifi_portal/evil_twin new DNSServer/WebServer every cycle |
| POS-AUDIT-205 | MED | 1 | WiFi | wifi_pmkid m2_len cap to 260 before hex_append |
| POS-AUDIT-206 | MED | 1 | WiFi | wifi_pmkid hunt_task reads s_cache[] without snapshot |
| POS-AUDIT-207 | MED | 1 | WiFi | wifi_wardrive first_seen timestamp vs current gps_snapshot mix |
| POS-AUDIT-208 | MED | 1 | WiFi | wifi_wardrive writes 0.0 GPS coords when never fixed |
| POS-AUDIT-209 | MED | 2 | WiFi | wifi_ciw SSID field 64B→33B |
| POS-AUDIT-210 | MED | 1 | WiFi | wifi_deauth s_seq shared between resume cycles |
| POS-AUDIT-211 | MED | 1 | WiFi | wifi_clients_all s_locked toggle not atomic vs hop_task |
| POS-AUDIT-212 | MED | 1 | WiFi | wifi_portal idle delay(5) tight loop CPU thrash |
| POS-AUDIT-213 | MED | 2 | WiFi | PMF warning on ApClone + Portal entries |
| POS-AUDIT-214 | LOW | 1 | WiFi | wifi_portal one-time esp_netif_init/event_loop gate |
| POS-AUDIT-215 | LOW | 2 | WiFi | wifi_beacon_spam Arduino WiFi.mode(WIFI_STA) → lean |
| POS-AUDIT-216 | LOW | 2 | WiFi | wifi_spectrum Arduino WiFi.mode(WIFI_STA) → lean |
| POS-AUDIT-217 | LOW | 1 | WiFi | wifi_scan dead retry path |
| POS-AUDIT-218 | LOW | 1 | WiFi | wifi_deauth_extras C5 5G frames inflate s_b_sent |
| POS-AUDIT-219 | LOW | 1 | WiFi | All-features promisc cleanup audit (8 sites) |
| POS-AUDIT-220 | MED | 1 | BLE | ble_flood ESC mid-conn may leak conn |
| POS-AUDIT-221 | MED | 1 | BLE | ble_clone mac[5]\|=0xC0 breaks public-MAC targets |
| POS-AUDIT-222 | MED | 0 | BLE | ble_db duplicate/colliding OUI rows |
| POS-AUDIT-223 | MED | 0 | BLE | sigdb_bt duplicate manufacturer IDs |
| POS-AUDIT-224 | MED | 2 | BLE | ble_db O(n) OUI lookup → binary search |
| POS-AUDIT-225 | MED | 1 | BLE | ble_db_identify Samsung/MS shortcuts return without verification |
| POS-AUDIT-226 | MED | 1 | BLE | ble_whisperpair child-from-target skips scan-stop |
| POS-AUDIT-227 | MED | 1 | BLE | ble_whisperpair 3s busy wait without WDT reset |
| POS-AUDIT-228 | MED | 1 | BLE | ble_hid/blueducky GATT services leak on repeat-entry |
| POS-AUDIT-229 | LOW | 1 | BLE | ble_spam enum-as-int sentinel pattern |
| POS-AUDIT-230 | LOW | 1 | BLE | ble_findmy volatile read of stable value per tick |
| POS-AUDIT-231 | LOW | 0 | BLE | ble_extras vs ble_scan MAC byte order inconsistent |
| POS-AUDIT-232 | LOW | 1 | BLE | ble_karma connectable advertising w/o GATT server |
| POS-AUDIT-233 | LOW | 0 | BLE | ble_db sentinel-terminated table fragile |
| POS-AUDIT-234 | LOW | 2 | BLE | Refactor ble_whisperpair + ble_blueducky splits |
| POS-AUDIT-235 | LOW | 2 | BLE | Refactor ble_random_addr helper |
| POS-AUDIT-236 | LOW | 2 | BLE | Refactor ble_hid_common shared HID |
| POS-AUDIT-237 | LOW | 2 | BLE | Refactor split ble_extras into tracker/sniff/ibeacon |
| POS-AUDIT-238 | LOW | 3 | BLE | ble_findmy SD-key load (Evil-M5 parity) |
| POS-AUDIT-239 | MED | 1 | SubGHz | subghz_jammer cap leaves chip armed |
| POS-AUDIT-240 | MED | 1 | SubGHz | subghz_jammer/bruteforce WDT feed in TX loops |
| POS-AUDIT-241 | MED | 2 | SubGHz | ui_rssi_scope shared global state across features |
| POS-AUDIT-242 | MED | 1 | SubGHz | nrf24_suite wasteful re-begin after jammer cap |
| POS-AUDIT-243 | MED | 1 | SubGHz | lora_spectrum scope +/- retunes radio w/o debounce |
| POS-AUDIT-244 | MED | 1 | SubGHz | cc1101_park_others always gps_end if already ended |
| POS-AUDIT-245 | MED | 2 | SubGHz | subghz_scan interrupt attach/detach scattered, RAII wrapper |
| POS-AUDIT-246 | MED | 2 | SubGHz | subghz_spectrum 28.8KB BSS waterfall ring lazy-alloc |
| POS-AUDIT-247 | LOW | 1 | SubGHz | subghz_broadcast ON-AIR badge pre-flash 100ms |
| POS-AUDIT-248 | LOW | 1 | SubGHz | nrf52 features no Feather FW version check |
| POS-AUDIT-249 | LOW | 1 | SubGHz | nrf52_ble_mitm_relay sd_is_mounted gate |
| POS-AUDIT-250 | LOW | 1 | SubGHz | nrf24_suite BLE access addr byte-reverse field validation |
| POS-AUDIT-251 | LOW | 1 | SubGHz | nrf52_suite Arduino String thrash in BLE_SCAN parser |
| POS-AUDIT-252 | LOW | 1 | SubGHz | cc1101_begin RxBW hardcoded; add preset parameter |
| POS-AUDIT-253 | LOW | 2 | SubGHz | subghz_scan bit-bang replay → cc1101_rmt_tx |
| POS-AUDIT-254 | LOW | 0 | SubGHz | lora_spectrum waterfall row-color shift document |
| POS-AUDIT-255 | LOW | 1 | SubGHz | cc1101_park_others defensive SX1262 reset pulse |
| POS-AUDIT-256 | LOW | 3 | SubGHz | radio_lora.cpp gated SD logging |
| POS-AUDIT-257 | LOW | 2 | SubGHz | Refactor split nrf24_suite into 5 features + commons |
| POS-AUDIT-258 | LOW | 2 | SubGHz | Refactor nrf52 ensure_feather consolidate |
| POS-AUDIT-259 | LOW | 2 | SubGHz | Refactor sub_file_io shared util |
| POS-AUDIT-260 | LOW | 2 | SubGHz | Refactor cc1101_begin(freq,rxbw_khz) preset overload |
| POS-AUDIT-261 | LOW | 2 | SubGHz | Refactor hspi_park_all_but centralised parking |
| POS-AUDIT-262 | LOW | 2 | SubGHz | Refactor nrf52 PING heartbeat for crash recovery |
| POS-AUDIT-263 | MED | 1 | Net | c5_cmd unconditional set_channel(1) ignores active features |
| POS-AUDIT-264 | MED | 1 | Net | c5_cmd.h vs proto.h type name drift CMD_HS / HS_CAPTURE |
| POS-AUDIT-265 | MED | 1 | Net | dhcp_cache header claims ISR-safe but dhcp_learn not atomic |
| POS-AUDIT-266 | MED | 0 | Net | net_dhcp feat_net_hijack inline copy of rogue_dhcp_loop |
| POS-AUDIT-267 | MED | 2 | Net | net_wpad build_type2 Type-3 readback duplicated |
| POS-AUDIT-268 | MED | 1 | Net | Meshtastic hardcoded LongFast PSK + channel hash |
| POS-AUDIT-269 | MED | 1 | Net | net_responder/wpad credential file unbounded append |
| POS-AUDIT-270 | MED | 1 | Net | net_wpad Serial.println leaks NTLMv2 to USB-CDC |
| POS-AUDIT-271 | MED | 1 | Net | net_lanrecon FILE_WRITE truncates lan.csv each run |
| POS-AUDIT-272 | MED | 1 | Net | net_responder SD I/O blocks AsyncUDP handler |
| POS-AUDIT-273 | MED | 1 | Net | c5_cmd Serial.printf hot-path logging |
| POS-AUDIT-274 | LOW | 1 | Net | c5_cmd c5_stas/probes/hits/spectrum_get return 0 forever |
| POS-AUDIT-275 | LOW | 1 | Net | satcom max_age_sec parameter ignored |
| POS-AUDIT-276 | LOW | 1 | Net | net_attacks upFile leaks on UPLOAD_FILE_START error |
| POS-AUDIT-277 | LOW | 1 | Net | net_attacks SSDP poisoner cap 200 vs Evil-M5 300 |
| POS-AUDIT-278 | LOW | 1 | Net | net_dhcp ROGUE_POOL_SZ silent ceiling no UI warning |
| POS-AUDIT-279 | LOW | 1 | Net | net_ssdp duplicate header lines across appends |
| POS-AUDIT-280 | LOW | 2 | Net | meshtastic RX task polls every 20ms; IRQ-driven |
| POS-AUDIT-281 | LOW | 1 | Net | c5_cmd c5_peer_name unsafe variant deprecate |
| POS-AUDIT-282 | LOW | 2 | Net | net_lanrecon banner+http use net_helpers wrappers |
| POS-AUDIT-283 | MED | 1 | System | screensaver_check_idle races feature animation loops |
| POS-AUDIT-284 | MED | 1 | System | sd_helper/tools two parallel SD-wipe implementations |
| POS-AUDIT-285 | MED | 0 | System | hat_manager detect() stub returns NONE always |
| POS-AUDIT-286 | MED | 1 | System | deauth_autotest cruft in production main |
| POS-AUDIT-287 | MED | 1 | System | serial_test unauth R\n reset + always-on 4KB task |
| POS-AUDIT-288 | MED | 1 | System | menu.cpp double C5 peer-walk per repaint |
| POS-AUDIT-289 | MED | 2 | System | Five parallel NVS lazy-read patterns → prefs_helper |
| POS-AUDIT-290 | LOW | 1 | System | radio.cpp unconditional delay(100) after teardown |
| POS-AUDIT-291 | LOW | 1 | System | ui_clear_body strobe-suppression drops legit clears |
| POS-AUDIT-292 | LOW | 0 | System | splash.cpp map says trident; code is Hokusai |
| POS-AUDIT-293 | LOW | 1 | System | argus no retry after RAM alloc fail |
| POS-AUDIT-294 | LOW | 1 | System | menu_carousel letter mnemonic missing IR park |
| POS-AUDIT-295 | LOW | 1 | System/c5_node | c5_node app_main no error gating on per-band proto |
| POS-AUDIT-296 | LOW | 1 | System/c5_node | c5_node CMDs 18-26 stubbed; gate UI or impl |
| POS-AUDIT-297 | LOW | 0 | System | version.h inline without static |
| POS-AUDIT-298 | HIGH | 1 | IR | ir_clone Samsung Power cmd 0x40 stale (should be 0x02) |
| POS-AUDIT-299 | LOW | 1 | IR | ir_tvbgone xTaskCreate no pdFAIL handling |
| POS-AUDIT-300 | LOW | 0 | IR | Refactor ir_extras_data split into 4 files |
| POS-AUDIT-301 | MED | 1 | GPS | gps_poll cap byte-drain ≤256 per call |
| POS-AUDIT-302 | LOW | 1 | GPS | gps_end explicit pinMode INPUT (vs CC1101 CS) |
| POS-AUDIT-303 | LOW | 1 | GPS | gps_poll overflow surfaced in s_diag |
| POS-AUDIT-304 | LOW | 1 | Specials | badusb kbuf[16] bump to 64 + surface truncation |
| POS-AUDIT-305 | LOW | 1 | Specials | badusb type_string multibyte-aware iteration |
| POS-AUDIT-306 | LOW | 1 | Specials | badusb implement real COMBO keyword |
| POS-AUDIT-307 | LOW | 4 | Specials | badusb optional VID/PID/manufacturer spoof (OPSEC) |
| POS-AUDIT-308 | MED | 1 | Specials | drone_remoteid altitude offset spec verification |
| POS-AUDIT-309 | MED | 1 | Specials | surveillance_hunter set rx_cb BEFORE promisc enable |
| POS-AUDIT-310 | LOW | 3 | Specials | surveillance BLE side (Raven UUID 0x09C8) |
| POS-AUDIT-311 | LOW | 0 | Specials | surveillance JSONL null lat/lon when no GPS |
| POS-AUDIT-312 | LOW | 1 | Specials | defensive_monitor zero-MAC BCAST misses random deauthers |
| POS-AUDIT-313 | MED | 1 | Specials | triton_learn_save truncates brain.bin on SD-fault |
| POS-AUDIT-314 | LOW | 1 | Specials | triton bump BS_N 24→64 |
| POS-AUDIT-315 | HIGH | 1 | Specials | trident readRect+Serial.write 135×/frame batch scanlines |
| POS-AUDIT-316 | MED | 1 | Specials | trident check g_mimir_cdc_active before claim |
| POS-AUDIT-317 | MED | 1 | Specials | trident loot streaming Arduino String per-byte |
| POS-AUDIT-318 | LOW | 1 | Specials | input_inject ring overflow bump + drop count report |
| POS-AUDIT-319 | MED | 1 | Specials | mimir check g_trident_cdc_active before claim |
| POS-AUDIT-320 | LOW | 1 | Specials | mimir JSON parser guard against nested-escape |
| POS-AUDIT-321 | MED | 1 | SaltyJack | saltyjack_wpad nt_len cap at 1024 (mirror responder) |

## Tickets

### POS-AUDIT-001 — [CRIT][System/IR] src/main.cpp:63-67 + 7 other files — IR LED parked LOW (= ON) at every park site

- **Severity:** CRIT
- **Module:** System / IR
- **Phase:** 1
- **File:** src/main.cpp:63-67,80-81,138-139 + src/menu.cpp:1323,1357 + src/menu_carousel.cpp:377,391-415 + src/features/ir_remote.cpp:110,127,149,196 + src/features/ir_tvbgone.cpp:122,186 + src/features/ir_clone.cpp:59,67-69,220,341
- **Problem:** Code writes `digitalWrite(44, LOW)` at boot, watchdog tick, menu transitions, and feature exits. Per `feedback_poseidon_ir_led_active_low.md` and confirmed by `ir_remote.cpp:97-110` (carrier_on writes LOW = ON), wiring is anode→3V3, cathode→GPIO 44 — LOW = ON. Comments throughout claim "park HIGH" but code writes LOW.
- **Fix recipe:** Replace every `digitalWrite(44, LOW)` outside an active mark/space with `digitalWrite(44, HIGH)`. Sites: main.cpp boot + watchdog (4), menu.cpp (2), menu_carousel.cpp (2 incl letter mnemonic path), ir_remote.cpp (4), ir_tvbgone.cpp (2), ir_clone.cpp (4). Verify with phone-camera at boot.
- **Effort:** S
- **Depends on:** none
- **Verification:** Phone-camera test — IR LED dark at boot until user enters an IR feature; LED off between bit-pairs and on exit
- **Release gate:** yes
- **Source findings:** sys-001, ir-001 (merged)

### POS-AUDIT-002 — [CRIT][SubGHz] src/features/subghz_jam_detect.cpp:78,114,194,202-205 — Exit paths skip radio_switch(RADIO_NONE)

- **Severity:** CRIT
- **Module:** SubGHz
- **Phase:** 1
- **File:** src/features/subghz_jam_detect.cpp:78,114,194,202-205
- **Problem:** All early-return paths (cc1101_begin fail, user-ESC during warmup, user-ESC during monitor) return without calling `radio_switch(RADIO_NONE)`. `s_active` stays `RADIO_SUBGHZ`. GPS UART stays dead (enter-side `gps_end()`), pin 13 driven HIGH. Next radio user thinks "nothing to tear down".
- **Fix recipe:** Add `radio_switch(RADIO_NONE)` on every exit path; also fix `pick_freq()` ESC return at line 73. Verify GPS re-arms on subsequent feature entry.
- **Effort:** S
- **Depends on:** none
- **Verification:** Enter jam_detect → fail / ESC → enter GPS-using feature, verify GPS fix returns; on-device sweep
- **Release gate:** yes
- **Source findings:** rf-001, rf-002 (merged)

### POS-AUDIT-003 — [CRIT][WiFi] src/features/wifi_ciw.cpp:238-296 — Arduino WiFi.softAP + per-rotate re-attach + APSTA teardown

- **Severity:** CRIT
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_ciw.cpp:238-239,265,295-296
- **Problem:** Lone surviving user of `WiFi.mode(WIFI_MODE_AP)` + `WiFi.softAP()` — documented to crash `ieee80211_hostap_attach +0x2c` under pinned Bruce libs. Re-calls `WiFi.softAP()` per rotation (in-loop AP thrash → `ESP_ERR_NO_MEM 257` cascade). Teardown ends in `WiFi.mode(APSTA)` which doubles WiFi buffers and corrupts next radio user.
- **Fix recipe:** Lift `wifi_raw_ap_up()` helper from `wifi_portal.cpp:417-481`; mutate SSID via `esp_wifi_set_config(WIFI_IF_AP, ...)` only OR rotate via raw beacon TX only; teardown via `esp_wifi_stop(); esp_wifi_deinit();`.
- **Effort:** M
- **Depends on:** POS-AUDIT-030 (-zmuldefs in build flags)
- **Verification:** Cold-boot test (CIW first feature), then back-to-back with other AP features; no rc=257; no hostap_attach crash
- **Release gate:** yes
- **Source findings:** wifi-001, wifi-002, wifi-003 (merged)

### POS-AUDIT-004 — [CRIT][System] src/menu_registry.cpp + src/menu_registry.h — Dead code

- **Severity:** CRIT
- **Module:** System
- **Phase:** 0
- **File:** src/menu_registry.cpp (201 LOC) + src/menu_registry.h (80 LOC)
- **Problem:** `MenuRegistry::add`, `::add_submenu`, `::build`, `::root`, `REGISTER_FEATURE` macro all have zero references repo-wide. Map's "dynamic self-reg" claim is false. ~280 LOC + 110 KB BSS reservation (32×704 + 128×688) never executes.
- **Fix recipe:** Decision required — either delete both files and remove all references / build entries; OR migrate static `MENU_*` tree in menu.cpp to actually use the registry via `REGISTER_FEATURE` macros at file scope. Delete is faster; activation is the long-term refactor target.
- **Effort:** M (delete) or L (activate)
- **Depends on:** none
- **Verification:** Build clean; binary size drops by ~110 KB BSS reservation; menu navigation regression sweep
- **Release gate:** no (Phase 0)
- **Source findings:** sys-002

### POS-AUDIT-005 — [CRIT][System/c5_node] c5_node/main/zb_sniffer.c:28-56 — ESP-NOW send + led_fx_set from ISR

- **Severity:** CRIT
- **Module:** System / c5_node
- **Phase:** 1
- **File:** c5_node/main/zb_sniffer.c:28-56
- **Problem:** `esp_ieee802154_receive_done` fires in ISR context. Handler builds 240 B `posei_msg_t` on stack, calls `proto_send_to` → `esp_now_send` (NOT IRAM-safe — may cache-miss crash during concurrent flash op), and `led_fx_set` (volatile store ok but wakes a non-ISR-safe task). Random reboots during high Zigbee traffic / coordinator beacon storms.
- **Fix recipe:** Queue frame summary into a FreeRTOS queue from ISR (`xQueueSendFromISR`); drain in a normal-priority task that calls `esp_now_send` and `led_fx_set`.
- **Effort:** M
- **Depends on:** none
- **Verification:** Zigbee coordinator stress test; no reboots over 30 min
- **Release gate:** yes
- **Source findings:** sys-003

### POS-AUDIT-006 — [HIGH][BLE] src/features/ble_scan.cpp:349-358 — feat_ble_scan bypasses radio_switch(RADIO_BLE)

- **Severity:** HIGH
- **Module:** BLE
- **Phase:** 1
- **File:** src/features/ble_scan.cpp:349-358
- **Problem:** Calls `NimBLEDevice::init("")` directly with stale debug comment about "isolating the hang". If prior domain was WiFi (typical — LAN attacks, scan, wardrive), WiFi is still up; BLE Scan runs alongside an active WiFi driver, skipping the PORKCHOP init-order coex recipe.
- **Fix recipe:** Replace direct `NimBLEDevice::init("")` with `radio_switch(RADIO_BLE)`; delete the debug comment block at line 371-390 (and the now-redundant `wifi_mode = ?` query).
- **Effort:** S
- **Depends on:** POS-AUDIT-008 (radio.cpp teardown discipline)
- **Verification:** Enter WiFi feature → ESC → enter BLE Scan; verify no heap-pressure regression or hang
- **Release gate:** yes
- **Source findings:** ble-001

### POS-AUDIT-007 — [HIGH][WiFi] src/features/wifi_portal.cpp:430 + 2 sites — Unconditional BTDM release one-way

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_portal.cpp:430 + src/features/evil_twin.cpp:401 + src/features/ap_signal_test.cpp:62
- **Problem:** `esp_bt_controller_mem_release(ESP_BT_MODE_BTDM)` called every entry. One-way until power cycle. Triton already removed this; portal family did not. User has no way to recover BLE without unplug.
- **Fix recipe:** Check `esp_bt_controller_get_status()` first; only release if BLE was never inited this session; emit UI toast warning "BLE disabled until reboot" OR gate behind explicit prompt.
- **Effort:** M
- **Depends on:** none
- **Verification:** Enter BLE feature → exit → enter portal → verify BLE still works after portal exit (if BTDM wasn't released); regression test for portal RAM headroom
- **Release gate:** yes
- **Source findings:** wifi-004, wifi-005, wifi-006 (merged)

### POS-AUDIT-008 — [HIGH][System] src/radio.cpp:108-111 — WiFi.mode(WIFI_OFF) violates PORKCHOP

- **Severity:** HIGH
- **Module:** System
- **Phase:** 1
- **File:** src/radio.cpp:108-111
- **Problem:** Every WiFi feature exit funnels through `radio_switch()` which calls `WiFi.mode(WIFI_OFF)`. PORKCHOP `wifi_utils.cpp:227-229` explicitly bans this — triggers `esp_wifi_deinit/init 257` errors on fragmented heap. Eventually deadlocks WiFi driver into init-257 state; subsequent re-init returns ESP_ERR.
- **Fix recipe:** Replace `WiFi.mode(WIFI_OFF)` with `esp_wifi_stop()` leaving driver inited; gate `esp_wifi_deinit` on a heap-healthy check (largest free > 35 KB threshold à la PORKCHOP `kMinHeapForTls`).
- **Effort:** M
- **Depends on:** POS-AUDIT-118 (heap-policy thresholds)
- **Verification:** Long session through 10+ WiFi features w/o restart; no rc=257; heap watermark stable
- **Release gate:** yes
- **Source findings:** sys-009

### POS-AUDIT-009 — [HIGH][WiFi] src/features/wifi_portal.cpp:551 + evil_twin.cpp:337 — Blocking overlay halts DNS+HTTP

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_portal.cpp:539-540,551 + src/features/evil_twin.cpp:329-330,337
- **Problem:** `s_dns->processNextRequest()` + `s_http->handleClient()` called synchronously inside UI loop with `delay(5)` between iterations. During the 1.2 s blocking `ui_action_overlay`, DNS+HTTP service halts entirely — captive-portal probe phones can time out and abort the redirect.
- **Fix recipe:** Spawn dedicated `xTaskCreatePinnedToCore` for DNS+HTTP service, OR run the overlay as a non-blocking per-frame state machine that yields between paint and service ticks.
- **Effort:** M
- **Depends on:** none
- **Verification:** Connect Android phone to portal; verify no redirect timeout during overlay; iOS test same
- **Release gate:** yes
- **Source findings:** wifi-007, wifi-008 (merged)

### POS-AUDIT-010 — [HIGH][Net/SaltyJack] 5 sites — Arduino WiFi.softAP() in non-CIW sites

- **Severity:** HIGH
- **Module:** Net / SaltyJack
- **Phase:** 1
- **File:** src/features/saltyjack/saltyjack_dhcp_rogue.cpp:287 + src/features/net_attacks.cpp:558 + src/features/net_wpad.cpp:279,512 + src/features/net_dhcp.cpp:327
- **Problem:** Five additional sites use forbidden Arduino softAP path despite map tagging them [WiFi-AP-IDF good]. Map is wrong — actual code calls Arduino. Same instability surface as POS-AUDIT-003; intermittent AP-mode rogue-DHCP failure suspected to trace back here.
- **Fix recipe:** Migrate all 5 sites to raw-IDF AP recipe from `wifi_portal.cpp:417-481`. After POS-AUDIT-216 (wifi_ap_helpers extraction in Phase 2), call the shared helper.
- **Effort:** M
- **Depends on:** POS-AUDIT-003 pattern proven
- **Verification:** Per-site cold-boot AP up sweep; capture rogue-DHCP success rate before/after on busy network
- **Release gate:** yes
- **Source findings:** net-002, slt-001 (merged)

### POS-AUDIT-011 — [HIGH][BLE] 7 sites — Indefinite scans without setMaxResults(0)

- **Severity:** HIGH
- **Module:** BLE
- **Phase:** 1
- **File:** src/features/ble_whisperpair.cpp:626-636 + src/features/ble_extras.cpp:88,211 + src/features/ble_karma.cpp:77 + src/features/ble_toys.cpp:233 + src/features/ble_finder.cpp:323,423
- **Problem:** Indefinite-duration scans (`scan->start(0, false)`) start without capping internal `m_scanResults` vector. NimBLE keeps growing the vector for every advertisement received. After 5+ minutes idle in busy RF environments, heap exhausts.
- **Fix recipe:** Add `scan->setMaxResults(0)` after `setScanCallbacks` at every indefinite-duration scan site.
- **Effort:** S
- **Depends on:** none
- **Verification:** Idle whisperpair/karma/finder for 10 min in busy mall RF environment; verify heap stable
- **Release gate:** yes
- **Source findings:** ble-003

### POS-AUDIT-012 — [HIGH][SubGHz] src/cc1101_hw.cpp:91 — cc1101_end doesn't restore GDO0/CS pins

- **Severity:** HIGH
- **Module:** SubGHz
- **Phase:** 1
- **File:** src/cc1101_hw.cpp:91
- **Problem:** `cc1101_end()` calls `setSidle` + `goSleep` but does not reset GDO0 to INPUT or de-park CS lines 12/13/6/5. Sub-GHz → LoRa transition could see CC1101 CS still driven HIGH and GDO0 left OUTPUT from previous TX path.
- **Fix recipe:** Add `pinMode(CC1101_GDO0, INPUT); pinMode(CC1101_CS, INPUT)` before return in `cc1101_end()`.
- **Effort:** S
- **Depends on:** none
- **Verification:** Sub-GHz scan → LoRa scan transitions; verify no MISO contention
- **Release gate:** yes
- **Source findings:** rf-004

### POS-AUDIT-013 — [HIGH][SubGHz] src/features/subghz_record.cpp:175 — 20s RX exceeds TWDT

- **Severity:** HIGH
- **Module:** SubGHz
- **Phase:** 1
- **File:** src/features/subghz_record.cpp:175 + src/cc1101_rmt.cpp (rx path)
- **Problem:** `cc1101_rmt_rx` invoked with `timeout_ms=20000` (20 s) exceeds FreeRTOS TWDT (5 s default in IDF 5.5). RX blocks on `ulTaskNotifyTake` for full 20 s if no signal arrives. No `yield()`/`esp_task_wdt_reset()` inside `cc1101_rmt_rx`. Idle-task watchdog panic.
- **Fix recipe:** Slice the wait into 1 s slices with `esp_task_wdt_reset()` between, OR `esp_task_wdt_delete()` for current task during RX window then re-add on exit.
- **Effort:** M
- **Depends on:** none
- **Verification:** Enter record on dead frequency; verify 20 s elapses without TWDT panic
- **Release gate:** yes
- **Source findings:** rf-008

### POS-AUDIT-014 — [HIGH→STALE][IR/specials] src/features/drone_remoteid.cpp:108,197 — SD I/O inside portENTER_CRITICAL

- **Status:** STALE — finding does not match current source
- **Severity:** HIGH (originally)
- **Module:** IR / specials
- **Phase:** 1
- **File:** src/features/drone_remoteid.cpp:108,197 + s_mux at :67
- **Problem (claimed):** `decode_astm()` holds `portENTER_CRITICAL(&s_mux)` and calls `log_event()` which does `s_log.printf` + `s_log.flush()` — FATFS calls under a portMUX.
- **Verification (POS-AUDIT-018 follow-up):** `portEXIT_CRITICAL(&s_mux)` is at line 195; `log_event(dr, msg_type)` is at line 197. SD I/O is NOT inside the critical section. The `dr` reference into `s_drones[idx]` is read after EXIT, but the only writer is `decode_astm` itself (running single-threaded on the NimBLE host task), so no race exists. Source finding `drn-001` mis-characterised line 197's position relative to line 195.
- **Fix recipe (not needed):** —
- **Effort:** 0
- **Depends on:** none
- **Verification:** Visual inspection of lines 149-197 confirms log_event is post-EXIT
- **Release gate:** no (closed-stale)
- **Source findings:** drn-001 (audit description error)

### POS-AUDIT-015 — [HIGH][IR/specials] src/features/defensive_monitor.cpp:608 + :201-228 — NimBLE churn + alert MPSC race

- **Severity:** HIGH
- **Module:** IR / specials
- **Phase:** 1
- **File:** src/features/defensive_monitor.cpp:201-228,601-635
- **Problem:** (a) `enter_wifi_phase`/`enter_ble_phase` cycle NimBLE init/deinit every 5 s (~30 KB heap churn per cycle); long sessions silently fragment heap until init fails. (b) `enqueue_alert` is called from BOTH WiFi-ISR-class promisc_cb AND NimBLE-task `ble_on_adv` — promisc_cb wraps in critical (line 355), `ble_on_adv` does NOT; MPSC ring corruption under concurrent enqueue.
- **Fix recipe:** (a) Widen phase windows to ≥30 s, OR switch to coexist pattern (NimBLE passive scan alongside promisc with phase-spike avoidance). (b) Add `portENTER_CRITICAL` around `ble_on_adv`'s `enqueue_alert` call.
- **Effort:** M
- **Depends on:** POS-AUDIT-118 (heap-policy gates)
- **Verification:** 30-min defensive monitor session in busy RF env; verify heap stable, no missed alerts under BLE+WiFi bursts
- **Release gate:** yes
- **Source findings:** dfn-001, dfn-002 (merged)

### POS-AUDIT-016 — [HIGH][IR/specials] src/features/triton.cpp (1457 LOC) — Freeze regression hypothesis cluster

- **Severity:** HIGH
- **Module:** IR / specials
- **Phase:** 1
- **File:** src/features/triton.cpp:131,188-198,583-588,680,1068,1250
- **Problem:** Multiple stacked risk patterns to iterate per `feedback_careful_iteration.md`: (a) lockless float RMW on `s_q[]` across cores (`:583-588`, `:680`) → NaN risk wedges softmax; (b) long `portENTER_CRITICAL` in WiFi RX cb holding 1024-byte strncpy (`:188-198`) → RX queue overflow under STORM; (c) Argus `pushImage` during deauth burst (`:1250`) = canonical TX-cache scramble per `feedback_poseidon_tx_layout_sensitivity.md`; (d) `s_file` FILE_APPEND + lazy `s_wdr_file` open = 2 concurrent FATFS handles + hashcat keeps growing.
- **Fix recipe:** Iterate per hypothesis (ONE fix per HW round per `feedback_careful_iteration.md` and `feedback_autonomous_hardware_debug.md`): (a) portMUX around `s_q[]` access; (b) trim CRITICAL window to just the enqueue, do strncpy outside; (c) suppress Argus draw during deauth bursts in STORM mode; (d) consolidate to single FATFS handle (close hashcat between bursts). DO NOT bundle.
- **Effort:** M to validate each, S to apply each
- **Depends on:** none (already cooperative-tick per memory)
- **Verification:** STORM-mode soak test 60 min after each fix; record freeze MTBF; per `project_triton_freeze_debug.md` checkpoint progress
- **Release gate:** yes
- **Source findings:** tri-002

### POS-AUDIT-017 — [HIGH][System] src/sfx.cpp:38-44 + src/input.cpp:71-90 — NVS handle leak + SFX blocking input loop

- **Severity:** HIGH
- **Module:** System
- **Phase:** 1
- **File:** src/sfx.cpp:38-44 + src/input.cpp:71-90
- **Problem:** `sfx_init` does `s_prefs.begin("sfx", false)` and never `.end()` — handle leak across process lifetime; uncommitted writes lost on crash. Separately, `sfx_click` (~14 ms) / `sfx_select` (~45 ms) / `sfx_back` (~39 ms) blocking calls fire from `input_poll_raw` on EVERY keypress. ENTER→feature latency adds 45 ms; rapid letter-mnemonic nav throttled to ~70 keys/sec.
- **Fix recipe:** (a) Scope NVS per call (`Preferences p; p.begin("sfx"); p.put...; p.end();`) like every other consumer. (b) Dispatch SFX into cooperative scheduler tick in `loop()`, OR `xTaskCreate` a tiny `sfx_player` task that consumes a queue of tone events.
- **Effort:** M
- **Depends on:** none
- **Verification:** Rapid mnemonic nav through carousel; no perceptible ENTER latency; NVS-write reliability after power cycle
- **Release gate:** yes
- **Source findings:** sys-004, sys-005 (merged)

### POS-AUDIT-018 — [HIGH][System/c5_node] c5_node/main/wifi_attacker.c:38,79-80,160-161 — Shared s_frame[26] between two tasks

- **Severity:** HIGH
- **Module:** System / c5_node
- **Phase:** 1
- **File:** c5_node/main/wifi_attacker.c:38-45,79-80,160-161
- **Problem:** Single static `s_frame[26]` mutated by both `deauth_targeted_task` and `deauth_bcast_task`. Currently serialised by one-CMD-at-a-time dispatch in `main.c:on_recv`, but back-to-back CMDs mean the earlier task may still loop over the buffer when the next mutates it.
- **Fix recipe:** Move `s_frame` to stack-local in each task; pass pointer.
- **Effort:** S
- **Depends on:** none
- **Verification:** Back-to-back targeted+broadcast deauth; verify no MAC corruption mid-burst
- **Release gate:** yes
- **Source findings:** sys-010

### POS-AUDIT-019 — [HIGH][WiFi] src/features/wifi_pmkid.cpp:447-449 — Blocking notification while RX task fires

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_pmkid.cpp:447-449
- **Problem:** `draw_notification` calls `M5Cardputer.Speaker.tone()` + `delay(130)` + tone + `delay(130)` (~600 ms total) while WiFi RX task continues firing `promisc_cb` → `emit_handshake` → blocking SD writes. UI main loop blocks; RX task queues EAPOL into 8-slot `dynamic_rx_buf` pool which can overflow before UI returns.
- **Fix recipe:** Non-blocking tone sequence (state machine in main loop schedules next tone via timer), or move notification to a dedicated low-prio task.
- **Effort:** M
- **Depends on:** POS-AUDIT-017 (general SFX async pattern)
- **Verification:** PMKID capture in busy environment during notification; verify no EAPOL drops
- **Release gate:** yes
- **Source findings:** wifi-009

### POS-AUDIT-020 — [HIGH][Net] src/mesh.cpp:139 — WiFi.getMode() fails after raw-IDF init

- **Severity:** HIGH
- **Module:** Net
- **Phase:** 1
- **File:** src/mesh.cpp:139
- **Problem:** Arduino `WiFi.getMode()` lies when raw-IDF init is in use (C5 has the documented fix at `c5_cmd.cpp:248-253`). `mesh_begin` from a path that just ran a raw-IDF feature (Triton, portal, deauth) hits `WiFi.mode(WIFI_STA)` which double-creates the netif and asserts in `esp_netif_create_default_wifi_sta`.
- **Fix recipe:** Copy `c5_cmd.cpp:248-253` `esp_wifi_get_mode` probe pattern.
- **Effort:** S
- **Depends on:** none
- **Verification:** Triton → ESC → enter mesh status; no netif assert
- **Release gate:** yes
- **Source findings:** net-001

### POS-AUDIT-021 — [HIGH][WiFi] src/features/wifi_scan.cpp:531 — Rescan xTaskCreate 8KB stack canary risk

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_scan.cpp:531
- **Problem:** Rescan xTaskCreate uses 8192B stack. `WiFi.scanNetworks` runs inline + lib's callback dispatch ~2-3 KB. Stack pressure on rescan is real; the `cached` rescan path may or may not actually use this task.
- **Fix recipe:** Verify via runtime canary check; preferably collapse rescan to inline scan logic from `:357-479` (which author notes at `:354-356` solved a canary overflow).
- **Effort:** M
- **Depends on:** none
- **Verification:** Rescan stress test; runtime canary check
- **Release gate:** yes
- **Source findings:** wifi-010

### POS-AUDIT-022 — [HIGH][WiFi] src/features/wifi_ciw.cpp:222 — 10KB CiwPayload BSS competes with WiFi driver

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_ciw.cpp:222-228
- **Problem:** `static CiwPayload active[PAYLOAD_COUNT]` in BSS at 64+1 bytes × ~157 = ~10KB. Large fixed BSS competes with WiFi driver allocations after `esp_wifi_init`. Combined with AP-softAP path (POS-AUDIT-003), this is a memory pressure point.
- **Fix recipe:** Allocate into heap on entry, free on exit (`heap_caps_malloc(MALLOC_CAP_INTERNAL)`); or shrink ssid field to 33 bytes (max valid SSID + nul) — saves ~5KB.
- **Effort:** S
- **Depends on:** POS-AUDIT-003
- **Verification:** Cold-boot CIW entry; heap watermark vs prior
- **Release gate:** yes
- **Source findings:** wifi-011

### POS-AUDIT-023 — [HIGH][BLE] src/features/ble_hid.cpp:186-187 + ble_blueducky.cpp:407-408 — Double-deinit esp_wifi

- **Severity:** HIGH
- **Module:** BLE
- **Phase:** 1
- **File:** src/features/ble_hid.cpp:186-187 + src/features/ble_blueducky.cpp:407-408
- **Problem:** Both call `radio_switch(RADIO_NONE)` first then directly `esp_wifi_stop()/esp_wifi_deinit()`. If prior domain was already RADIO_NONE, esp_wifi was already torn down; second deinit returns `ESP_ERR_WIFI_NOT_INIT` silently. The deinit-already-deinited pattern is PORKCHOP's documented `init 257` trigger.
- **Fix recipe:** Gate on `esp_wifi_get_mode(&m) == ESP_OK` before stop/deinit, OR push the logic into a `radio_force_wifi_off()` helper in `radio.cpp`.
- **Effort:** S
- **Depends on:** POS-AUDIT-008
- **Verification:** Cold-boot HID feature; verify no rc=257 after first use
- **Release gate:** yes
- **Source findings:** ble-002

### POS-AUDIT-024 — [HIGH][SubGHz] src/features/nrf24_suite.cpp:32-42,186-187,359 — Raw SPI register writes bypass RF24 lib shadow

- **Severity:** HIGH
- **Module:** SubGHz
- **Phase:** 1
- **File:** src/features/nrf24_suite.cpp:32-42,186-187,359
- **Problem:** `nrf_write_reg()` does raw SPI register writes on `sd_get_spi()` (v0.6.1 Hydra fix). Call sites at 186-187 (`EN_RXADDR=0x3F`, `SETUP_AW=0`) and 359 (`SETUP_AW=0x03`) happen AFTER `nrf24_begin()` which already configured the radio through RF24 lib. Lib caches CONFIG/EN_AA shadow; raw writes diverge it.
- **Fix recipe:** Prefer `rf.setAddressWidth(2)` / `rf.setAddressWidth(5)` (RF24 lib supports it — already used at `:462`); eliminate raw `nrf_write_reg(0x03, ...)`.
- **Effort:** S
- **Depends on:** none
- **Verification:** Sniffer + mousejack sweep; verify SETUP_AW correctness post-call
- **Release gate:** yes
- **Source findings:** rf-006

### POS-AUDIT-025 — [HIGH][SubGHz] src/features/subghz_replay.cpp:67 — Picker misses /poseidon/signals/custom/ subdir

- **Severity:** HIGH
- **Module:** SubGHz
- **Phase:** 1
- **File:** src/features/subghz_replay.cpp:67
- **Problem:** `pick_sub_file` opens `/poseidon` and only iterates root-level `.sub` files. Real recordings live under `/poseidon/signals/custom/` (where `subghz_scan` and `subghz_record` write). Feature has been silently empty for users.
- **Fix recipe:** Descend into `/poseidon/signals/` subdirs, OR pivot to `subghz_broadcast.cpp` category picker (reuse).
- **Effort:** S
- **Depends on:** none
- **Verification:** Record a signal → enter replay → file appears in picker
- **Release gate:** yes
- **Source findings:** rf-007

### POS-AUDIT-026 — [HIGH][SubGHz] src/lora_hw.cpp:124 — lora_begin doesn't park CC1101/nRF24 CS

- **Severity:** HIGH
- **Module:** SubGHz
- **Phase:** 1
- **File:** src/lora_hw.cpp:118-124
- **Problem:** `lora_begin` parks SD CS=12 HIGH but does NOT park CC1101 CS=13 or nRF24 CS=6. Comment says "CC1101 never coexists with LoRa" — true at s_active level, but if a feature exited via early-return without `radio_switch(RADIO_NONE)` (POS-AUDIT-002), CC1101 CS could be left driven such that LoRa sees noise.
- **Fix recipe:** Defensive — park CS=13 (after `gps_end` check — pin shared with GPS TX) and CS=6 HIGH before LoRa NSS asserts.
- **Effort:** S
- **Depends on:** POS-AUDIT-002 + POS-AUDIT-012
- **Verification:** Stress-test sub-GHz → LoRa transitions; verify no MISO contention
- **Release gate:** yes
- **Source findings:** rf-005

### POS-AUDIT-027 — [HIGH][Net] src/satcom.cpp:153 — 8s blocking http.GET freezes UI

- **Severity:** HIGH
- **Module:** Net
- **Phase:** 1
- **File:** src/satcom.cpp:153 + src/features/feat_satcom.cpp:131
- **Problem:** `http.setTimeout(8000)` then blocking GET. Whole 8 s blocks UI; ESC ignored; toast "refreshing TLE..." shown for 800 ms then UI freezes.
- **Fix recipe:** Replace HTTPClient with chunked async loop polling `input_poll` between socket reads; cancel on ESC.
- **Effort:** M
- **Depends on:** none
- **Verification:** Trigger TLE refresh on weak network; ESC interrupts within 200 ms
- **Release gate:** yes
- **Source findings:** net-004

### POS-AUDIT-028 — Merged into POS-AUDIT-001

POS-AUDIT-001 covers `sys-001` + `ir-001` IR-polarity dedupe.

### POS-AUDIT-029 — Merged into POS-AUDIT-015

POS-AUDIT-015 covers `dfn-001` + `dfn-002` defensive_monitor cluster.

### POS-AUDIT-030 — [HIGH][WiFi] platformio.ini — Verify -Wl,-zmuldefs in build_flags

- **Severity:** HIGH
- **Module:** WiFi (build)
- **Phase:** 0
- **File:** platformio.ini
- **Problem:** Per memory invariant + cross-ref with PORKCHOP (`platformio.ini:34`), `-Wl,-zmuldefs` is required to override `ieee80211_raw_frame_sanity_check`. Audit needs to confirm.
- **Fix recipe:** Grep `build_flags` in `platformio.ini`; confirm or add `-Wl,-zmuldefs`.
- **Effort:** S
- **Depends on:** none
- **Verification:** Build emits no multiple-definition warnings; deauth still TXes raw frames
- **Release gate:** no (Phase 0)
- **Source findings:** wifi-035

### POS-AUDIT-031 — [HIGH][WiFi] src/features/wifi_deauth_frame.h:191-197 — Drop STA→AP reverse-pair in broadcast deauth

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 1
- **File:** src/features/wifi_deauth_frame.h:191-197
- **Problem:** `wifi_deauth_pair` does 4× `esp_wifi_80211_tx` + `vTaskDelay(2ms)` per pair (8 ms airtime). Bruce/Porkchop typically do 1 frame per pair. For broadcast deauth on busy AP, STA→AP reverse direction wastes bandwidth.
- **Fix recipe:** Add `bool include_reverse` parameter to `wifi_deauth_pair`, default false for broadcast; halves airtime, doubles effective deauth-per-second.
- **Effort:** S
- **Depends on:** none
- **Verification:** Pre/post deauth-per-second measurement vs Wireshark capture
- **Release gate:** yes
- **Source findings:** wifi-040

### POS-AUDIT-032 — Merged into POS-AUDIT-118

PORKCHOP heap-policy adoption tracked under POS-AUDIT-118.

### POS-AUDIT-101 — [HIGH][BLE] Port Bruce HID Exploit Engine 9-tactic

- **Severity:** HIGH
- **Module:** BLE
- **Phase:** 3
- **File:** src/features/ble_hid.cpp extension
- **Problem:** Bruce's `HIDExploitEngine::executeHIDInjection` (`BLE_Suite.cpp:462-825`) implements 9 OS-specific tactics; POSEIDON has single tactic + 6 disguise names. Significant capability gap.
- **Fix recipe:** Port 9 tactic dispatch + per-OS payload sequences; integrate with POSEIDON's existing HID descriptor + ascii_to_hid (shared with ble_blueducky via POS-AUDIT-236).
- **Effort:** L
- **Depends on:** POS-AUDIT-008, POS-AUDIT-236
- **Verification:** Per-OS test matrix (Win/Mac/iOS/Android/Linux/ChromeOS); each tactic enumerated
- **Release gate:** yes
- **Source findings:** CROSS_REF gaps row 1

### POS-AUDIT-102 — [HIGH][BLE] Port Bruce WhisperPair multi-variant exploits

- **Severity:** HIGH
- **Module:** BLE
- **Phase:** 3
- **File:** src/features/ble_whisperpair_*.cpp extensions
- **Problem:** Bruce ships `sendProtocolAttack`, `sendStateConfusionAttack`, `sendCryptoOverflowAttack` (`BLE_Suite.cpp:1004,1045,1074`). POSEIDON has only standard ECDH+AES path.
- **Fix recipe:** Extend probe at `ble_whisperpair.cpp:425` with alternative payload sequences; per-variant verdict logging.
- **Effort:** L
- **Depends on:** POS-AUDIT-234 (whisperpair split)
- **Verification:** Replay against patched/unpatched FastPair devices; per-variant verdicts logged
- **Release gate:** yes
- **Source findings:** CROSS_REF gaps row 2

### POS-AUDIT-103 — [MED][Net] Port Ghost DIAL/Chromecast/YouTube-Lounge hijack

- **Severity:** MED
- **Module:** Net
- **Phase:** 3
- **File:** Future src/features/net_dial.cpp
- **Problem:** Ghost `dial_manager.c` + `commandline.c:236` hits `youtube.com/api/lounge/bc/bind` to cast videos to LAN TVs. POSEIDON-absent.
- **Fix recipe:** Port DIAL discovery + YouTube Lounge bind protocol; HTTP client + LAN SSDP reuse from `net_ssdp.cpp`.
- **Effort:** L
- **Depends on:** none
- **Verification:** Cast to a Chromecast on test LAN
- **Release gate:** yes
- **Source findings:** CROSS_REF gaps row 3

### POS-AUDIT-104 — [MED][Net] Port Ghost TP-Link Kasa control

- **Severity:** MED
- **Module:** Net
- **Phase:** 3
- **File:** Future src/features/net_kasa.cpp
- **Problem:** Ghost ships XOR scheme + on/off toggle (`commandline.c:518-534`). POSEIDON-absent.
- **Fix recipe:** Port XOR encrypt/decrypt + TCP/9999 broadcast probe + state toggle UI.
- **Effort:** M
- **Depends on:** none
- **Verification:** Toggle Kasa plug on test LAN
- **Release gate:** yes
- **Source findings:** CROSS_REF gaps row 4

### POS-AUDIT-105 — [LOW][System] Bruce mquickjs JS interpreter (deferred to v0.8)

- **Severity:** LOW
- **Module:** System
- **Phase:** 3 (deferred)
- **File:** Future src/js_runtime/*
- **Problem:** Bruce ships mquickjs with WiFi/BLE/BadUSB bindings. Big-effort capability gap.
- **Fix recipe:** Out-of-scope for v0.7; evaluate mquickjs@0.0.6; estimate binding surface; defer to v0.8.
- **Effort:** L (deferred)
- **Depends on:** none
- **Verification:** n/a (deferred)
- **Release gate:** no
- **Source findings:** CROSS_REF gaps row 5

### POS-AUDIT-106 — [LOW][Net] Port Bruce Wireguard/SSH/SOCKS4/Telnet

- **Severity:** LOW
- **Module:** Net
- **Phase:** 3
- **File:** Future src/features/net_ssh.cpp / net_wg.cpp / net_socks.cpp
- **Problem:** Bruce ships full client suite. POSEIDON-absent.
- **Fix recipe:** Start SSH client; Telnet client; Wireguard L; SOCKS4 S.
- **Effort:** L (composite)
- **Depends on:** none
- **Verification:** Live SSH session from device
- **Release gate:** yes
- **Source findings:** CROSS_REF gaps row 6

### POS-AUDIT-107 — [LOW][Specials] Bruce RFID/NFC stack (hardware eval first)

- **Severity:** LOW
- **Module:** Specials
- **Phase:** 3
- **File:** Future src/features/nfc/*
- **Problem:** Bruce ships PN532, ChameleonUltra, Amiibo, EMV, SRIX, RFID125, MFRC522. Requires NFC hat hardware.
- **Fix recipe:** Hardware support eval first; defer scope decision.
- **Effort:** L (deferred)
- **Depends on:** hardware eval
- **Verification:** n/a
- **Release gate:** no
- **Source findings:** CROSS_REF gaps row 7

### POS-AUDIT-108 — [LOW][Net] Bruce W5500 wired LAN attacks (defer to SIREN)

- **Severity:** LOW
- **Module:** Net
- **Phase:** 3
- **File:** Future SIREN satellite firmware
- **Problem:** Bruce ships ARP scan/spoof/poison, DHCPStarvation, MACFlooding on W5500. POSEIDON SaltyJack is WiFi-side only.
- **Fix recipe:** SIREN W5500 satellite in roadmap (`project_siren`) — reuse SaltyJack attack logic when hardware ships.
- **Effort:** L (deferred, hat-dependent)
- **Depends on:** SIREN hardware
- **Verification:** Wired ARP scan via SIREN
- **Release gate:** no
- **Source findings:** CROSS_REF gaps row 8

### POS-AUDIT-109 — [HIGH][Net/SaltyJack] Evil-M5 Network Hijack combo wrapper

- **Severity:** HIGH
- **Module:** Net / SaltyJack
- **Phase:** 3
- **File:** Future src/features/saltyjack/saltyjack_hijack.cpp
- **Problem:** Evil-M5 case 50 orchestrates DHCP+DNS+WPAD. SaltyJack has pieces but no fire-all wrapper.
- **Fix recipe:** New feature: rogue DHCP → captive DNS → WPAD/Autodiscover in coordinated sequence; UI shows chain state.
- **Effort:** M
- **Depends on:** POS-AUDIT-010, POS-AUDIT-111
- **Verification:** Full chain on target VM; credential capture verified
- **Release gate:** yes
- **Source findings:** slt-002

### POS-AUDIT-110 — [MED][Net/SaltyJack] Evil-M5 DHCPAttackAuto

- **Severity:** MED
- **Module:** Net / SaltyJack
- **Phase:** 3
- **File:** src/features/net_dhcp.cpp extension
- **Problem:** Evil-M5 case 51 detects real DHCP server, auto-picks starve vs rogue.
- **Fix recipe:** Extend selector with `detectDHCPServer` probe; dispatch.
- **Effort:** M
- **Depends on:** none
- **Verification:** Networks with/without DHCP; correct attack picked
- **Release gate:** yes
- **Source findings:** slt-003

### POS-AUDIT-111 — [MED][Net] Evil-M5 Switch DNS AP↔STA bind toggle

- **Severity:** MED
- **Module:** Net
- **Phase:** 3
- **File:** src/features/net_responder.cpp extension OR new src/features/net_captive_dns.cpp
- **Problem:** Evil-M5 case 49 toggles captive DNS between AP and STA bind.
- **Fix recipe:** Add per-name spoof map alongside wildcard; toggle bind interface; UI toast for current state.
- **Effort:** M
- **Depends on:** none
- **Verification:** Captive DNS hijack from STA works; toggle changes behavior
- **Release gate:** yes
- **Source findings:** slt-004

### POS-AUDIT-112 — [MED][Net] Evil-M5 SSDP fake-300 poisoner

- **Severity:** MED
- **Module:** Net
- **Phase:** 3
- **File:** src/features/net_attacks.cpp (extend feat_ssdp_poison)
- **Problem:** Evil-M5 case 71 spawns 300 fake UPnP devices. POSEIDON caps at 200 (PSRAM-broken).
- **Fix recipe:** PSRAM broken on this hardware; stay at 200, document the cap delta; OR investigate internal SRAM extension.
- **Effort:** S (document) / L (extend)
- **Depends on:** none
- **Verification:** SSDP M-SEARCH from LAN client; count fake replies
- **Release gate:** yes
- **Source findings:** slt-005

### POS-AUDIT-113 — [LOW][Net] Evil-M5 UPnP NAT abuse

- **Severity:** LOW
- **Module:** Net
- **Phase:** 3
- **File:** Future src/features/net_upnp_nat.cpp
- **Problem:** Evil-M5 cases 77/78 enumerate IGD mappings + craft NAT-add requests.
- **Fix recipe:** Reuse `net_ssdp.cpp` discovery; HTTP POST UPnP SOAP for GetGenericPortMappingEntry + AddPortMapping.
- **Effort:** L
- **Depends on:** none
- **Verification:** Open NAT port on test router; verify via external probe
- **Release gate:** yes
- **Source findings:** slt-006

### POS-AUDIT-114 — [HIGH][WiFi] Evil-M5 captive cookie siphon endpoint

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_portal.cpp extension
- **Problem:** Evil-M5 `/siphon` + `/logcookie` (`:3895`) — JS-side payload posts captured cookies back. POSEIDON portal has no equivalent.
- **Fix recipe:** Add `/siphon` POST endpoint to portal HTTP server + writes to `/poseidon/cookies.log`; ship JS template snippet.
- **Effort:** M
- **Depends on:** POS-AUDIT-009
- **Verification:** Test JS payload in portal; cookies captured to SD
- **Release gate:** yes
- **Source findings:** net-026

### POS-AUDIT-115 — [MED][Net/SaltyJack] Evil-M5 NTLM hash de-dup pass

- **Severity:** MED
- **Module:** Net / SaltyJack
- **Phase:** 3
- **File:** src/features/saltyjack/saltyjack_ntlm_crack.cpp extension OR new src/ntlm_helpers.cpp
- **Problem:** Evil-M5 case 62 removes duplicate hash lines. SaltyJack appends only.
- **Fix recipe:** Add `ntlm_dedup_file()` helper; call on entry to crack feature; reusable across saltyjack + net_responder + net_wpad outputs.
- **Effort:** S
- **Depends on:** none
- **Verification:** Append duplicates, run dedup, output unique
- **Release gate:** yes
- **Source findings:** net-027, slt-007

### POS-AUDIT-116 — [HIGH][WiFi] Evil-M5 portal SD template loader (31+27)

- **Severity:** HIGH
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_portal.cpp extension
- **Problem:** Evil-M5 ships 31 base + 27 community templates from `/evil/sites/`; POSEIDON has 16 inline only.
- **Fix recipe:** SD enumeration in `pick_template` for `/poseidon/portal/*.html`; file streaming in `handle_root`. Keep inline fallbacks. Optional: `portal_ip_sel` rewrite like Evil-M5.
- **Effort:** L
- **Depends on:** none
- **Verification:** Drop new HTML to SD; verify in picker; serves correctly
- **Release gate:** yes
- **Source findings:** wifi-030

### POS-AUDIT-117 — [MED][System] Evil-M5 boot-to-feature (kiosk drop-box)

- **Severity:** MED
- **Module:** System
- **Phase:** 3
- **File:** src/main.cpp + new prefs key
- **Problem:** Evil-M5 `caseToStartAtBoot` + countdown auto-launches a feature. Useful for kiosk / drop-box.
- **Fix recipe:** NVS `boot_feature` (index or name) + countdown UI in splash; ENTER cancels; System Settings "Boot to" menu.
- **Effort:** M
- **Depends on:** POS-AUDIT-289
- **Verification:** Set boot_feature, reboot, auto-launch + countdown UX
- **Release gate:** yes
- **Source findings:** sys-032

### POS-AUDIT-118 — [HIGH][System] PORKCHOP heap-pressure state machine

- **Severity:** HIGH
- **Module:** System
- **Phase:** 3
- **File:** Future src/core/heap_policy.{cpp,h} + heap_health.{cpp,h} + heap_gates.{cpp,h}
- **Problem:** PORKCHOP has 47 named thresholds + 4-level state machine + EMA-smoothed display. POSEIDON has none.
- **Fix recipe:** Port the three header trio; integrate with ui status badge (EMA percent), gate Argus + ble_spam on pressure level; thresholds adapted for POSEIDON heap.
- **Effort:** L
- **Depends on:** none
- **Verification:** Long session + diagnostics shows pressure transitions; Argus auto-disabled under Critical
- **Release gate:** yes
- **Source findings:** sys-028

### POS-AUDIT-119 — [HIGH][System] PORKCHOP 80KB Reservation Fence pre-WiFi init

- **Severity:** HIGH
- **Module:** System
- **Phase:** 3
- **File:** src/radio.cpp::wifi_lean_sta_init prologue
- **Problem:** PORKCHOP allocates 80 KB fence pre-`WiFi.mode(STA)` to force WiFi driver buffers above fence. POSEIDON has no equivalent.
- **Fix recipe:** Add `heap_caps_malloc(80*1024, MALLOC_CAP_INTERNAL); free();` before `esp_wifi_init` in `wifi_lean_sta_init`. Measure layout before/after.
- **Effort:** M
- **Depends on:** none
- **Verification:** Cold-boot heap watermark; fragmented-heap repro after long session; no rc=257
- **Release gate:** yes
- **Source findings:** sys-029, tri-004

### POS-AUDIT-120 — [MED][System] PORKCHOP watermark persistence to SD

- **Severity:** MED
- **Module:** System
- **Phase:** 3
- **File:** Future src/core/heap_health.cpp
- **Problem:** PORKCHOP `persistWatermarks()` persists min-free + min-largest across reboots, rate-limited 60 s.
- **Fix recipe:** Write to `/poseidon/heapwatermark.log` every 60 s + on shutdown.
- **Effort:** M
- **Depends on:** POS-AUDIT-118
- **Verification:** Reboot after long session; log shows correct watermarks
- **Release gate:** yes
- **Source findings:** sys-030

### POS-AUDIT-121 — [MED][System] PORKCHOP canGrow(minFree,minFrag) gate

- **Severity:** MED
- **Module:** System
- **Phase:** 3
- **File:** Future src/core/heap_gates.cpp
- **Problem:** PORKCHOP fragmentation-aware push_back guard. POSEIDON features push_back blindly.
- **Fix recipe:** Helper `bool can_grow_vec(int min_free, float min_frag)`; instrument `g_wdr_aps`, triton deauth list, wifi_clients_all, surveillance dedup.
- **Effort:** M
- **Depends on:** POS-AUDIT-118
- **Verification:** Stress wardrive dense env; capped at threshold not OOM
- **Release gate:** yes
- **Source findings:** sys-028 (heap_gates)

### POS-AUDIT-122 — [MED][WiFi] PORKCHOP BOAR BROS wardrive exclusion list

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_wardrive.cpp extension + SD persistence
- **Problem:** PORKCHOP 50-entry exclusion keeps wardrive clean of operator gear.
- **Fix recipe:** 50-entry BSSID exclusion, persist to `/poseidon/wardrive_exclude.txt`, UI to add/remove from wardrive view; check on each AP add.
- **Effort:** M
- **Depends on:** none
- **Verification:** Add own AP to exclude; not in CSV
- **Release gate:** yes
- **Source findings:** wifi-038

### POS-AUDIT-123 — [LOW][WiFi] PORKCHOP adaptive 4-state DNH hopper

- **Severity:** LOW
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/triton.cpp extension OR new src/features/wifi_donoham.cpp
- **Problem:** PORKCHOP 4-state HOPPING/DWELLING/HUNTING/IDLE_SWEEP w/ per-channel dead-streak. POSEIDON Triton has RL but no per-channel state.
- **Fix recipe:** Port FSM into triton's channel-pick; layer alongside RL softmax.
- **Effort:** L
- **Depends on:** POS-AUDIT-016
- **Verification:** Handshake rate dense env vs current
- **Release gate:** yes
- **Source findings:** wifi cross-ref

### POS-AUDIT-124 — [LOW][BLE] PORKCHOP NimBLE internal-only allocation cflag

- **Severity:** LOW
- **Module:** BLE (build)
- **Phase:** 3
- **File:** platformio.ini
- **Problem:** PORKCHOP `-DCONFIG_BT_NIMBLE_MEM_ALLOC_MODE_INTERNAL=1`. POSEIDON has no flag — PSRAM broken so works by accident.
- **Fix recipe:** Add flag to `platformio.ini build_flags`; verify BLE inits.
- **Effort:** S
- **Depends on:** none
- **Verification:** BLE feature sweep post-flag
- **Release gate:** yes
- **Source findings:** ble-023

### POS-AUDIT-125 — [MED][System] PORKCHOP crash viewer (SD-backed)

- **Severity:** MED
- **Module:** System
- **Phase:** 3
- **File:** Future src/features/system_crash_viewer.cpp
- **Problem:** PORKCHOP `ui/crash_viewer.cpp` reads SD crash files. POSEIDON has only autotest NVS marker.
- **Fix recipe:** Register `esp_register_shutdown_handler`; write panic reason/PC/task to `/poseidon/crash/N.log`; cap 32; new System submenu to browse.
- **Effort:** M
- **Depends on:** none
- **Verification:** Force panic; log written; viewer reads
- **Release gate:** yes
- **Source findings:** sys cross-ref

### POS-AUDIT-126 — [MED][System] PORKCHOP first-class diagnostics menu

- **Severity:** MED
- **Module:** System
- **Phase:** 3
- **File:** Future src/features/system_diagnostics.cpp
- **Problem:** PORKCHOP `ui/diagnostics_menu.cpp` heap/PSRAM/WiFi state/Knuth. POSEIDON per-feature ad-hoc.
- **Fix recipe:** New System submenu: heap free/largest/EMA, PSRAM size, WiFi mode, C5 peer count, hat status (after POS-AUDIT-285), Knuth ratio (gated to this view).
- **Effort:** M
- **Depends on:** POS-AUDIT-118
- **Verification:** Enter diag; live values; no perf regression elsewhere
- **Release gate:** yes
- **Source findings:** sys-031

### POS-AUDIT-127 — [MED][WiFi] PORKCHOP/Bruce hidden-SSID reveal via broadcast deauth

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_spectrum.cpp extension (reveal hotkey)
- **Problem:** PORKCHOP `spectrum.cpp` reveal mode sends broadcast deauths to flush probe-response carrying hidden SSID. POSEIDON-absent.
- **Fix recipe:** Hotkey in spectrum view; brief broadcast deauth burst + listen probe responses; surface SSIDs.
- **Effort:** M
- **Depends on:** none
- **Verification:** Test on hidden SSID; reveal works
- **Release gate:** yes
- **Source findings:** wifi cross-ref

### POS-AUDIT-128 — [MED][WiFi] Ghost 802.11ax HE Capabilities IE in beacons

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_beacon_spam.cpp:146 extension
- **Problem:** Ghost injects 802.11ax HE Capabilities IE — phones show Wi-Fi 6 badge. POSEIDON ships legacy only.
- **Fix recipe:** Append IE 0xFF Extended Capabilities + 0x23 HE Cap to `build_beacon`.
- **Effort:** M
- **Depends on:** none
- **Verification:** Phone shows Wi-Fi 6 on spammed SSID
- **Release gate:** yes
- **Source findings:** wifi-036

### POS-AUDIT-129 — [MED][WiFi] Ghost AP-list beacon spam mode

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_beacon_spam.cpp:146 extension
- **Problem:** Ghost `-l` re-broadcasts scanned AP SSIDs. POSEIDON-absent.
- **Fix recipe:** New mode; pull from `g_wdr_aps[]` and rotate.
- **Effort:** M
- **Depends on:** none
- **Verification:** Run scan, beacon spam AP-list; capture confirms
- **Release gate:** yes
- **Source findings:** wifi-037

### POS-AUDIT-130 — [LOW][Net] Ghost Power Printer mass-print

- **Severity:** LOW
- **Module:** Net
- **Phase:** 3
- **File:** src/features/net_attacks.cpp (extend feat_printer)
- **Problem:** Ghost mass-sends print jobs. POSEIDON has single-job version.
- **Fix recipe:** Extend `feat_printer` with mass mode + repeat count.
- **Effort:** S
- **Depends on:** none
- **Verification:** Test against IPP printer
- **Release gate:** yes
- **Source findings:** CROSS_REF gaps row 30

### POS-AUDIT-131 — [MED][IR/specials] Ghost PineAP rogue-AP detection

- **Severity:** MED
- **Module:** IR / specials
- **Phase:** 3
- **File:** src/features/defensive_monitor.cpp extension
- **Problem:** Ghost PineAP heuristic vs known BSSIDs. POSEIDON-absent.
- **Fix recipe:** Add detector class to defensive_monitor: track BSSID-vs-SSID consistency, flag PineAP-style rebroadcast.
- **Effort:** M
- **Depends on:** POS-AUDIT-015
- **Verification:** Test with WiFi Pineapple replay
- **Release gate:** yes
- **Source findings:** wifi/sys cross-ref

### POS-AUDIT-132 — [MED][IR/specials] Ghost Pwnagotchi presence detection

- **Severity:** MED
- **Module:** IR / specials
- **Phase:** 3
- **File:** src/features/wifi_scan.cpp or defensive_monitor.cpp extension
- **Problem:** Ghost `is_pwn_response` detects pwnagotchi grid frames. POSEIDON-absent.
- **Fix recipe:** Add Pwnagotchi grid signature match in promisc cb; surface in scan or defensive monitor.
- **Effort:** M
- **Depends on:** none
- **Verification:** Test with live pwnagotchi
- **Release gate:** yes
- **Source findings:** specials cross-ref

### POS-AUDIT-133 — [MED][IR/specials] Ghost BLE skimmer detection

- **Severity:** MED
- **Module:** IR / specials
- **Phase:** 3
- **File:** src/features/defensive_monitor.cpp BLE side extension
- **Problem:** Ghost skimmer detection tuned for card-skimmer adverts. POSEIDON-absent.
- **Fix recipe:** BLE detector class with known skimmer manufacturer IDs + GATT UUIDs (cross-ref sigdb_bt.h).
- **Effort:** M
- **Depends on:** POS-AUDIT-015
- **Verification:** Test with known skimmer signatures
- **Release gate:** yes
- **Source findings:** specials cross-ref

### POS-AUDIT-134 — [MED][BLE] Ghost BLE wardrive (WiGLE CSV + GPS)

- **Severity:** MED
- **Module:** BLE
- **Phase:** 3
- **File:** Future src/features/ble_wardrive.cpp
- **Problem:** Ghost ships BLE wardrive CSV with GPS. POSEIDON wardrives WiFi only.
- **Fix recipe:** NimBLE passive scan + GPS-gated CSV in WiGLE format.
- **Effort:** M
- **Depends on:** none
- **Verification:** Run BLE wardrive dense env; CSV parseable by WiGLE
- **Release gate:** yes
- **Source findings:** BLE cross-ref

### POS-AUDIT-135 — [LOW][BLE] Ghost BLE pcap writer

- **Severity:** LOW
- **Module:** BLE
- **Phase:** 3
- **File:** Future src/pcap_writer.cpp + ble_extras integration
- **Problem:** Ghost ships BT pcap. POSEIDON writes CSV blesniff only.
- **Fix recipe:** Shared pcap writer (also for POS-AUDIT-137); BLE writes link-type 256 BT-HCI-H4-with-PHDR.
- **Effort:** M
- **Depends on:** none
- **Verification:** Capture; Wireshark
- **Release gate:** yes
- **Source findings:** BLE cross-ref

### POS-AUDIT-136 — [LOW][System] Ghost REST + SPA control surface

- **Severity:** LOW
- **Module:** System
- **Phase:** 3
- **File:** Future src/features/system_remote.cpp
- **Problem:** Ghost serves SPA + REST. POSEIDON TRIDENT is USB-CDC. No browser access.
- **Fix recipe:** New System → Remote submenu; minimal HTTPD on STA-associated IP; reuse SaltyJack auth pattern. Large effort.
- **Effort:** L
- **Depends on:** POS-AUDIT-008
- **Verification:** Browser GET / POST against ESP
- **Release gate:** yes
- **Source findings:** sys-033

### POS-AUDIT-137 — [MED][WiFi] Ghost EAPOL→PCAP alongside hashcat 22000

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_pmkid.cpp + new src/pcap_writer.cpp
- **Problem:** Ghost writes raw EAPOL pcap; POSEIDON only hashcat 22000. Both useful.
- **Fix recipe:** Per-frame PCAP write in `promisc_cb` alongside emit_handshake.
- **Effort:** M
- **Depends on:** POS-AUDIT-135
- **Verification:** Open pcap; EAPOL frames intact
- **Release gate:** yes
- **Source findings:** wifi-039

### POS-AUDIT-138 — [MED][SubGHz] Bruce nRF24 drone FHSS jammer profile

- **Severity:** MED
- **Module:** SubGHz
- **Phase:** 3
- **File:** src/features/nrf24_suite.cpp:658 extension
- **Problem:** Bruce drone-specific FHSS hop. POSEIDON has 7 generic presets, no drone.
- **Fix recipe:** Add "Drone FHSS" preset with Bruce's hop sequence (DJI / FrSky / FlySky).
- **Effort:** S
- **Depends on:** none
- **Verification:** Hop sequence on SDR
- **Release gate:** yes
- **Source findings:** rf-031

### POS-AUDIT-139 — [LOW][IR] Bruce IR jammer

- **Severity:** LOW
- **Module:** IR
- **Phase:** 3
- **File:** Future src/features/ir_jammer.cpp
- **Problem:** Bruce IR jammer floods noise. POSEIDON-absent.
- **Fix recipe:** Continuous 38 kHz carrier with random data bursts (post-POS-AUDIT-001 polarity).
- **Effort:** S
- **Depends on:** POS-AUDIT-001
- **Verification:** IR receiver locks out during jam
- **Release gate:** yes
- **Source findings:** specials cross-ref

### POS-AUDIT-140 — [HIGH][IR] Bruce IR clone with RX capture (already TODOd)

- **Severity:** HIGH
- **Module:** IR
- **Phase:** 3
- **File:** src/features/ir_clone.cpp:11 (already TODO'd in code)
- **Problem:** Bruce raw IR capture + save. POSEIDON has 3 profile pickers but NO RX.
- **Fix recipe:** IR RX via GPIO 9 (Cardputer-Adv IR RX); decode NEC/Samsung/Sony/RC5/RC6; persist `.ir` to SD; reuse send path.
- **Effort:** M
- **Depends on:** POS-AUDIT-001
- **Verification:** Capture from real remote; replay; target responds
- **Release gate:** yes
- **Source findings:** ir cross-ref

### POS-AUDIT-141 — [LOW][SubGHz] Bruce sub-GHz listen (pulse-decode audio)

- **Severity:** LOW
- **Module:** SubGHz
- **Phase:** 3
- **File:** Future src/features/subghz_listen.cpp
- **Problem:** Bruce decodes pulses to audio passthrough.
- **Fix recipe:** Reuse CC1101 RMT RX; convert pulses to tone via speaker.
- **Effort:** M
- **Depends on:** none
- **Verification:** Tune to active source; audio playback
- **Release gate:** yes
- **Source findings:** rf-030

### POS-AUDIT-142 — [HIGH][Net/SaltyJack] mDNS poisoner reply path

- **Severity:** HIGH
- **Module:** Net / SaltyJack
- **Phase:** 3
- **File:** src/features/net_responder.cpp:194 extension
- **Problem:** ABSENT in both POSEIDON + Evil-M5 — macOS/iOS unaffected. POSEIDON already listens 5353 multicast; needs reply logic.
- **Fix recipe:** A/AAAA query handler — reply with attacker IP for any name (mirror LLMNR pattern).
- **Effort:** M
- **Depends on:** none
- **Verification:** macOS `dns-sd -B _http._tcp.` etc; reply
- **Release gate:** yes
- **Source findings:** slt-011

### POS-AUDIT-143 — [LOW][BLE] Bruce Ninebot scooter unlock

- **Severity:** LOW
- **Module:** BLE
- **Phase:** 3
- **File:** Future src/features/ble_ninebot.cpp
- **Problem:** Bruce ships Xiaomi/Ninebot scooter unlock. POSEIDON-absent.
- **Fix recipe:** Port BLE GATT handshake + key derivation.
- **Effort:** M
- **Depends on:** none
- **Verification:** Test on Ninebot ES-series
- **Release gate:** yes
- **Source findings:** BLE cross-ref

### POS-AUDIT-144 — [LOW][Specials] Evil-M5 Skyjack Parrot drone hunter

- **Severity:** LOW
- **Module:** Specials
- **Phase:** 3
- **File:** src/features/drone_remoteid.cpp extension OR new drone_skyjack.cpp
- **Problem:** Evil-M5 case 72 hunts Parrot AR.Drone SSIDs. POSEIDON has passive RemoteID only.
- **Fix recipe:** New feature: scan WiFi for Parrot SSID patterns, optional deauth + clone.
- **Effort:** S
- **Depends on:** none
- **Verification:** Lab Parrot drone
- **Release gate:** yes
- **Source findings:** slt-013

### POS-AUDIT-145 — [INFO][Specials] Evil-M5 IMSI catcher (deferred, no LTE)

- **Severity:** INFO (deferred)
- **Module:** Specials
- **Phase:** 4
- **File:** n/a
- **Problem:** Requires LTE radio not present on Cardputer-Adv.
- **Fix recipe:** Out-of-scope; deferred indefinitely.
- **Effort:** n/a
- **Depends on:** LTE hat
- **Verification:** n/a
- **Release gate:** no
- **Source findings:** slt-014

### POS-AUDIT-146 — [LOW][Net] Evil-M5 LDAP anonymous bind enum

- **Severity:** LOW
- **Module:** Net
- **Phase:** 3
- **File:** Future src/features/net_ldap.cpp
- **Problem:** Evil-M5 LDAPDump anonymous bind + enum against target DC.
- **Fix recipe:** Port LDAP bind + search; output to SD CSV.
- **Effort:** M
- **Depends on:** none
- **Verification:** Test AD
- **Release gate:** yes
- **Source findings:** net cross-ref

### POS-AUDIT-147 — [LOW][System] Evil-M5 web admin dashboard (overlaps POS-AUDIT-136)

- **Severity:** LOW
- **Module:** System
- **Phase:** 3
- **File:** Folds into POS-AUDIT-136
- **Problem:** Evil-M5 exposes captured creds via HTTP dashboard (default pwd `7h30th3r0n3`). POSEIDON on-device only.
- **Fix recipe:** Merge with POS-AUDIT-136 REST surface — add credential review page.
- **Effort:** see POS-AUDIT-136
- **Depends on:** POS-AUDIT-136
- **Verification:** see POS-AUDIT-136
- **Release gate:** yes
- **Source findings:** slt-012

### POS-AUDIT-148 — [MED][WiFi] Bruce WiGLE.net upload

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_wardrive.cpp extension
- **Problem:** Bruce POSTs wardrive CSVs to WiGLE. POSEIDON-absent.
- **Fix recipe:** System Settings entry for WiGLE API key (NVS); upload button in wardrive; HTTPS POST.
- **Effort:** M
- **Depends on:** none
- **Verification:** Upload test CSV; WiGLE accepts
- **Release gate:** yes
- **Source findings:** wifi cross-ref

### POS-AUDIT-149 — [MED][WiFi] PORKCHOP wpasec upload

- **Severity:** MED
- **Module:** WiFi
- **Phase:** 3
- **File:** src/features/wifi_pmkid.cpp extension
- **Problem:** PORKCHOP POSTs handshakes/PMKID to wpa-sec.stanev.org. POSEIDON-absent.
- **Fix recipe:** Upload button in PMKID; HTTPS POST captured 22000 records.
- **Effort:** M
- **Depends on:** none
- **Verification:** Upload test handshake; accepted
- **Release gate:** yes
- **Source findings:** wifi cross-ref

### POS-AUDIT-150 — [HIGH][Specials] BadUSB SD payload loader (promised, unimplemented)

- **Severity:** HIGH
- **Module:** Specials
- **Phase:** 3
- **File:** src/features/badusb.cpp:240
- **Problem:** Header/comment at `badusb.cpp:6` mentions SD `/poseidon/ducky/*.txt` but `feat_badusb` only reads built-in payloads.
- **Fix recipe:** SD enumerator + payload picker UI; path-traversal sanitiser (reject `..` / absolute); reuse `bd_run_payload` runner.
- **Effort:** M
- **Depends on:** none
- **Verification:** Drop ducky.txt to SD; appears in picker; path-traversal blocked
- **Release gate:** yes
- **Source findings:** bdu-001

### POS-AUDIT-200 — [MED][WiFi] wifi_beacon_spam.cpp:40 — s_beacon race main↔task

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_beacon_spam.cpp:40 · **Effort:** S
- **Problem:** Global non-const `static uint8_t s_beacon[128]` mutated from `spam_task` while user-space `pick_list` UI reads from main task. No sync.
- **Fix:** Allocate frame on task's stack OR `portMUX` mutation OR document benign race.
- **Depends on:** none · **Verification:** static-only · **Release gate:** yes · **Source:** wifi-012

### POS-AUDIT-201 — [MED][WiFi] wifi_deauth_extras silent_ap_begin scope inconsistency

- **Severity:** MED · **Phase:** 2 · **File:** src/features/wifi_deauth_extras.cpp:34 vs wifi_clients_all.cpp:123,138 · **Effort:** S
- **Problem:** `broad_task` calls `wifi_silent_ap_begin(1)` ONCE then iterates with `set_channel`; `wifi_clients_all` calls per-burst. Inconsistent.
- **Fix:** Standardize on begin-once-per-feature (correct given STA-only model).
- **Depends on:** none · **Verification:** sweep both features · **Release gate:** yes · **Source:** wifi-013

### POS-AUDIT-202 — [MED][WiFi] wifi_clients.cpp:90 — Sync deauth burst from UI task

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_clients.cpp:90 · **Effort:** M
- **Problem:** `deauth_client` calls `wifi_silent_ap_begin` + 30 pairs synchronously on UI task. 4 frames × 30 iter × ~8 ms ≈ 1 s UI freeze.
- **Fix:** Spawn brief task for burst OR drop iteration count + spread across cooperative ticks.
- **Depends on:** none · **Verification:** ENTER deauth client; UI responsive · **Release gate:** yes · **Source:** wifi-014

### POS-AUDIT-203 — [MED][WiFi] wifi_probe.cpp:222 — TX from RX cb context

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_probe.cpp:222-240 · **Effort:** M
- **Problem:** `probe_cb` (WiFi RX task ctx) calls `esp_wifi_80211_tx` directly. Serializes RX with TX inside WiFi task; sustained probe traffic could starve RX.
- **Fix:** Queue (sta+ssid) pairs into StreamBuffer; TX from dedicated low-prio task (pattern matches Karma's beacon-spam loop at :419-437).
- **Depends on:** none · **Verification:** Karma in busy env; no RX drops · **Release gate:** yes · **Source:** wifi-015

### POS-AUDIT-204 — [MED][WiFi] wifi_portal/evil_twin — new DNSServer/WebServer every cycle

- **Severity:** MED · **Phase:** 2 · **File:** src/features/wifi_portal.cpp:497-501 + evil_twin.cpp:309-313 · **Effort:** S
- **Problem:** Heap-allocates per portal cycle. Evil-twin churns every 30 s, fragments heap.
- **Fix:** File-scope statics, `begin()`/`stop()` to cycle; OR arena allocator.
- **Depends on:** none · **Verification:** Long evil-twin session; heap stable · **Release gate:** yes · **Source:** wifi-016, wifi-017

### POS-AUDIT-205 — [MED][WiFi] wifi_pmkid.cpp:174 — Cap m2_len to 260 before hex_append

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_pmkid.cpp:174 · **Effort:** S
- **Problem:** `s_emit_handshake_line[600]` BSS; M2 hex blob can theoretically exceed 600 B if M2 EAPOL body > 260 B (RSN KDE multi-AKM).
- **Fix:** `if (m2_len > 260) m2_len = 260;` before line 175.
- **Depends on:** none · **Verification:** Test with multi-AKM AP; no overflow · **Release gate:** yes · **Source:** wifi-018

### POS-AUDIT-206 — [MED][WiFi] wifi_pmkid.cpp:362 — hunt_task reads s_cache[] without snapshot

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_pmkid.cpp:362-381 · **Effort:** S
- **Problem:** `hunt_task` reads `s_cache[i].bssid` while `cache_beacon` may modify same slot. Race.
- **Fix:** Snapshot BSSID list under brief crit-section in hunt_task before burst loop, OR atomic seqlock-style versioning.
- **Depends on:** none · **Verification:** static-only · **Release gate:** yes · **Source:** wifi-019

### POS-AUDIT-207 — [MED][WiFi] wifi_wardrive — first_seen vs current gps_snapshot mix

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_wardrive.cpp:53-67,93,97-98 · **Effort:** S
- **Problem:** Row rewriting on later flush mixes valid-GPS rows with placeholder timestamps; per-AP `first_seen` exists but timestamp written is current snapshot.
- **Fix:** Keep first-seen timestamp + GPS-at-first-fix per AP; don't conflate with current snapshot.
- **Depends on:** none · **Verification:** Wardrive with GPS coming online mid-session; verify rows consistent · **Release gate:** yes · **Source:** wifi-016, wifi-020

### POS-AUDIT-208 — [MED][WiFi] wifi_wardrive.cpp:160 — 0.0 GPS coords written when never fixed

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_wardrive.cpp:160-165 · **Effort:** S
- **Problem:** WiGLE rows can get `0.0,0.0` coords if weak GPS sighting comes after stronger no-GPS sighting. OPSEC invariant violation.
- **Fix:** Gate GPS write on `g.valid` AND only set once when fix is real; emit empty fields for unfixed rows.
- **Depends on:** none · **Verification:** Wardrive without GPS for stretch; CSV has empty (not 0.0) coords · **Release gate:** yes · **Source:** wifi-021

### POS-AUDIT-209 — [MED][WiFi] wifi_ciw struct — SSID field 64B → 33B

- **Severity:** MED · **Phase:** 2 · **File:** src/features/wifi_ciw.cpp:34 · **Effort:** S
- **Problem:** 802.11 max SSID is 32; current 64B → 33B (32 + nul) saves ~5 KB BSS.
- **Fix:** Change field width + update `memcpy_P` use; OR build longer into real 802.11 SSID tag (fuzz vector).
- **Depends on:** POS-AUDIT-022 · **Verification:** Build size delta · **Release gate:** yes · **Source:** wifi-022

### POS-AUDIT-210 — [MED][WiFi] wifi_deauth.cpp:44 — s_seq shared between resume cycles

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_deauth.cpp:38,42,44,304-305 · **Effort:** S
- **Problem:** `s_seq` read+written from both `deauth_task` and `wifi_deauth_pair`. Race window on resume (200 ms gap between old task exit and new task start).
- **Fix:** Atomic seq counter OR pass-by-value into task.
- **Depends on:** none · **Verification:** static-only · **Release gate:** yes · **Source:** wifi-023

### POS-AUDIT-211 — [MED][WiFi] wifi_clients_all.cpp:118 — s_locked not atomic vs hop_task

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_clients_all.cpp:118-131 · **Effort:** S
- **Problem:** `unicast_deauth` toggles `s_locked` to suppress hop, but hop_task checks once per loop iter with 200/400 ms delay; deauth burst can fire across hop boundary.
- **Fix:** Binary semaphore OR join hop_task to state machine.
- **Depends on:** none · **Verification:** Stress-test mass unicast; verify on-channel · **Release gate:** yes · **Source:** wifi-024

### POS-AUDIT-212 — [MED][WiFi] wifi_portal.cpp:572 — Idle delay(5) tight loop CPU thrash

- **Severity:** MED · **Phase:** 1 · **File:** src/features/wifi_portal.cpp:539-540,572 · **Effort:** S
- **Problem:** When no input, loop runs ~200 `processNextRequest`+`handleClient` calls per UI frame. CPU thrash on idle.
- **Fix:** Raise delay to 20 ms when zero STAs connected (check `esp_wifi_ap_get_sta_list`), 5 ms otherwise.
- **Depends on:** none · **Verification:** Idle portal; CPU% vs current · **Release gate:** yes · **Source:** wifi-025

### POS-AUDIT-213 — [MED][WiFi] PMF warning on ApClone + Portal entries

- **Severity:** MED · **Phase:** 2 · **File:** src/features/wifi_deauth_frame.h:238-253 callers · **Effort:** S
- **Problem:** `wifi_auth_has_pmf` is called in deauth + evil_twin but ApClone + Portal don't warn even though PMF makes deauth-driven reassoc impossible.
- **Fix:** Call `wifi_auth_has_pmf` in `feat_wifi_apclone` and `feat_wifi_portal` clone paths.
- **Depends on:** none · **Verification:** Target PMF AP; warning shows · **Release gate:** yes · **Source:** wifi-041

### POS-AUDIT-214 — [LOW][WiFi] wifi_portal.cpp:434 — One-time esp_netif_init/event_loop gate

- **Severity:** LOW · **Phase:** 1 · **File:** wifi_portal.cpp:434-435 + evil_twin.cpp:138-139 + ap_signal_test.cpp:66-67 · **Effort:** S
- **Problem:** `esp_netif_init` + `esp_event_loop_create_default` called every entry. Idempotent but rc ignored.
- **Fix:** Call from boot once; gate with `static bool s_inited`.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** wifi-026

### POS-AUDIT-215 — [LOW][WiFi] wifi_beacon_spam.cpp:149 — Arduino WiFi.mode → lean

- **Severity:** LOW · **Phase:** 2 · **File:** wifi_beacon_spam.cpp:149-150 · **Effort:** S
- **Problem:** Arduino `WiFi.mode(WIFI_STA)` inconsistent with lean IDF init pattern.
- **Fix:** Use `wifi_lean_sta_init()`.
- **Depends on:** none · **Verification:** beacon spam still works · **Release gate:** yes · **Source:** wifi-027

### POS-AUDIT-216 — [LOW][WiFi] wifi_spectrum.cpp:388 — Arduino WiFi.mode → lean

- **Severity:** LOW · **Phase:** 2 · **File:** wifi_spectrum.cpp:388 · **Effort:** S
- **Problem:** Same as POS-AUDIT-215.
- **Fix:** Use `wifi_lean_sta_init()`.
- **Depends on:** none · **Verification:** spectrum works · **Release gate:** yes · **Source:** wifi-028

### POS-AUDIT-217 — [LOW][WiFi] wifi_scan.cpp:67 — Dead STA-mode retry loop

- **Severity:** LOW · **Phase:** 1 · **File:** wifi_scan.cpp:67-79 · **Effort:** S
- **Problem:** Dead retry path in `scan_task` (Rescan only).
- **Fix:** Collapse per POS-AUDIT-021.
- **Depends on:** POS-AUDIT-021 · **Verification:** scan sweep · **Release gate:** no · **Source:** wifi-029

### POS-AUDIT-218 — [LOW][WiFi] wifi_deauth_extras.cpp:55 — C5 5G frames inflate s_b_sent

- **Severity:** LOW · **Phase:** 1 · **File:** wifi_deauth_extras.cpp:55 · **Effort:** S
- **Problem:** `s_b_sent += 1500` estimated per C5 5GHz rotation inflates displayed deauth-per-sec.
- **Fix:** Separate "C5 5G in flight" badge.
- **Depends on:** none · **Verification:** UI shows two counters · **Release gate:** no · **Source:** wifi-031

### POS-AUDIT-219 — [LOW][WiFi] All-features promisc cleanup audit (8 sites)

- **Severity:** LOW · **Phase:** 1 · **File:** wifi_pmkid.cpp:507, wifi_deauth.cpp:243, wifi_wardrive.cpp:217, wifi_spectrum.cpp:395, wifi_clients.cpp:126, wifi_clients_all.cpp:164, wifi_deauth_extras.cpp:300, wifi_pmkid.cpp:554 · **Effort:** M
- **Problem:** Promiscuous cb dangling on exit at 8 sites.
- **Fix:** Add `esp_wifi_set_promiscuous_rx_cb(nullptr)` before promisc-off in every exit path; or extract `wifi_promisc_stop()` helper (Phase 2).
- **Depends on:** none · **Verification:** static + sweep · **Release gate:** yes · **Source:** wifi-032, wifi-033

### POS-AUDIT-220 — [MED][BLE] ble_flood.cpp:134 — ESC mid-conn may leak conn

- **Severity:** MED · **Phase:** 1 · **File:** src/features/ble_flood.cpp:82-135 · **Effort:** S
- **Problem:** `ble_gap_conn_cancel` at exit doesn't terminate new conn if BLE_GAP_EVENT_CONNECT arrives between cancel and function exit.
- **Fix:** Loop `delay(50)` + `ble_gap_conn_cancel()` until `s_flood_last_rc != 0` or 500 ms elapsed.
- **Depends on:** none · **Verification:** ESC during active flood; no conn leak · **Release gate:** yes · **Source:** ble-004

### POS-AUDIT-221 — [MED][BLE] ble_clone.cpp:65 — mac[5]|=0xC0 breaks public-MAC targets

- **Severity:** MED · **Phase:** 1 · **File:** src/features/ble_clone.cpp:65 · **Effort:** S
- **Problem:** OR-with-0xC0 means for public target the clone MAC differs from target — no collision, defeats stated purpose.
- **Fix:** When `g_ble_target.is_public`, abort with `ui_toast("public MAC — clone needs random")` OR document "clone only works on random-addressed peripherals".
- **Depends on:** none · **Verification:** Clone target with random MAC works; public produces toast · **Release gate:** yes · **Source:** ble-005

### POS-AUDIT-222 — [MED][BLE] ble_db.cpp — Duplicate/colliding OUI rows

- **Severity:** MED · **Phase:** 0 · **File:** src/ble_db.cpp:18-136 · **Effort:** S
- **Problem:** 0x10417F Apple twice (line 31); 0x001A11 Google + Xiaomi (first wins); 0x0013A9 Sony + Sony PS; 0x0017A4 Dell + HP. Dead rows.
- **Fix:** Dedup the table; for genuine OEM-vs-OEM splits, pick more current owner.
- **Depends on:** none · **Verification:** Build size delta; classify spot-check · **Release gate:** no · **Source:** ble-006

### POS-AUDIT-223 — [MED][BLE] sigdb_bt.h — Duplicate manufacturer IDs

- **Severity:** MED · **Phase:** 0 · **File:** src/sigdb_bt.h:25-83 · **Effort:** XS
- **Problem:** 0x0131 Cypress twice (55, 75); 0x0157 dup (Anhui Huami vs Mi-Fit). Dead rows burn flash.
- **Fix:** Dedup.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** ble-007

### POS-AUDIT-224 — [MED][BLE] ble_db O(n) OUI lookup → binary search

- **Severity:** MED · **Phase:** 2 · **File:** src/ble_db.cpp:300-305 + sigdb_bt.h:86-91 · **Effort:** M
- **Problem:** ~200 entries linear scan called per BLE adv; ~5000+ table walks per scan in busy env.
- **Fix:** Sort `OUI[]` by `oui` at compile-time, binary-search; same for `bt_mfr_name`.
- **Depends on:** POS-AUDIT-222 + POS-AUDIT-223 · **Verification:** CPU% on dense BLE scan · **Release gate:** yes · **Source:** ble-008

### POS-AUDIT-225 — [MED][BLE] ble_db_identify Samsung/MS shortcuts return without verification

- **Severity:** MED · **Phase:** 1 · **File:** src/ble_db.cpp:307-353 · **Effort:** M
- **Problem:** Samsung CID + sub-prefix mismatch returns "Samsung" for unrelated Samsung phones; badge colour wrong.
- **Fix:** Factor badge colour decision into `addr.is_public` purely; decouple from classify string. Tighten manufacturer-data branch decoders.
- **Depends on:** none · **Verification:** Random-MAC Samsung phone classify · **Release gate:** yes · **Source:** ble-009

### POS-AUDIT-226 — [MED][BLE] ble_whisperpair child-from-target skips scan-stop

- **Severity:** MED · **Phase:** 1 · **File:** src/features/ble_whisperpair.cpp:685-715 · **Effort:** S
- **Problem:** Hotkey path skips scan setup but parent scan from `ble_scan` may still be running.
- **Fix:** `if (scan->isScanning()) scan->stop(); delay(20);` at function head OR ensure caller-side stop.
- **Depends on:** none · **Verification:** W hotkey from scan view works · **Release gate:** yes · **Source:** ble-010

### POS-AUDIT-227 — [MED][BLE] ble_whisperpair 3s busy wait without WDT reset

- **Severity:** MED · **Phase:** 1 · **File:** src/features/ble_whisperpair.cpp:425-429 · **Effort:** XS
- **Problem:** 3 s `delay(50)` loop; no `esp_task_wdt_reset()`; close to 5 s TWDT default.
- **Fix:** `esp_task_wdt_reset()` per iteration.
- **Depends on:** none · **Verification:** Probe a slow target; no WDT panic · **Release gate:** yes · **Source:** ble-011

### POS-AUDIT-228 — [MED][BLE] ble_hid/blueducky GATT services leak on repeat-entry

- **Severity:** MED · **Phase:** 1 · **File:** src/features/ble_hid.cpp:240-253 + ble_blueducky.cpp:600-609 · **Effort:** M
- **Problem:** NimBLEService/Characteristic registered in `setup_hid` owned by server, not HIDDevice. Each entry leaks ~1.5 KB GATT structures.
- **Fix:** Force `radio_switch(RADIO_NONE)` then `radio_switch(RADIO_BLE)` on exit; OR `srv->removeService(s_hid->getHidService(), true)` before delete (NimBLE 2.x supports).
- **Depends on:** none · **Verification:** Repeat-enter Bad-KB 10×; heap stable · **Release gate:** yes · **Source:** ble-012, ble-017

### POS-AUDIT-229 — [LOW][BLE] ble_spam enum-as-int sentinel pattern

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/ble_spam.cpp:172,192 · **Effort:** XS
- **Problem:** Casts enum return to int and tests `< 0`. Works on default int enum; breaks if enum sized later.
- **Fix:** Add `SPAM_NONE=-1` enum value.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** ble-013

### POS-AUDIT-230 — [LOW][BLE] ble_findmy volatile read of stable value per tick

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/ble_findmy.cpp:99 · **Effort:** XS
- **Problem:** Reads volatile `s_fm_tags` per tick; constant within session.
- **Fix:** Cache to local once.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** ble-014

### POS-AUDIT-231 — [LOW][BLE] ble_extras vs ble_scan MAC byte order inconsistent

- **Severity:** LOW · **Phase:** 0 · **File:** ble_extras.cpp:178-179 vs ble_scan.cpp:457-459 + ble_blueducky.cpp · **Effort:** S
- **Problem:** blesniff CSV writes MSB-first; blescan CSV writes LSB-first. Cross-correlation requires manual reverse.
- **Fix:** Canonicalize to display-order MSB-first.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** ble-015

### POS-AUDIT-232 — [LOW][BLE] ble_karma connectable advertising w/o GATT server

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/ble_karma.cpp:116 · **Effort:** S
- **Problem:** `BLE_GAP_CONN_MODE_UND` advertises as connectable but no GATT server registered; connect attempts hit empty surfaces and disconnect.
- **Fix:** Switch to `BLE_GAP_CONN_MODE_NON` for pure beacon karma, OR add minimal generic service.
- **Depends on:** none · **Verification:** Connect attempt cleanly rejected · **Release gate:** no · **Source:** ble-016

### POS-AUDIT-233 — [LOW][BLE] ble_db sentinel-terminated table fragile

- **Severity:** LOW · **Phase:** 0 · **File:** src/ble_db.cpp:135,302 · **Effort:** XS
- **Problem:** `{0, nullptr}` sentinel; a legitimate 0x000000 OUI would terminate early.
- **Fix:** Use `sizeof(OUI)/sizeof(OUI[0])` explicit count.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** ble-017

### POS-AUDIT-234 — [LOW][BLE] Refactor: split ble_whisperpair + ble_blueducky

- **Severity:** LOW · **Phase:** 2 · **File:** src/features/ble_whisperpair.cpp (715) + ble_blueducky.cpp (610) · **Effort:** M
- **Problem:** Both >600 LOC; split candidates.
- **Fix:** ble_whisperpair → scan/crypto/probe + main; ble_blueducky → keymap/hid + main.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** ble-018

### POS-AUDIT-235 — [LOW][BLE] Refactor: ble_random_addr helper

- **Severity:** LOW · **Phase:** 2 · **File:** ble_spam/flood/karma/findmy/sourapple · **Effort:** S
- **Problem:** Random-MAC pattern duplicated in 5 files.
- **Fix:** Promote to `ble_random_addr()` in new `ble_helpers.h`.
- **Depends on:** none · **Verification:** sweep all 5 callers · **Release gate:** no · **Source:** ble-019

### POS-AUDIT-236 — [LOW][BLE] Refactor: ble_hid_common shared

- **Severity:** LOW · **Phase:** 2 · **File:** ble_hid.cpp + ble_blueducky.cpp · **Effort:** M
- **Problem:** Shared HID report descriptor + ascii_to_hid + send_key drift ~150 LOC.
- **Fix:** Promote to `ble_hid_common.cpp`.
- **Depends on:** none · **Verification:** Both Bad-KB and BlueDucky work · **Release gate:** no · **Source:** ble-020

### POS-AUDIT-237 — [LOW][BLE] Refactor: split ble_extras

- **Severity:** LOW · **Phase:** 2 · **File:** src/features/ble_extras.cpp (286) · **Effort:** S
- **Problem:** Mixes tracker + sniffer + iBeacon — 3 features in 1 file.
- **Fix:** Split into `ble_tracker.cpp`, `ble_sniff.cpp`, `ble_ibeacon.cpp`.
- **Depends on:** none · **Verification:** menu-symmetry · **Release gate:** no · **Source:** ble-021

### POS-AUDIT-238 — [LOW][BLE] ble_findmy SD-key load (Evil-M5 parity)

- **Severity:** LOW · **Phase:** 3 · **File:** src/features/ble_findmy.cpp:71 · **Effort:** M
- **Problem:** POSEIDON uses random 22-byte keys per rotation; Evil-M5 reads `/evil/FindMyEvil_keys.txt`.
- **Fix:** Optional SD-key load (`/poseidon/findmy_keys.bin`).
- **Depends on:** none · **Verification:** Drop keys; verify flock uses them · **Release gate:** yes · **Source:** ble-022

### POS-AUDIT-239 — [MED][SubGHz] subghz_jammer cap leaves chip armed

- **Severity:** MED · **Phase:** 1 · **File:** src/features/subghz_jammer.cpp:35-39 · **Effort:** S
- **Problem:** 20 s cap only calls `cc1101_set_idle()`; chip still occupies radio slot. User toast claims "20 s limit" but chip silently armed for re-ENTER.
- **Fix:** Cap path should `cc1101_end(); radio_switch(RADIO_NONE);` to drop slot fully.
- **Depends on:** none · **Verification:** Hit cap; re-enter feature; clean start · **Release gate:** yes · **Source:** rf-003

### POS-AUDIT-240 — [MED][SubGHz] subghz_jammer/bruteforce WDT feed in TX loops

- **Severity:** MED · **Phase:** 1 · **File:** subghz_jammer.cpp + subghz_bruteforce.cpp · **Effort:** S
- **Problem:** Jammer mode-1 full-carrier and brute TX loops have no WDT feed; fragile if cap path extended.
- **Fix:** `esp_task_wdt_reset()` once per second in active loops.
- **Depends on:** none · **Verification:** Stress test 60 s jam + brute · **Release gate:** yes · **Source:** rf-009

### POS-AUDIT-241 — [MED][SubGHz] ui_rssi_scope shared global state

- **Severity:** MED · **Phase:** 2 · **File:** src/ui_subghz.cpp:185-187 · **Effort:** S
- **Problem:** Other features touching scope leak history between sessions.
- **Fix:** Caller responsibility OK if documented; OR namespace per feature.
- **Depends on:** none · **Verification:** sweep · **Release gate:** no · **Source:** rf-010

### POS-AUDIT-242 — [MED][SubGHz] nrf24_suite wasteful re-begin after jammer cap

- **Severity:** MED · **Phase:** 1 · **File:** src/features/nrf24_suite.cpp:684,748 · **Effort:** S
- **Problem:** After 20 s jammer cap, code does `powerDown + nrf24_begin`; user-ESC path also re-begins then immediately ends. Wasted work.
- **Fix:** Drop intermediate re-begin.
- **Depends on:** none · **Verification:** ESC after cap; clean exit · **Release gate:** no · **Source:** rf-011

### POS-AUDIT-243 — [MED][SubGHz] lora_spectrum scope +/- retunes without debounce

- **Severity:** MED · **Phase:** 1 · **File:** src/features/lora_spectrum.cpp:241-307,299-305 · **Effort:** S
- **Problem:** Holding +/- mashes setFrequency dozens of times per second; can wedge on BUSY.
- **Fix:** 100 ms cooldown between `setFrequency` calls.
- **Depends on:** none · **Verification:** Hold + key; no wedge · **Release gate:** yes · **Source:** rf-014

### POS-AUDIT-244 — [MED][SubGHz] cc1101_park_others always gps_end

- **Severity:** MED · **Phase:** 1 · **File:** src/cc1101_hw.cpp:12-27 · **Effort:** S
- **Problem:** Always calls `gps_end()` even if GPS already ended; destructive each call.
- **Fix:** Gate on `gps_is_up()` if exposed.
- **Depends on:** POS-AUDIT-002 · **Verification:** Sub-GHz freq change; GPS state untouched · **Release gate:** no · **Source:** rf-015

### POS-AUDIT-245 — [MED][SubGHz] subghz_scan interrupt attach/detach scattered

- **Severity:** MED · **Phase:** 2 · **File:** src/features/subghz_scan.cpp:147-301 · **Effort:** M
- **Problem:** ISR state management scattered around freq change, replay, save_sub. Fragile.
- **Fix:** RAII wrapper for ISR attach/detach.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** rf-016

### POS-AUDIT-246 — [MED][SubGHz] subghz_spectrum 28.8KB BSS waterfall ring lazy-alloc

- **Severity:** MED · **Phase:** 2 · **File:** src/features/subghz_spectrum.cpp:169 · **Effort:** S
- **Problem:** Static 28.8 KB BSS waterfall ring lives forever.
- **Fix:** `heap_caps_malloc(MALLOC_CAP_DEFAULT)` on entry, free on exit.
- **Depends on:** none · **Verification:** Heap watermark when feature not active · **Release gate:** no · **Source:** rf-017

### POS-AUDIT-247 — [LOW][SubGHz] subghz_broadcast ON-AIR badge pre-flash

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/subghz_broadcast.cpp:280 · **Effort:** S
- **Problem:** Static red badge for whole TX duration. Cosmetic.
- **Fix:** Pre-TX 100 ms intro/decay flash.
- **Depends on:** none · **Verification:** UX · **Release gate:** no · **Source:** rf-019

### POS-AUDIT-248 — [LOW][SubGHz] nrf52 features no Feather FW version check

- **Severity:** LOW · **Phase:** 1 · **File:** nrf52_scout_strike.cpp:196 + peers · **Effort:** S
- **Problem:** Commands may return ERR on older Feather build.
- **Fix:** Version check in `ensure_feather` per POS-AUDIT-258.
- **Depends on:** POS-AUDIT-258 · **Verification:** Older Feather FW · **Release gate:** no · **Source:** rf-020

### POS-AUDIT-249 — [LOW][SubGHz] nrf52_ble_mitm_relay sd_is_mounted gate

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/nrf52_ble_mitm_relay.cpp:230,243 · **Effort:** S
- **Problem:** `sdlog_open` reuses logger without `sd_is_mounted()` gate.
- **Fix:** Add gate.
- **Depends on:** none · **Verification:** Run with SD unmounted; no crash · **Release gate:** no · **Source:** rf-021

### POS-AUDIT-250 — [LOW][SubGHz] nrf24 BLE access addr byte-reverse field validation

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/nrf24_suite.cpp:90-94 · **Effort:** S
- **Problem:** Bit-reversed-byte form correct per spec, but needs field validation against ESP32-as-BLE receiver.
- **Fix:** Cross-check with phone scanner.
- **Depends on:** none · **Verification:** Phone scanner test · **Release gate:** no · **Source:** rf-022

### POS-AUDIT-251 — [LOW][SubGHz] nrf52_suite Arduino String thrash

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/nrf52_suite.cpp:106-138 · **Effort:** S
- **Problem:** `String::indexOf`/`substring` per line; 50+ adv/s = ~12 KB/s churn.
- **Fix:** `strtok_r` or char buffer parsing.
- **Depends on:** none · **Verification:** Long BLE scan; heap stable · **Release gate:** no · **Source:** rf-025

### POS-AUDIT-252 — [LOW][SubGHz] cc1101_begin RxBW hardcoded

- **Severity:** LOW · **Phase:** 1 · **File:** src/cc1101_hw.cpp:75-79 · **Effort:** S
- **Problem:** RxBW=650 hardcoded; record/jam_detect override to 270 immediately after. Double-write.
- **Fix:** `cc1101_begin(freq, rxbw_khz=650)` overload.
- **Depends on:** none · **Verification:** record/jam_detect work · **Release gate:** no · **Source:** rf-026

### POS-AUDIT-253 — [LOW][SubGHz] subghz_scan bit-bang replay → cc1101_rmt_tx

- **Severity:** LOW · **Phase:** 2 · **File:** src/features/subghz_scan.cpp:281-296 · **Effort:** S
- **Problem:** R=replay bit-bangs via `digitalWrite` + `delayMicroseconds`; duplicates RMT poorly.
- **Fix:** Switch to `cc1101_rmt_tx` for `g_subghz_last_cap` replay.
- **Depends on:** none · **Verification:** Recorded replay accuracy on SDR · **Release gate:** yes · **Source:** rf-027

### POS-AUDIT-254 — [LOW][SubGHz] lora_spectrum waterfall row-color shift document

- **Severity:** LOW · **Phase:** 0 · **File:** src/features/lora_spectrum.cpp:211-216 · **Effort:** S
- **Problem:** Color shifts by -20 dBm over 60 rows for texture; doesn't represent freq data.
- **Fix:** Document or make optional.
- **Depends on:** none · **Verification:** docs · **Release gate:** no · **Source:** rf-028

### POS-AUDIT-255 — [LOW][SubGHz] cc1101_park_others defensive SX1262 reset pulse

- **Severity:** LOW · **Phase:** 1 · **File:** src/cc1101_hw.cpp:12-27 · **Effort:** S
- **Problem:** If LoRa state leaked (rf-013 OOM), next CC1101 has chatty MISO.
- **Fix:** Add `pinMode(LORA_RST, OUTPUT); digitalWrite(LOW); delay(1); HIGH;` to `cc1101_park_others`.
- **Depends on:** POS-AUDIT-026 · **Verification:** Stress sub-GHz after LoRa OOM repro · **Release gate:** no · **Source:** rf-029

### POS-AUDIT-256 — [LOW][SubGHz] radio_lora gated SD logging

- **Severity:** LOW · **Phase:** 3 · **File:** src/features/radio_lora.cpp:69-73 · **Effort:** M
- **Problem:** No SD log in LoRa scan; feature gap vs Bruce wardrive CSV.
- **Fix:** Add gated SD log behind `lora_rx_flag`.
- **Depends on:** none · **Verification:** Logs intact · **Release gate:** yes · **Source:** rf-024

### POS-AUDIT-257 — [LOW][SubGHz] Refactor: split nrf24_suite

- **Severity:** LOW · **Phase:** 2 · **File:** src/features/nrf24_suite.cpp (771) · **Effort:** M
- **Problem:** 5 features in one file.
- **Fix:** Split into sniffer/mousejack/ble_spam/scanner/jammer + `nrf24_common.cpp`.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** rf-018

### POS-AUDIT-258 — [LOW][SubGHz] Refactor: nrf52 ensure_feather consolidate

- **Severity:** LOW · **Phase:** 2 · **File:** nrf52_suite + nrf52_ble_mitm_relay + nrf52_scout_strike + nrf52_wifi_ble_combo · **Effort:** S
- **Problem:** 4 near-identical implementations.
- **Fix:** `NRF52Hardware::ensure_or_prompt`.
- **Depends on:** none · **Verification:** all 4 features still work · **Release gate:** no · **Source:** rf-033

### POS-AUDIT-259 — [LOW][SubGHz] Refactor: sub_file_io shared util

- **Severity:** LOW · **Phase:** 2 · **File:** subghz_scan + subghz_record + subghz_broadcast + subghz_replay · **Effort:** M
- **Problem:** `save_sub_file` + `pick_sub_file` + `.sub` parse duplicated.
- **Fix:** Extract `sub_file_io.cpp` with shared parser + saver + picker.
- **Depends on:** POS-AUDIT-025 · **Verification:** all sub-GHz features work · **Release gate:** no · **Source:** rf refactor

### POS-AUDIT-260 — [LOW][SubGHz] Refactor: cc1101_begin(freq, rxbw_khz) overload

- **Severity:** LOW · **Phase:** 2 · **File:** src/cc1101_hw.cpp · **Effort:** S
- **Problem:** Same as POS-AUDIT-252 but as refactor.
- **Fix:** Overload signature.
- **Depends on:** POS-AUDIT-252 · **Verification:** static · **Release gate:** no · **Source:** rf-035

### POS-AUDIT-261 — [LOW][SubGHz] Refactor: hspi_park_all_but

- **Severity:** LOW · **Phase:** 2 · **File:** cc1101_hw + nrf24_hw + lora_hw · **Effort:** S
- **Problem:** Park-others duplication.
- **Fix:** `hspi_park_all_but(int active_cs)` centralised.
- **Depends on:** none · **Verification:** sweep all 4 radios · **Release gate:** no · **Source:** rf refactor

### POS-AUDIT-262 — [LOW][SubGHz] Refactor: nrf52 PING heartbeat for crash recovery

- **Severity:** LOW · **Phase:** 2 · **File:** nrf52_*.cpp · **Effort:** M
- **Problem:** No graceful path when Feather firmware crashes mid-session; UART read hangs.
- **Fix:** Inline `PING` heartbeat every N seconds.
- **Depends on:** none · **Verification:** Kill Feather mid-session; UI recovers · **Release gate:** yes · **Source:** rf-034

### POS-AUDIT-263 — [MED][Net] c5_cmd unconditional set_channel(1)

- **Severity:** MED · **Phase:** 1 · **File:** src/c5_cmd.cpp:260 · **Effort:** S
- **Problem:** Every `c5_begin()` yanks radio to ch 1 without notifying concurrent mesh/wardrive workers.
- **Fix:** Document invariant; OR signal abort to mesh task before set_channel.
- **Depends on:** none · **Verification:** Enter c5_status while wardrive runs; verify wardrive recovers · **Release gate:** yes · **Source:** net-001

### POS-AUDIT-264 — [MED][Net] c5_cmd.h vs proto.h type name drift

- **Severity:** MED · **Phase:** 1 · **File:** src/c5_cmd.h:31 vs c5_node/main/proto.h:34 · **Effort:** S
- **Problem:** S3 calls type 17 CMD_HS, C5 calls POSEI_TYPE_CMD_HS_CAPTURE. Same value, cosmetic.
- **Fix:** Unify name to match c5_node side OR document.
- **Depends on:** none · **Verification:** Build clean · **Release gate:** no · **Source:** net-002

### POS-AUDIT-265 — [MED][Net] dhcp_cache header claims ISR-safe but dhcp_learn not atomic

- **Severity:** MED · **Phase:** 1 · **File:** src/dhcp_cache.cpp:22 + header comment · **Effort:** S
- **Problem:** Header lies; `find()` + `s_n++` not atomic.
- **Fix:** Either add `portMUX` OR correct header comment.
- **Depends on:** none · **Verification:** Static-only docs · **Release gate:** no · **Source:** net-015

### POS-AUDIT-266 — [MED][Net] net_dhcp feat_net_hijack inline copy of rogue_dhcp_loop

- **Severity:** MED · **Phase:** 0 · **File:** src/features/net_dhcp.cpp:421-509 · **Effort:** M
- **Problem:** 75 LOC duplicate of `rogue_dhcp_loop`.
- **Fix:** Parameterized `rogue_dhcp_loop(false, duration_ms)`.
- **Depends on:** none · **Verification:** hijack feature still works · **Release gate:** no · **Source:** net refactor

### POS-AUDIT-267 — [MED][Net] net_wpad build_type2 Type-3 readback duplicated

- **Severity:** MED · **Phase:** 2 · **File:** src/features/net_wpad.cpp:249-261,460-475 · **Effort:** S
- **Problem:** Type-3 readback pattern appears twice.
- **Fix:** Consolidate into one helper.
- **Depends on:** none · **Verification:** WPAD + Autodiscover still capture · **Release gate:** no · **Source:** net refactor

### POS-AUDIT-268 — [MED][Net] Meshtastic hardcoded LongFast PSK + channel hash

- **Severity:** MED · **Phase:** 1 (documentation), **Phase:** 4 (multi-channel impl) · **File:** src/mesh/meshtastic_node.cpp:27-30,464 + meshtastic.h:19-22,49 · **Effort:** L
- **Problem:** PSK + MESH_CHANNEL_HASH=0x08 hardcoded; private channels silently drop packets with no diagnostic.
- **Fix:** Surface "channel filtered" diagnostic in mesh_status; multi-PSK support deferred to Phase 4 (POS-AUDIT-268 phase 4 entry).
- **Depends on:** none · **Verification:** Join private channel; toast shows filter active · **Release gate:** yes · **Source:** net-008

### POS-AUDIT-269 — [MED][Net] net_responder/wpad credential file unbounded append

- **Severity:** MED · **Phase:** 1 · **File:** net_responder.cpp:185 + net_wpad.cpp:161-163 · **Effort:** S
- **Problem:** APPEND forever; no rotation, no max-size check.
- **Fix:** Roll at 64 KB to .1, .2, ...; cap 4 files.
- **Depends on:** none · **Verification:** Long session; verify rotation · **Release gate:** yes · **Source:** net-009

### POS-AUDIT-270 — [MED][Net] net_wpad Serial.println leaks NTLMv2 to USB-CDC

- **Severity:** MED · **Phase:** 1 · **File:** src/features/net_wpad.cpp:164 · **Effort:** S
- **Problem:** `Serial.println("[+] NTLMv2: " + line)` leaks captured hash to USB-CDC. OPSEC.
- **Fix:** Gate behind `POSEIDON_DEBUG_NTLM`; default off.
- **Depends on:** none · **Verification:** No hash on serial in default build · **Release gate:** yes · **Source:** net-010

### POS-AUDIT-271 — [MED][Net] net_lanrecon FILE_WRITE truncates lan.csv

- **Severity:** MED · **Phase:** 1 · **File:** src/features/net_lanrecon.cpp:259 · **Effort:** S
- **Problem:** FILE_WRITE truncates each run, prior recon overwritten silently.
- **Fix:** Use `sdlog_open("lan", ...)` like net_cctv.
- **Depends on:** none · **Verification:** Two runs; both CSVs retained · **Release gate:** yes · **Source:** net-011

### POS-AUDIT-272 — [MED][Net] net_responder SD I/O blocks AsyncUDP

- **Severity:** MED · **Phase:** 1 · **File:** src/features/net_responder.cpp:194 · **Effort:** M
- **Problem:** Handler does `s_log.printf` + `s_log.flush` on every query; can block 5-20 ms on SD I/O.
- **Fix:** Queue-defer log lines to a worker.
- **Depends on:** none · **Verification:** Bursty LLMNR; no UDP drops · **Release gate:** yes · **Source:** net-012

### POS-AUDIT-273 — [MED][Net] c5_cmd Serial.printf hot-path logging

- **Severity:** MED · **Phase:** 1 · **File:** src/c5_cmd.cpp:380-385 · **Effort:** S
- **Problem:** Spam on every tick of every C5 command.
- **Fix:** Wrap in `#ifdef C5_DEBUG` or rate-limit.
- **Depends on:** none · **Verification:** Serial idle during c5 ops · **Release gate:** yes · **Source:** net-013

### POS-AUDIT-274 — [LOW][Net] c5_cmd c5_stas/probes/hits/spectrum_get return 0 forever

- **Severity:** LOW · **Phase:** 1 · **File:** src/c5_cmd.cpp:522-525 · **Effort:** M
- **Problem:** Accessors no-op forever; UI reading them sees empty.
- **Fix:** Implement ring buffers OR remove API surface until C5 catches up.
- **Depends on:** POS-AUDIT-296 · **Verification:** sweep · **Release gate:** no · **Source:** net-014

### POS-AUDIT-275 — [LOW][Net] satcom max_age_sec parameter ignored

- **Severity:** LOW · **Phase:** 1 · **File:** src/satcom.cpp:139 · **Effort:** S
- **Problem:** Cache age intentionally ignored.
- **Fix:** Drop parameter OR implement sidecar `fetched_ts` file.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** net-016

### POS-AUDIT-276 — [LOW][Net] net_attacks upFile leaks on UPLOAD_FILE_START error

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/net_attacks.cpp:528 · **Effort:** S
- **Problem:** `static File upFile` not reset between separate uploads; failed upload leaks handle.
- **Fix:** Close pre-existing handle on `UPLOAD_FILE_START`.
- **Depends on:** none · **Verification:** Fail upload mid-stream; verify no leak · **Release gate:** no · **Source:** net-017

### POS-AUDIT-277 — [LOW][Net] net_attacks SSDP poisoner cap 200 vs Evil-M5 300

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/net_attacks.cpp:744 · **Effort:** S
- **Problem:** Cap 200 — PSRAM broken on this hardware so 300 not achievable.
- **Fix:** Document accepted constraint; leave at 200.
- **Depends on:** none · **Verification:** Doc update · **Release gate:** no · **Source:** net-018

### POS-AUDIT-278 — [LOW][Net] net_dhcp ROGUE_POOL_SZ silent ceiling

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/net_dhcp.cpp:246 · **Effort:** S
- **Problem:** Once 32 clients alloc'd, further requests return suffix=0 silently. UI shows 32/32 but no error toast.
- **Fix:** Toast at 90%.
- **Depends on:** none · **Verification:** Stress 32 clients; toast appears · **Release gate:** no · **Source:** net-019

### POS-AUDIT-279 — [LOW][Net] net_ssdp duplicate header lines across appends

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/net_ssdp.cpp:158 · **Effort:** S
- **Problem:** Writes header line every run; duplicates accumulate.
- **Fix:** Only write header if file empty (`f.size() == 0`).
- **Depends on:** none · **Verification:** Two runs; one header · **Release gate:** no · **Source:** net-020

### POS-AUDIT-280 — [LOW][Net] meshtastic RX task polls every 20ms

- **Severity:** LOW · **Phase:** 2 · **File:** src/mesh/meshtastic_node.cpp:449 · **Effort:** M
- **Problem:** Could be IRQ-driven via `setPacketReceivedAction` — saves ~80% cycles.
- **Fix:** Use SX1262 DIO IRQ via RadioLib.
- **Depends on:** none · **Verification:** RX still works; CPU% drops · **Release gate:** yes · **Source:** net-021

### POS-AUDIT-281 — [LOW][Net] c5_cmd c5_peer_name unsafe variant deprecate

- **Severity:** LOW · **Phase:** 1 · **File:** src/c5_cmd.cpp:344-348 · **Effort:** S
- **Problem:** Returns pointer to live array without lock; safer `c5_peer_name_copy` exists.
- **Fix:** Mark `[[deprecated]]` or delete and chase callers.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** net-022

### POS-AUDIT-282 — [LOW][Net] net_lanrecon banner+http use raw WiFiClient

- **Severity:** LOW · **Phase:** 2 · **File:** src/features/net_lanrecon.cpp · **Effort:** S
- **Problem:** Uses raw WiFiClient instead of `net_helpers::net_http_get` / `net_tcp_open`.
- **Fix:** Route through helpers.
- **Depends on:** none · **Verification:** static · **Release gate:** no · **Source:** net-023

### POS-AUDIT-283 — [MED][System] screensaver_check_idle races feature animation loops

- **Severity:** MED · **Phase:** 1 · **File:** src/screensaver.cpp:1085-1089 + menu.cpp:1286 · **Effort:** M
- **Problem:** Only menu repaint forced on wake; features running own animation loops can be silently clobbered.
- **Fix:** Move idle-detect into `input_poll` so any feature waiting on it benefits.
- **Depends on:** none · **Verification:** Enter long-running feature; idle in; screensaver doesn't clobber · **Release gate:** yes · **Source:** sys-012

### POS-AUDIT-284 — [MED][System] sd_helper/tools two parallel SD-wipe implementations

- **Severity:** MED · **Phase:** 1 · **File:** src/sd_helper.cpp:115-142 vs tools.cpp:28-69 · **Effort:** S
- **Problem:** Two recursive nukes; `sd_format()` bypasses confirmation if called externally.
- **Fix:** Consolidate; move recursive nuke into sd_helper.cpp; document confirmation contract.
- **Depends on:** none · **Verification:** SD format from tools menu works · **Release gate:** yes · **Source:** sys-014

### POS-AUDIT-285 — [MED][System] hat_manager detect() stub returns NONE

- **Severity:** MED · **Phase:** 0 · **File:** src/hat_manager.cpp:27-36 + hat_manager.h · **Effort:** S/M
- **Problem:** Implementation unconditionally returns `HatType::NONE`. Map claims active probing — false. Park helpers unused.
- **Fix:** Either implement probe sequences (SX1262 v05, CC1101 PARTNUM=0x80, NRF24 STATUS) OR delete the class.
- **Depends on:** none · **Verification:** Per-hat presence reported correctly OR deletion sweep · **Release gate:** no · **Source:** sys-013

### POS-AUDIT-286 — [MED][System] deauth_autotest cruft in production main

- **Severity:** MED · **Phase:** 1 · **File:** src/main.cpp:164-179 + deauth_autotest.cpp · **Effort:** S
- **Problem:** Dev-only stress test gated by `-DPOSEIDON_AUTO_DEAUTH_TEST` but ifdef + code is cruft.
- **Fix:** Move to `test/` directory OR behind runtime flag via serial cmd.
- **Depends on:** none · **Verification:** Production build unchanged · **Release gate:** no · **Source:** sys-016

### POS-AUDIT-287 — [MED][System] serial_test unauth R\n reset + always-on 4KB task

- **Severity:** MED · **Phase:** 1 · **File:** src/serial_test.cpp:46-49,68-71 · **Effort:** S
- **Problem:** Any USB-CDC peer (TRIDENT bridge, hot-plug) writing "R\n" resets device with no auth. Always-on 4 KB task wastes heap.
- **Fix:** Lazy-start on first incoming byte; require "RESET\n" full string.
- **Depends on:** none · **Verification:** Plug TRIDENT; no reset · **Release gate:** yes · **Source:** sys-017

### POS-AUDIT-288 — [MED][System] menu.cpp double C5 peer-walk per repaint

- **Severity:** MED · **Phase:** 1 · **File:** src/menu.cpp:1090-1113 + ui.cpp:152 · **Effort:** S
- **Problem:** `c5_any_online()` + `c5_peer_count()` walked every paint frame (~33 ms); ui_draw_status also reads.
- **Fix:** Cache `c5_n` once per poll tick.
- **Depends on:** none · **Verification:** Heap-pressure trace · **Release gate:** no · **Source:** sys-018

### POS-AUDIT-289 — [MED][System] Five parallel NVS lazy-read patterns → prefs_helper

- **Severity:** MED · **Phase:** 2 · **File:** ui_ambient.cpp + theme.cpp + screensaver.cpp + menu.cpp + sfx.cpp · **Effort:** M
- **Problem:** Five `s_loaded` flags + `Preferences p; p.begin; p.get; p.end;` patterns.
- **Fix:** Consolidate into `prefs_helper.{cpp,h}` with `prefs_read_*` / `prefs_write_*` and per-namespace caches.
- **Depends on:** none · **Verification:** All 5 namespaces still load · **Release gate:** no · **Source:** sys-019

### POS-AUDIT-290 — [LOW][System] radio.cpp delay(100) unconditional

- **Severity:** LOW · **Phase:** 1 · **File:** src/radio.cpp:132 · **Effort:** S
- **Problem:** Delay after every teardown unconditional; perceptible lag.
- **Fix:** Gate on actual domain (NimBLE deinit needs it; WiFi STA stop doesn't).
- **Depends on:** none · **Verification:** UX faster · **Release gate:** no · **Source:** sys-020

### POS-AUDIT-291 — [LOW][System] ui_clear_body strobe-suppression drops legit clears

- **Severity:** LOW · **Phase:** 1 · **File:** src/ui.cpp:64-78 · **Effort:** S
- **Problem:** If cleared >4 times in 80 ms, skip — brittle; fast features legitimately need 5 clears.
- **Fix:** `ui_clear_body_force` already exists at :84 — audit every caller to use force when appropriate.
- **Depends on:** none · **Verification:** Per-feature sweep · **Release gate:** no · **Source:** sys-021

### POS-AUDIT-292 — [LOW][System] splash.cpp map says trident; code is Hokusai

- **Severity:** LOW · **Phase:** 0 · **File:** _audit/maps/poseidon.md:28 vs src/splash.cpp:55-148 · **Effort:** trivial
- **Problem:** Map wrong.
- **Fix:** Update map line 28.
- **Depends on:** none · **Verification:** docs · **Release gate:** no · **Source:** sys-022

### POS-AUDIT-293 — [LOW][System] argus no retry after RAM alloc fail

- **Severity:** LOW · **Phase:** 1 · **File:** src/argus.cpp:134-145 · **Effort:** M
- **Problem:** On alloc fail, permanently falls back to flash `pushImage` — TX-cache scramble risk.
- **Fix:** Allow retry after `radio_switch(RADIO_BLE)` (when feature opens that doesn't WiFi TX).
- **Depends on:** none · **Verification:** Triton STORM after BLE feature; sprite intact · **Release gate:** yes · **Source:** sys-023

### POS-AUDIT-294 — [LOW][System] menu_carousel letter mnemonic missing IR park

- **Severity:** LOW · **Phase:** 1 · **File:** src/menu_carousel.cpp:391-415 · **Effort:** S
- **Problem:** Letter mnemonic feature exec path missing defensive IR park that ENTER path has at :376-377. Whatever the right polarity should be (POS-AUDIT-001), both paths need it.
- **Fix:** Mirror ENTER-path IR park to letter mnemonic.
- **Depends on:** POS-AUDIT-001 · **Verification:** mnemonic launch IR feature; no LED state leak · **Release gate:** yes · **Source:** sys-024

### POS-AUDIT-295 — [LOW][System/c5_node] c5_node app_main no error gating per-band proto set

- **Severity:** LOW · **Phase:** 1 · **File:** c5_node/main/main.c:222-298,282 · **Effort:** S
- **Problem:** `WIFI_ERR_NOT_SUPPORTED` on older IDF silently degrades 5 GHz; HELLO still advertises 5G capability.
- **Fix:** `ESP_ERROR_CHECK` OR mark `s_has_5g = false` so HELLO is honest.
- **Depends on:** none · **Verification:** Older C5 fw; HELLO truthful · **Release gate:** no · **Source:** sys-025

### POS-AUDIT-296 — [LOW][System/c5_node] c5_node CMDs 18-26 stubbed

- **Severity:** LOW · **Phase:** 1/3 · **File:** c5_node/main/main.c:198-208 + src/c5_cmd.h surface · **Effort:** S/L
- **Problem:** UI calls e.g. `c5_cmd_beacon_spam` will sit waiting for RESP frames that never arrive.
- **Fix:** Either gate S3-side feature menu entries behind "C5 firmware ≥ X.Y" (S), OR implement on C5 (L).
- **Depends on:** none · **Verification:** Either gate appears OR commands work · **Release gate:** yes · **Source:** sys-026

### POS-AUDIT-297 — [LOW][System] version.h inline without static

- **Severity:** LOW · **Phase:** 0 · **File:** src/version.h:19-20 · **Effort:** trivial
- **Problem:** `inline const char *poseidon_version()` not `static inline`; -flto warns on some toolchains. Bruce uses `static inline`.
- **Fix:** Prepend `static`.
- **Depends on:** none · **Verification:** Build clean · **Release gate:** no · **Source:** sys-027

### POS-AUDIT-298 — [HIGH][IR] ir_clone Samsung Power cmd 0x40 stale (should be 0x02)

- **Severity:** HIGH · **Phase:** 1 · **File:** src/features/ir_clone.cpp:152 + ir_remote.cpp:32 (comment) · **Effort:** S
- **Problem:** SAMSUNG_BTNS Power cmd `0x40` but ir_remote.cpp:32 comment says correct toggle is `0x02`. Two files disagree; ir_clone is stale.
- **Fix:** SAMSUNG_BTNS[0].cmd = 0x02.
- **Depends on:** none · **Verification:** Test Samsung TV power toggle from ir_clone feature · **Release gate:** yes · **Source:** ir-002

### POS-AUDIT-299 — [LOW][IR] ir_tvbgone xTaskCreate no pdFAIL handling

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/ir_tvbgone.cpp:155 · **Effort:** S
- **Problem:** No failure handling on `xTaskCreate(blaster_task, 3072)`.
- **Fix:** Toast on pdFAIL.
- **Depends on:** none · **Verification:** Force low heap; UI shows error · **Release gate:** no · **Source:** ir-004

### POS-AUDIT-300 — [LOW][IR] Refactor ir_extras_data split

- **Severity:** LOW · **Phase:** 0 · **File:** src/features/ir_extras_data.h (1028 LOC) · **Effort:** M
- **Problem:** Header monolith mixing raw codes / command tables / AC captures / inline prank drivers; forward decls at file scope cause ODR risk.
- **Fix:** Split into `ir_codes_tvs.h`, `ir_codes_projector_soundbar.h`, `ir_codes_ac.h`, `ir_pranks.cpp` (real .cpp).
- **Depends on:** none · **Verification:** Build clean; pranks still work · **Release gate:** no · **Source:** ir-003

### POS-AUDIT-301 — [MED][GPS] gps_poll cap byte-drain ≤256 per call

- **Severity:** MED · **Phase:** 1 · **File:** src/gps.cpp:147 · **Effort:** S
- **Problem:** Under 115200 baud with 8 sentences/sec, each call drains ~1500 bytes blocking ~10 ms. Triton calls every loop.
- **Fix:** Cap iteration count per call (e.g. 256 bytes max).
- **Depends on:** none · **Verification:** Triton with full-sky GPS; UI responsive · **Release gate:** yes · **Source:** gps-001

### POS-AUDIT-302 — [LOW][GPS] gps_end explicit pinMode INPUT

- **Severity:** LOW · **Phase:** 1 · **File:** src/gps.cpp:29 · **Effort:** S
- **Problem:** TX pin G13 may be left OUTPUT-HIGH post `s_uart.end()`; if CC1101 then drives LOW for CS-assert, momentary back-drive.
- **Fix:** Explicit `pinMode(GPS_UART_TX_PIN, INPUT)`.
- **Depends on:** none · **Verification:** Scope on G13 during transition · **Release gate:** no · **Source:** gps-002

### POS-AUDIT-303 — [LOW][GPS] gps_poll overflow surfaced in s_diag

- **Severity:** LOW · **Phase:** 1 · **File:** src/gps.cpp:160 · **Effort:** S
- **Problem:** Overflow discards line silently; diag looks fine even when wedged.
- **Fix:** Bump overflow counter in `s_diag`.
- **Depends on:** none · **Verification:** Diag screen shows overflow count · **Release gate:** no · **Source:** gps-003

### POS-AUDIT-304 — [LOW][Specials] badusb kbuf[16] bump to 64 + surface truncation

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/badusb.cpp:120 · **Effort:** S
- **Problem:** `kbuf[16]` truncates modifier tail silently.
- **Fix:** Bump to 64; toast on overflow.
- **Depends on:** none · **Verification:** Long COMBO line · **Release gate:** no · **Source:** bdu-002

### POS-AUDIT-305 — [LOW][Specials] badusb type_string multibyte-aware

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/badusb.cpp:38 · **Effort:** M
- **Problem:** Non-ASCII bytes (>127) rejected silently; Latin-1/UTF-8 STRING payloads drop chars.
- **Fix:** Switch to multibyte-aware iteration.
- **Depends on:** none · **Verification:** Test Latin-1 STRING · **Release gate:** no · **Source:** bdu-003

### POS-AUDIT-306 — [LOW][Specials] badusb implement real COMBO keyword

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/badusb.cpp:16 (comment), :124-129 · **Effort:** S
- **Problem:** File comment promises `COMBO CTRL ALT T` but parser doesn't actually implement it.
- **Fix:** Add real COMBO parsing alongside existing CTRL/ALT/SHIFT modifiers.
- **Depends on:** none · **Verification:** COMBO CTRL ALT t triggers terminal · **Release gate:** no · **Source:** bdu-004

### POS-AUDIT-307 — [LOW][Specials] badusb OPSEC VID/PID spoof toggle

- **Severity:** LOW · **Phase:** 4 · **File:** src/features/badusb.cpp + new System Settings entry · **Effort:** S
- **Problem:** Device enumerates as ESP32-S3-DevKitC defaults. OPSEC opportunity to fake Logitech.
- **Fix:** Settings toggle calling `USB.VID(0x046D)` etc.
- **Depends on:** none · **Verification:** USB enum check · **Release gate:** yes · **Source:** bdu-005

### POS-AUDIT-308 — [MED][Specials] drone_remoteid altitude offset spec verification

- **Severity:** MED · **Phase:** 1 · **File:** src/features/drone_remoteid.cpp:184 · **Effort:** S
- **Problem:** Altitude formula `((float)alt_raw * 0.5f) - 1000.0f` matches F3411-22a §A.2.1.2 only for pressure-alt (bytes 13-14); code applies it to geodetic (bytes 15-16).
- **Fix:** Verify against spec; correct the byte-offset vs scaling combo.
- **Depends on:** none · **Verification:** Replay known drone broadcast; match published altitude · **Release gate:** yes · **Source:** drn-002

### POS-AUDIT-309 — [MED][Specials] surveillance_hunter set rx_cb BEFORE promisc enable

- **Severity:** MED · **Phase:** 1 · **File:** src/features/surveillance_hunter.cpp:288-289 · **Effort:** S
- **Problem:** Order backwards per IDF docs; window where promisc is on without callback can panic if a frame lands.
- **Fix:** Set callback first, then enable promisc.
- **Depends on:** none · **Verification:** Cold-boot surveillance; no panic · **Release gate:** yes · **Source:** srv-001

### POS-AUDIT-310 — [LOW][Specials] surveillance BLE side (Raven UUID 0x09C8)

- **Severity:** LOW · **Phase:** 3 · **File:** src/features/surveillance_hunter.cpp:14-17 (deferred TODO) · **Effort:** L
- **Problem:** Surveillance is WiFi-only.
- **Fix:** Add BLE side w/ RAVEN_BLE manufacturer-ID 0x09C8 + RAVEN_UUID_16 detection.
- **Depends on:** none · **Verification:** Replay Raven BLE pattern · **Release gate:** yes · **Source:** srv-002

### POS-AUDIT-311 — [LOW][Specials] surveillance JSONL null lat/lon when no GPS

- **Severity:** LOW · **Phase:** 0 · **File:** src/features/surveillance_hunter.cpp + defensive_monitor.cpp:520-530 · **Effort:** S
- **Problem:** Writes `0.0,0.0` literally when no GPS; downstream tooling interprets as Null Island. OPSEC + correctness.
- **Fix:** Write `null` JSON value or omit field when `g.valid == false`.
- **Depends on:** none · **Verification:** JSONL parses; no Null Island rows · **Release gate:** yes · **Source:** srv-003, dfn GPS

### POS-AUDIT-312 — [LOW][Specials] defensive_monitor zero-MAC BCAST misses random deauthers

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/defensive_monitor.cpp:364 · **Effort:** S
- **Problem:** `k_bcast_zero[6] = {0}` BCAST detect, but real Spacehuhn uses random BSSID — class under-triggers.
- **Fix:** Add `addr2 == FF:FF:FF:FF:FF:FF` and broadcast-RA heuristic.
- **Depends on:** none · **Verification:** Replay Spacehuhn frames; detected · **Release gate:** yes · **Source:** dfn-004

### POS-AUDIT-313 — [MED][Specials] triton_learn_save truncates brain.bin on SD-fault

- **Severity:** MED · **Phase:** 1 · **File:** src/features/triton.cpp:131-138 · **Effort:** S
- **Problem:** No `sd_is_mounted()` check before writing brain.bin at session-end; FILE_WRITE = truncate.
- **Fix:** Check `sd_is_mounted()` first; OR write to `.bin.tmp` and rename atomically.
- **Depends on:** none · **Verification:** Unmount mid-session; verify brain.bin survives · **Release gate:** yes · **Source:** tri-003

### POS-AUDIT-314 — [LOW][Specials] triton bump BS_N 24→64

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/triton.cpp:172 · **Effort:** S
- **Problem:** `s_bs[24]` BSSID→SSID cache fills in dense environments; subsequent EAPOLs lack ESSID hex for hashcat.
- **Fix:** Bump to 64 with LRU eviction.
- **Depends on:** none · **Verification:** Dense env; check hashcat output completeness · **Release gate:** no · **Source:** tri-005, opt-1

### POS-AUDIT-315 — [HIGH][Specials] trident readRect+Serial.write 135×/frame batch

- **Severity:** HIGH · **Phase:** 1 · **File:** src/features/trident.cpp:35,38 · **Effort:** M
- **Problem:** `readRect` × 135 + `Serial.write` × 135 per frame at 10 fps contends with WiFi TX bus; ~1351 Serial.writes/sec.
- **Fix:** Batch multiple scanlines per Serial.write call (8 lines = 3840 bytes) to amortise USB-CDC framing cost.
- **Depends on:** none · **Verification:** TRIDENT bridge at 10 fps + active WiFi TX; no scramble, framerate ≥ current · **Release gate:** yes · **Source:** trd-001, opt-4

### POS-AUDIT-316 — [MED][Specials] trident check g_mimir_cdc_active before claim

- **Severity:** MED · **Phase:** 1 · **File:** src/features/trident.cpp:211 · **Effort:** S
- **Problem:** `g_trident_cdc_active=true` set without checking Mimir first; concurrent entry corrupts both flags.
- **Fix:** Mirror BadUSB's exclusive check.
- **Depends on:** none · **Verification:** Try Trident while Mimir active; toast + abort · **Release gate:** yes · **Source:** trd-002

### POS-AUDIT-317 — [MED][Specials] trident loot streaming Arduino String per-byte

- **Severity:** MED · **Phase:** 1 · **File:** src/features/trident.cpp:157-176 · **Effort:** S
- **Problem:** `String row += (char)c` per byte → O(n²) realloc; 32 KB log can block seconds + exhaust heap.
- **Fix:** Fixed scratch buffer + line-batched send.
- **Depends on:** none · **Verification:** Loot 32 KB log; no heap exhaustion · **Release gate:** yes · **Source:** trd-003

### POS-AUDIT-318 — [LOW][Specials] input_inject ring overflow bump + drop report

- **Severity:** LOW · **Phase:** 1 · **File:** src/input.cpp:31,36-39 · **Effort:** S
- **Problem:** `s_injected[16]` ring drops on overflow silently; Trident host has no flow-control feedback.
- **Fix:** Bump to 32 AND report drop count to Trident host in `status` response.
- **Depends on:** none · **Verification:** Flood inject; host sees drop count · **Release gate:** no · **Source:** trd-004

### POS-AUDIT-319 — [MED][Specials] mimir check g_trident_cdc_active before claim

- **Severity:** MED · **Phase:** 1 · **File:** src/features/mimir.cpp:555 · **Effort:** S
- **Problem:** Symmetric to POS-AUDIT-316; concurrent Mimir/Trident entry corrupts both flags.
- **Fix:** Check `g_trident_cdc_active` first.
- **Depends on:** none · **Verification:** Try Mimir while Trident active; toast + abort · **Release gate:** yes · **Source:** mim-002

### POS-AUDIT-320 — [LOW][Specials] mimir JSON parser guard nested-escape

- **Severity:** LOW · **Phase:** 1 · **File:** src/features/mimir.cpp:62-87 · **Effort:** S
- **Problem:** `json_str` uses `strstr` over line buffer — `\"key\":\"` matches across nested escaped strings.
- **Fix:** Guard against false matches OR document protocol assumption.
- **Depends on:** none · **Verification:** Test pathological SSID with embedded escapes · **Release gate:** no · **Source:** mim-003

### POS-AUDIT-321 — [MED][SaltyJack] saltyjack_wpad nt_len cap at 1024

- **Severity:** MED · **Phase:** 1 · **File:** src/features/saltyjack/saltyjack_wpad.cpp:163 · **Effort:** S
- **Problem:** Only checks `if (nt_len < 16) return`; no upper bound; 65535-byte nt_len triggers ~130 KB allocation that fails on no-PSRAM S3.
- **Fix:** Mirror responder's `nt_resp_len <= 1024` cap.
- **Depends on:** none · **Verification:** Crafted oversized NTLM response; no OOM · **Release gate:** yes · **Source:** slt-015

