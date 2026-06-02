# POSEIDON vs Bruce / Ghost ESP / PORKCHOP / Evil-M5 — Feature Matrix

Verdict symbols: ✓ implemented/parity · ✓+ implemented and best-in-class · ~ partial/divergent approach · ✗ absent · n/a doesn't apply.

## Matrix

### WiFi

| Feature | POSEIDON | Bruce | Ghost | Porkchop | EvilM5 |
|---|---|---|---|---|---|
| WiFi scan | wifi_scan.cpp:340 ✓+ | wifi_common.cpp:183 ✓ | wifi_manager.c:944 ✓ | network_recon.cpp:759 ✓ | Evil-Cardputer-v1-5-2.ino:2628 ✓ |
| WiFi deauth (targeted) | wifi_deauth.cpp:205 ✓ | deauther.cpp:143 ✓ | wifi_manager.c:1099 ✓ | oink.cpp sendDeauthFrame ✓ | Evil-Cardputer-v1-5-2.ino:2645 ✓ |
| WiFi deauth (broadcast nuke) | wifi_deauth_extras.cpp:125 ✓+ | ✗ | ✓ | ✓ | ✗ |
| WiFi beacon spam | wifi_beacon_spam.cpp:146 ✓ | wifi_atks.cpp:977 ✓ | wifi_manager.c:2153 ✓+ | bacon.cpp:496 ~ | Evil-Cardputer-v1-5-2.ino:2644 ✓ |
| Beacon AP-list mode | ✗ | ✗ | wifi_manager_broadcast_ap -l ✓+ | ✗ | ✗ |
| 802.11ax HE Capabilities IE | ✗ | ✗ | wifi_manager.c:2153 ✓+ | ✗ | ✗ |
| WiFi probe sniff | wifi_probe.cpp:516 ✓+ | karma_attack.cpp:909 ~ | callbacks.c:587 ✓ | ✗ | Evil-Cardputer-v1-5-2.ino:2635 ✓ |
| WiFi Karma | wifi_probe.cpp:517 ✓ | karma_attack.cpp:909 ✓+ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:2373 ✓ |
| Evil portal | wifi_portal.cpp:582 ~ | evil_portal.cpp:61 ~ | wifi_manager.c:726 ~ | ✗ | Evil-Cardputer-v1-5-2.ino:2628 ✓+ |
| Portal templates count | 16 inline | 1 SD | URL-proxied | n/a | 58 (31+27) ✓+ |
| Portal SD-loadable | ✗ | ✗ | ✗ | n/a | /evil/sites/*.html ✓+ |
| Evil Twin (clone + portal + deauth) | evil_twin.cpp:364 ✓+ | ~ | ~ | ✗ | Evil-Cardputer-v1-5-2.ino:20154 ✓ |
| AP Clone | wifi_portal.cpp:653 ✓+ | ✗ | ✗ | ✗ | ✓ |
| Wardrive (WiGLE CSV) | wifi_wardrive.cpp:180 ✓ | wardriving.cpp:451 ✓ | callbacks.c:505 ✓ | warhog.cpp:459 ✓ | utilities/wardriving/ ✓ |
| WiGLE upload | ✗ | wigle.h ✓ | ✗ | web/wigle.cpp ✓ | ✗ |
| wpasec upload | ✗ | ✗ | ✗ | web/wpasec.cpp ✓+ | ✗ |
| PMKID extract (hashcat 22000) | wifi_pmkid.cpp:458 ✓+ | karma_attack.cpp:2326 ~ | callbacks.c:639 (pcap only) ~ | oink.cpp processEAPOL ✓ | pcap-based ~ |
| Handshake M1/M2 capture | wifi_pmkid.cpp:151 ✓+ | wifi_recover.cpp:235 (offline) ~ | callbacks.c:639 ✓ | oink.cpp ✓ | EAPOL counter :809 ~ |
| EAPOL → PCAP | ✗ | ✗ | callbacks.c:639 ✓+ | oink.cpp ✓ | utilities/wardriving ✓ |
| Pwnagotchi detect | ✗ | pwngrid.cpp:310 ✓ | callbacks.c:626 ✓+ | ✗ | Evil-Cardputer-v1-5-2.ino:2646 ✓ |
| WPS detect | wifi_scan.cpp (IE flag) ✓ | ✓ | callbacks.c:652 ✓ | ✗ | ✗ |
| PineAP rogue-AP detect | ✗ | ✗ | callbacks.c:174 ✓+ | ✗ | ✗ |
| Sanity-check bypass (raw 80211 TX) | wifi_sanity_override.cpp:33 (linker stub) ✓ | wifi_atks.cpp:157 en_sys_seq=false ~ | main/main.c:23 (linker stub) ✓ | wsl_bypasser.cpp -zmuldefs ✓ | n/a |
| Clients (one AP) | wifi_clients.cpp:98 ✓ | sniffer.cpp:696 ✓ | wifi_manager.c:1040 ✓ | ✗ | Evil-Cardputer-v1-5-2.ino:2652 ✓ |
| Clients (global hunter) | wifi_clients_all.cpp:148 ✓+ | ✗ | ✗ | ✗ | ✗ |
| WiFi spectrum (2.4 GHz) | wifi_spectrum.cpp:385 ✓+ | ✗ | ✗ | spectrum.cpp ✓+ | ✗ |
| CIW SSID injection | wifi_ciw.cpp:214 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Hidden-SSID reveal | ✗ | ✗ | ✗ | spectrum.cpp ✓+ | ✗ |
| AP signal test | ap_signal_test.cpp:144 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Deauth detector | wifi_deauth_extras.cpp:289 ✓+ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:2653 ✓ |

### BLE

| Feature | POSEIDON | Bruce | Ghost | Porkchop | EvilM5 |
|---|---|---|---|---|---|
| BLE scan (active+passive) | ble_scan.cpp:343 ✓+ | ble_common.cpp:169 ✓ | ble_manager.c:465 ✓ | piggyblues.h:94 ✓ | Evil-Cardputer-v1-5-2.ino:28939 ~ |
| BLE spam (multi-brand) | ble_sourapple.cpp:409 (7-mode) ✓+ | ble_spam.cpp:482 (5-mode) ✓ | ✗ | piggyblues.cpp ✓ | .ino:308 ✓ |
| BLE flood / connection DoS | ble_flood.cpp:82 (single) ~ | BLE_Suite.cpp:2725 (multi) ✓+ | ✗ | ✗ | ✗ |
| BLE sour apple | ble_sourapple.cpp:409 ✓ | ble_spam.cpp:156 ✓ | ✗ | ✓ | ✗ |
| BLE finder (Geiger RSSI) | ble_finder.cpp:307 ✓+ | ✗ | ✗ | ✗ | ✗ |
| BLE findmy (AirTag emit) | ble_findmy.cpp:114 ✓ | ✗ | ✗ | ✗ | .ino:29927 ✓+ (SD keys) |
| BLE karma | ble_karma.cpp:65 ✓+ | ✗ | ✗ | ✗ | ✗ |
| BLE whisperpair CVE-2025-36911 | ble_whisperpair.cpp:615 (1-variant) ✓ | BLE_Suite.cpp:1115 (multi-variant) ✓+ | ✗ | ✗ | ✗ |
| BLE clone (rebroadcast identity) | ble_clone.cpp:52 ~ | BLE_Suite.cpp:2091 (AuthBypass) ✓+ | ✗ | ✗ | ✗ |
| BLE GATT explorer | ble_gatt.cpp:220 ✓ | BLE_Suite.cpp:214 ✓ | ✗ | ✗ | ✗ |
| BLE HID (Bad-KB) | ble_hid.cpp:176 (1-tactic) ✓ | BLE_Suite.cpp:921 (9-tactic) ✓+ | ✗ | ✗ | ✗ |
| BlueDucky CVE-2023-45866 | ble_blueducky.cpp:543 ✓ | ducky_typer.cpp:454 ✓ | ✗ | ✗ | ✗ |
| BLE MITM relay | nrf52_ble_mitm_relay.cpp:200 ✓+ | ✗ | ✗ | ✗ | ✗ |
| BLE Ninebot scooter | ✗ | ble_ninebot.cpp:111 ✓+ | ✗ | ✗ | ✗ |
| BLE tracker / AirTag scanner | ble_extras.cpp:77 ✓ | ✗ | ble_manager.c:389 ✓ | ✗ | ✗ |
| BLE spam DETECTOR | ✗ | ✗ | ble_manager.c:348 ✓+ | ✗ | ✗ |
| BLE skimmer detect | ✗ | ✗ | ble_manager.c:746 ✓+ | ✗ | ✗ |
| BLE wardrive CSV | ✗ | ✗ | callbacks.c:769 ✓+ | ✗ | ✗ |
| BLE pcap | ✗ | ✗ | vendor/pcap.c ✓+ | ✗ | ✗ |
| BLE toys (Lovense) | ble_toys.cpp:223 ✓+ | ✗ | ✗ | ✗ | ✗ |

### Sub-GHz / CC1101 / nRF24 / nRF52 / LoRa

| Feature | POSEIDON | Bruce | Ghost | Porkchop | EvilM5 |
|---|---|---|---|---|---|
| CC1101 SPI init | cc1101_hw.cpp:42 ✓ | rf_utils.cpp:356 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz scan | subghz_scan.cpp:133 ✓ | rf_scan.cpp:13 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz replay | subghz_replay.cpp:115 ✓ | rf_send.cpp:137 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz record (.sub Flipper) | subghz_record.cpp:67 ✓+ | record.cpp:287 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz broadcast (categorised library) | subghz_broadcast.cpp:214 ✓+ | SD .sub only ~ | ✗ | ✗ | ✗ |
| Sub-GHz signal library (baked) | subghz_signals_data.h ✓+ | ✗ | ✗ | ✗ | ✗ |
| Sub-GHz bruteforce | subghz_bruteforce.cpp:34 ✓ | rf_bruteforce.cpp:101 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz spectrum | subghz_spectrum.cpp:362 ✓+ | rf_spectrum.cpp:28 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz jammer | subghz_jammer.cpp:18 ✓ | rf_jammer.cpp:212 ✓ | ✗ | ✗ | ✗ |
| Sub-GHz jam-detect (RSSI anomaly) | subghz_jam_detect.cpp:68 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Sub-GHz listen (audio passthrough) | ✗ | rf_listen.cpp:26 ✓+ | ✗ | ✗ | ✗ |
| RMT TX/RX (microsecond timing) | cc1101_rmt.cpp ✓+ | bit-bang loop ~ | ✗ | ✗ | ✗ |
| IR jammer (multi-protocol) | ✗ | ir_jammer.cpp:698 ✓+ | ✗ | ✗ | ✗ |
| IR TV-B-Gone | ir_tvbgone.cpp:139 ~ (6 brands) | TV-B-Gone.cpp:149 ✓+ (WORLD codes) | ✗ | ✗ | ✗ |
| IR clone (RX capture) | ✗ (v2-TODO) | ir_read.cpp:78 ✓+ | ✗ | ✗ | ✗ |
| IR remote (TX) | ir_remote.cpp:152 (Samsung) ~ | custom_ir.cpp:286 ✓+ | ✗ | ✗ | ✗ |
| nRF24 init | nrf24_hw.cpp:21 ✓ | nrf_common.cpp:29 ✓+ (multi-bus) | ✗ | ✗ | ✗ |
| nRF24 mousejack | nrf24_suite.cpp:359 ✓ | nrf_mousejack.cpp:934 ✓ | ✗ | ✗ | ✗ |
| nRF24 sniffer | nrf24_suite.cpp:186 ✓ | nrf_mousejack.cpp ✓ | ✗ | ✗ | ✗ |
| nRF24 spectrum | nrf24_suite.cpp:539 ✓ | nrf_spectrum.cpp:57 ✓ | ✗ | ✗ | ✗ |
| nRF24 BLE spam | nrf24_suite.cpp:449 ✓+ | ✗ | ✗ | ✗ | ✗ |
| nRF24 jammer + drone FHSS | nrf24_suite.cpp:658 ~ (no drone preset) | nrf_jammer.cpp:61 ✓+ (drone preset) | ✗ | ✗ | ✗ |
| LoRa SX1262 | lora_hw.cpp:106 ✓+ | LoRaRF.cpp:401 ✓ | ✗ | quiesce only ~ | ✗ |
| LoRa scan + GPS beacon | radio_lora.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| LoRa spectrum | lora_spectrum.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| LoRa chat (raw P2P) | ✗ | LoRaRF.cpp:401 ✓ | ✗ | ✗ | ✗ |
| Meshtastic LongFast listener | mesh/meshtastic_node.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| nRF52 (Adafruit Feather) | nrf52_hw.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| nRF52 BLE 5.0 / coded PHY | nrf52_suite.cpp:415 ✓+ | ✗ | ✗ | ✗ | ✗ |
| 802.15.4 Zigbee sniffer | nrf52_suite.cpp + c5_node zb_sniffer.c ✓+ | ✗ | ✗ | ✗ | ✗ |
| Scout & Strike dual-radio | nrf52_scout_strike.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| WiFi+BLE combo (deauth → provision capture) | nrf52_wifi_ble_combo.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| rf_finder thermometer | rf_finder.cpp:227 ✓+ | ✗ | ✗ | ✗ | ✗ |

### Net / Mesh

| Feature | POSEIDON | Bruce | Ghost | Porkchop | EvilM5 |
|---|---|---|---|---|---|
| LAN port scan | net_tools.cpp:39 ✓ | ✗ | wifi_manager.c:1633 ✓+ | ✗ | Evil-Cardputer-v1-5-2.ino case 37 ✓ |
| ARP recon | net_lanrecon.cpp:366 ✓ | ARPScanner.cpp ✓+ (wired) | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:13509 ✓ |
| ARP poison / netcut | ✗ | netcut.cpp:336 ✓+ | ✗ | ✗ | ✗ |
| DHCP starvation | net_dhcp.cpp:124 + saltyjack_dhcp_starve.cpp ✓ | DHCPStarvation.cpp ✓ (wired) | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:17296 ✓ |
| Rogue DHCP (STA+AP) | net_dhcp.cpp:413/414 + saltyjack_dhcp_rogue.cpp ✓ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:16888 ✓ |
| DHCPAttackAuto (auto pick) | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino case 51 ✓+ |
| Network Hijack combo | partial (net_dhcp:421) ~ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino case 50 ✓+ |
| Switch DNS AP↔STA bind | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:17985 ✓+ |
| Responder LLMNR | net_responder.cpp:176 + saltyjack_responder.cpp ✓+ (incl mDNS) | responder.cpp:113 ✓ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:21585 ✓ |
| Responder NBT-NS | net_responder.cpp ✓ | responder.cpp ✓ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:21581 ✓ |
| Responder mDNS poisoner | net_responder.cpp:194 ✓+ | ✗ | ✗ | ✗ | ✗ |
| NTLMv2 capture | net_responder.cpp + saltyjack_responder.cpp:619 ✓ | responder.cpp:113 ✓ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:4656 ✓ |
| NTLMv2 on-device crack | saltyjack_ntlm_crack.cpp:496 ✓ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:26615 ✓ |
| NTLM hash de-dup | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:27025 ✓+ |
| WPAD abuse + Autodiscover | net_wpad.cpp:274 + saltyjack_wpad.cpp:437 ✓ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:3897 ✓ |
| SSDP discovery scanner | net_ssdp.cpp:74 ✓+ | ✗ | ✗ | ✗ | ✗ |
| SSDP fake / poisoner (300 device) | net_attacks.cpp:735 (200 cap) ~ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:27437 ✓+ |
| UPnP NAT abuse | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino cases 77/78 ✓+ |
| CCTV / IP cam recon | net_cctv.cpp:542 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Reverse TCP tunnel | net_attacks.cpp:222 ✓ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:16656 ✓ |
| UART shell bridge | net_attacks.cpp:89 ✓ | ✗ | ✗ | ✗ | ✓ |
| Telnet honeypot | net_attacks.cpp:348 ✓+ | ✗ | ✗ | ✗ | ✗ |
| WiFi dead-drop (hidden AP + portal) | net_attacks.cpp:547 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Printer port-9100 | net_attacks.cpp:614 ~ | ✗ | printer.c (powerprinter) ✓+ | ✗ | ✗ |
| DIAL / Chromecast hijack | ✗ | ✗ | dial_manager.c ✓+ | ✗ | ✗ |
| TP-Link Kasa control | ✗ | ✗ | commandline.c:518 ✓+ | ✗ | ✗ |
| Captive cookie siphon | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:3895 ✓+ |
| PCAP for net attacks | ✗ | ✗ | vendor/pcap.c ✓+ | ✗ | Evil-Cardputer-v1-5-2.ino:10696 ✓ |
| Web admin / remote review | ✗ | ✗ | ap_manager.c:846 (REST) ✓+ | ✗ | Evil-Cardputer-v1-5-2.ino:535 (dashboard) ✓ |
| Wireguard / SSH / SOCKS4 / Telnet client | ✗ | LibSSH-ESP32 etc. ✓+ | ✗ | ✗ | ✓ (SSH only) |
| JS scripting (wifi/ble/badusb bindings) | ✗ | mquickjs ✓+ | ✗ | ✗ | EvilChat LLM ~ |
| Mesh ESP-NOW PigSync | mesh.cpp + c5_cmd ✓ | ✗ | ✗ | pigsync_client.cpp ✓+ (encrypted PMK+LMK) | ESP-NOW present ✓ |
| Meshtastic chat / nodes / page / position | mesh_chat.cpp etc ✓+ | ✗ | ✗ | ✗ | ✗ |
| Satellite tracker (SGP4) | satcom.cpp + feat_satcom.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |

### IR / GPS / Specials

| Feature | POSEIDON | Bruce | Ghost | Porkchop | EvilM5 |
|---|---|---|---|---|---|
| BadUSB USB-HID DuckyScript | badusb.cpp + extras ✓ | ducky_typer.cpp:454 ✓ | ✗ | ✗ | ✓ |
| BadUSB SD payload loader | ✗ (promised, unimplemented) | ✓ | ✗ | ✗ | ✓ |
| BadUSB OPSEC VID/PID spoof | ✗ | ✗ | ✗ | ✗ | ✗ |
| Drone RemoteID passive (ASTM F3411) | drone_remoteid.cpp:227 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Skyjack Parrot drone | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:27718 ✓+ |
| Surveillance hunter (Flock + Raven) | surveillance_hunter.cpp:264 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Defensive monitor multi-class | defensive_monitor.cpp:637 ✓+ | partial (deauth detect) ~ | partial (PineAP) ~ | ✗ | ✗ |
| Triton autonomous hunter | triton.cpp:1006 ✓+ | ✗ | ✗ | ✗ | ✗ |
| Trident PC bridge (USB-CDC framebuffer) | trident.cpp:209 ✓+ | ✗ | scripts/control_app ~ | ✗ | ✗ |
| Mimir drop-box C2 | mimir.cpp:553 ✓+ | ✗ | ✗ | ✗ | ✗ |
| GPS NMEA parser | gps.cpp:147 ✓ | ✓ | gps_manager.c ✓ | gps.cpp ✓ | ✓ |
| GPS-off OPSEC invariant | enforced ✓+ | n/a | n/a | n/a | n/a |
| IMSI catcher | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino case ✓+ |
| LDAP anonymous bind | ✗ | ✗ | ✗ | ✗ | Evil-Cardputer-v1-5-2.ino:30988 ✓+ |
| RFID/NFC stack | ✗ | rfid/* ✓+ | ✗ | ✗ | ✗ |
| QR code | ✗ | qrcode_menu.cpp ✓ | ✗ | ✗ | ✗ |
| U2F | ✗ | u2f.cpp ✓ | ✗ | ✗ | ✗ |
| EvilChat LLM | ✗ | ✗ | ✗ | ✗ | menu :290-291 ✓+ |

### System / UI

| Feature | POSEIDON | Bruce | Ghost | Porkchop | EvilM5 |
|---|---|---|---|---|---|
| Menu system | menu.cpp:935 (1379 LOC static) ~ | main_menu.cpp:38 ✓ | views/main_menu_screen.c ✓ | ui/menu.cpp ✓ | flat menuItems[]:233 ~ |
| Carousel renderer | menu_carousel.cpp ✓+ | ✗ | ✗ | ✗ | ✗ |
| Theme | theme.cpp (6 palettes) ✓+ | theme.cpp BruceTheme ✓ | rgb_manager.c ~ | display.cpp ✓ | INI on SD ~ |
| Screensaver pool | screensaver.cpp (10 painters) ✓+ | ✗ | ✗ | ✗ | ✗ |
| Splash | splash.cpp (Hokusai wave) ✓+ | splash.h/.cpp ✓ | views/splash_screen.c ✓ | display.cpp ✓ | ✓ |
| SFX | sfx.cpp ✓ | ✗ | ✗ | ✗ | ✓ |
| Status bar with radio badge | ui.cpp:152 ✓+ | ✗ | ✗ | display.cpp topBar ✓ | ✓ |
| Hat manager | hat_manager.cpp (stub) ~ | n/a | n/a | config.cpp pinmap ~ | ✗ |
| SD helper / multi-speed mount | sd_helper.cpp ✓+ | bruceConfig.sd_bus ~ | sd_card_manager.c ✓ | config.cpp ✓ | ✓ |
| Heap-pressure state machine | ✗ | ✗ | ✗ | heap_health.h + heap_gates.h + heap_policy.h ✓+ | ✗ |
| Reservation fence (pre-WiFi 80 KB) | ✗ | ✗ | ✗ | main.cpp:59-87 ✓+ | ✗ |
| Watermark persistence | ✗ | ✗ | ✗ | heap_health.h:50 ✓+ | ✗ |
| Knuth 50% fragmentation telemetry | ✗ | ✗ | ✗ | heap_health.h:38 ✓+ | ✗ |
| `canGrow(minFree,minFrag)` gate | ✗ | ✗ | ✗ | heap_gates.h:51 ✓+ | ✗ |
| Ban on `WiFi.mode(WIFI_OFF)` | violated ✗ | ✗ | ✗ | wifi_utils.cpp:227-229 ✓+ | n/a |
| WiFi+BLE init order recipe | partial ~ | ble_common.cpp:124 ✓ | n/a | network_recon.cpp:794-822 ✓+ | n/a |
| Boot-to-feature kiosk | ✗ | ✗ | ✗ | ✗ | caseToStartAtBoot ✓+ |
| Crash viewer (SD-backed) | autotest marker only ~ | ui/crash_viewer.cpp ✓+ | ✗ | ui/crash_viewer.cpp ✓+ | ✗ |
| Diagnostics menu | per-feature ad-hoc ~ | ✗ | ✗ | ui/diagnostics_menu.cpp ✓+ | ✗ |
| Achievements / XP / Unlockables | ✗ | ✗ | ✗ | core/xp.cpp ✓+ | ✗ |
| Avatar / Mood (Tamagotchi) | argus.cpp:175 (Triton-bound) ✓+ | ✗ | ✗ | piglet/ ✓ | ✗ |
| Stress test (no-RF heap stress) | ✗ | ✗ | ✗ | core/stress_test.cpp ✓+ | ✗ |
| Auto-deauth at boot | ✗ | ✗ | wifi_manager.c:2006 ✓ | ✗ | Evil-Cardputer-v1-5-2.ino case 24 ✓ |
| REST/SPA control surface | ✗ | ✗ | ap_manager.c:846 ✓+ | ✗ | inline dashboard ✓ |
| CLI shell (commandline) | serial_test (4 cmds) ~ | ✗ | commandline.c (33 cmds) ✓+ | ✗ | UART shell ✓ |
| Settings_manager unified | per-namespace NVS ~ | bruceConfig ✓ | settings_manager.c ✓+ | core/settings ✓ | INI ✓ |
| PSRAM NimBLE internal-only flag | ✗ | ✗ | ✗ | platformio.ini:27-28 ✓+ | ✗ |
| BACON / hide-seek game | ✗ | ✗ | ✗ | bacon.cpp ✓+ | ✗ |
| BOAR BROS exclusion list | ✗ | ✗ | ✗ | oink.h:172 ✓+ | ✗ |
| Adaptive DNH channel hopper | ✗ | ✗ | ✗ | donoham.cpp ✓+ | ✗ |

## Gaps we should close

Features marked ✗ in POSEIDON but ✓+ in any ref, sorted by user-value. Each maps to a Phase 3 backlog ticket.

1. **Bruce HID Exploit Engine (9 OS tactics)** — `BLE_Suite.cpp:462-825`. Entry: extend `ble_hid.cpp` keyboard with per-OS payload dispatch. Backlog: POS-AUDIT-101.
2. **Bruce WhisperPair multi-variant** (protocol-state-confusion + crypto-overflow) — `BLE_Suite.cpp:1004-1074`. Entry: extend `ble_whisperpair.cpp:425` probe to alternative payload sequences. Backlog: POS-AUDIT-102.
3. **Ghost DIAL/Chromecast/YouTube-Lounge hijack** — `dial_manager.c`. Entry: new `net_dial.cpp`. Backlog: POS-AUDIT-103.
4. **Ghost TP-Link Kasa smart-plug control** — `commandline.c:518`. Entry: new `net_kasa.cpp`. Backlog: POS-AUDIT-104.
5. **Bruce mquickjs JS interpreter + wifi/ble/badusb bindings** — `modules/bjs_interpreter/*`. Entry: new `js_runtime.cpp`; out-of-scope for v0.7 (large effort). Backlog: POS-AUDIT-105.
6. **Bruce Wireguard / SSH / SOCKS4 / Telnet client** — `LibSSH-ESP32`. Entry: new `net_ssh.cpp` / `net_wg.cpp`. Backlog: POS-AUDIT-106.
7. **Bruce RFID/NFC stack (PN532 + ChameleonUltra + Amiibo + EMV + SRIX + RFID125)** — `modules/rfid/*`. Entry: new `nfc/` subdir; requires hardware support eval first. Backlog: POS-AUDIT-107.
8. **Bruce W5500 wired LAN attacks** — `ethernet/*`. Entry: would need hat support; defer to hat eval. Backlog: POS-AUDIT-108.
9. **Evil-M5 Network Hijack combo** — orchestrate DHCP + DNS + WPAD chain. Entry: `saltyjack/saltyjack_hijack.cpp` wrapper. Backlog: POS-AUDIT-109.
10. **Evil-M5 DHCPAttackAuto (auto pick starve vs rogue)** — `case 51`. Entry: extend `net_dhcp.cpp`. Backlog: POS-AUDIT-110.
11. **Evil-M5 Switch DNS AP↔STA bind** — `:17985`. Entry: extend captive DNS in `net_responder.cpp`. Backlog: POS-AUDIT-111.
12. **Evil-M5 SSDP fake-300 poisoner** — `:27437`. Entry: extend `net_attacks.cpp:735` SSDP poisoner from cap 200 to 300. Backlog: POS-AUDIT-112.
13. **Evil-M5 UPnP NAT abuse (list mappings + add target NAT)** — cases 77/78. Entry: new `net_upnp_nat.cpp`. Backlog: POS-AUDIT-113.
14. **Evil-M5 cookie siphon endpoint** — `/siphon`, `/logcookie`. Entry: extend `wifi_portal.cpp` HTTP server. Backlog: POS-AUDIT-114.
15. **Evil-M5 NTLM hash de-dup pre-pass** — `CleanNTLMHashes`. Entry: `crypto_md4.cpp` helper or per-feature in `saltyjack_ntlm_crack.cpp`. Backlog: POS-AUDIT-115.
16. **Evil-M5 portal SD template loader (31 base + 27 community)** — `/evil/sites/*.html`. Entry: `wifi_portal.cpp` template enumerator + `streamFile`. Backlog: POS-AUDIT-116.
17. **Evil-M5 boot-to-feature (kiosk drop-box mode)** — `caseToStartAtBoot`. Entry: NVS `boot_feature` + countdown UI. Backlog: POS-AUDIT-117.
18. **PORKCHOP heap-pressure state machine** (`heap_policy.h` 47 thresholds + 4-level state + EMA-smoothed display) — `heap_health.h`/`heap_gates.h`/`heap_policy.h`. Entry: new `core/heap_policy.{cpp,h}`. Backlog: POS-AUDIT-118.
19. **PORKCHOP Reservation Fence** (80 KB malloc-then-free pre-WiFi) — `main.cpp:59-87`. Entry: `radio.cpp::wifi_lean_sta_init` prologue. Backlog: POS-AUDIT-119.
20. **PORKCHOP watermark persistence to SD** — `heap_health.h:50`. Entry: `/poseidon/heapwatermark.log`. Backlog: POS-AUDIT-120.
21. **PORKCHOP `canGrow(minFree,minFrag)` vector-growth gate** — `heap_gates.h:51`. Entry: helper for `g_wdr_aps`, `triton` deauth list. Backlog: POS-AUDIT-121.
22. **PORKCHOP BOAR BROS wardrive exclusion list (50-entry, SD-persist)** — `oink.h:172`. Entry: `wifi_wardrive.cpp`. Backlog: POS-AUDIT-122.
23. **PORKCHOP adaptive 4-state DNH channel hopper** — `donoham.cpp`. Entry: extend Triton's channel-pick logic. Backlog: POS-AUDIT-123.
24. **PORKCHOP NimBLE internal-only allocation cflag** — `platformio.ini:27-28`. Entry: `platformio.ini build_flags`. Backlog: POS-AUDIT-124.
25. **PORKCHOP crash viewer (SD-backed coredump reader)** — `ui/crash_viewer.cpp`. Entry: new `system_tools.cpp` submenu. Backlog: POS-AUDIT-125.
26. **PORKCHOP first-class diagnostics menu** (heap / frag / WiFi / C5 / hat) — `ui/diagnostics_menu.cpp`. Entry: new System submenu. Backlog: POS-AUDIT-126.
27. **PORKCHOP / Bruce hidden-SSID reveal via broadcast deauth** — `spectrum.cpp`. Entry: extend `wifi_spectrum.cpp` with reveal hotkey. Backlog: POS-AUDIT-127.
28. **Ghost 802.11ax HE Capabilities IE (Wi-Fi 6 spoofing) in beacons** — `wifi_manager.c:2153`. Entry: extend `wifi_beacon_spam.cpp:146` beacon builder. Backlog: POS-AUDIT-128.
29. **Ghost AP-list beacon-spam mode (rebroadcast scanned APs)** — `wifi_manager_broadcast_ap -l`. Entry: extend `wifi_beacon_spam.cpp` to source SSIDs from `g_wdr_aps[]`. Backlog: POS-AUDIT-129.
30. **Ghost Power Printer mass-print** — `printer.c`. Entry: extend `net_attacks.cpp:614` `feat_printer`. Backlog: POS-AUDIT-130.
31. **Ghost PineAP rogue-AP detection** — `callbacks.c:174`. Entry: extend `defensive_monitor.cpp`. Backlog: POS-AUDIT-131.
32. **Ghost Pwnagotchi presence detection** — `callbacks.c:484/626`. Entry: extend `defensive_monitor.cpp` or `wifi_scan.cpp`. Backlog: POS-AUDIT-132.
33. **Ghost BLE skimmer detection** — `ble_manager.c:746`. Entry: extend `defensive_monitor.cpp` BLE side. Backlog: POS-AUDIT-133.
34. **Ghost BLE wardrive (WiGLE CSV with GPS)** — `callbacks.c:769`. Entry: new feature reusing wardrive infra. Backlog: POS-AUDIT-134.
35. **Ghost BLE pcap writer** — `vendor/pcap.c PCAP_CAPTURE_BLUETOOTH`. Entry: helper unit. Backlog: POS-AUDIT-135.
36. **Ghost REST + embedded SPA control surface** — `ap_manager.c:846` + 3988-line embedded HTML. Entry: System → Remote submenu; LARGE. Backlog: POS-AUDIT-136.
37. **Ghost EAPOL→PCAP output alongside hashcat 22000** — `callbacks.c:639`. Entry: `wifi_pmkid.cpp` add per-frame PCAP write in promisc_cb. Backlog: POS-AUDIT-137.
38. **Bruce nRF24 drone FHSS jammer profile** — `nrf_jammer.cpp:61`. Entry: add preset to `nrf24_suite.cpp:658`. Backlog: POS-AUDIT-138.
39. **Bruce IR jammer (multi-protocol noise)** — `ir_jammer.cpp:698`. Entry: new `ir_jammer.cpp`. Backlog: POS-AUDIT-139.
40. **Bruce IR clone with RX capture** — `ir_read.cpp:78`. Entry: extend `ir_clone.cpp` (already TODOd at line 11) with RX + .ir SD store. Backlog: POS-AUDIT-140.
41. **Bruce sub-GHz listen (pulse-decode passthrough audio)** — `rf_listen.cpp:26`. Entry: new `subghz_listen.cpp`. Backlog: POS-AUDIT-141.
42. **mDNS poisoner (udp/5353)** — ABSENT in both Evil-M5 and POSEIDON. Entry: extend `net_responder.cpp:194` (already listens for mDNS — just needs reply path). Backlog: POS-AUDIT-142.
43. **Bruce Ninebot scooter BLE unlock** — `ble_ninebot.cpp:111`. Entry: new `ble_ninebot.cpp`. Backlog: POS-AUDIT-143.
44. **Evil-M5 Skyjack Parrot drone hunter** — `case 72`. Entry: extend `drone_remoteid.cpp` or new `drone_skyjack.cpp`. Backlog: POS-AUDIT-144.
45. **Evil-M5 IMSI catcher (opportunistic)** — `IMSI-catched.txt`. Entry: out-of-scope without LTE hardware; defer. Backlog: POS-AUDIT-145.
46. **Evil-M5 LDAP anonymous bind enum** — `:30988`. Entry: `net_ldap.cpp`. Backlog: POS-AUDIT-146.
47. **Evil-M5 web admin / remote review dashboard** — port 80 dashboard. Entry: System → Remote submenu (overlaps POS-AUDIT-136). Backlog: POS-AUDIT-147.
48. **Bruce WiGLE upload** — `wigle.h`. Entry: `wifi_wardrive.cpp` add WiFi-side uploader. Backlog: POS-AUDIT-148.
49. **PORKCHOP wpasec upload** — `web/wpasec.cpp`. Entry: `wifi_pmkid.cpp` add uploader. Backlog: POS-AUDIT-149.
50. **BadUSB SD `/poseidon/ducky/*.txt` payload loader (promised, unimplemented)** — `badusb.cpp:240`. Entry: SD enumerator + path-traversal sanitiser. Backlog: POS-AUDIT-150.

## Leads we should keep

Features marked ✓+ in POSEIDON. Protect these during refactor:

- **Triton autonomous handshake hunter** (`triton.cpp:1006`) — 4-mode FSM + RL channel weighting + Argus mood. No peer has anything close. Protect: state machine, RL persistence, mood transitions, cooperative-tick architecture. Refactor work (D16) must preserve all four mode semantics.
- **BLE WhisperPair CVE-2025-36911 with SD-loaded pubkeys** (`ble_whisperpair.cpp:615`) — POSEIDON has SD-side anti-spoof keys (Bruce uses compiled fastpair_models). Protect: SD key load path, ECDH+AES crypto, BR/EDR MAC lift, CSV verdict log.
- **BLE finder/findmy/karma/clone/MITM/sourapple** — TX-side suite no other firmware has. Protect: cooperative ticks, random-MAC discipline, per-feature scan-stop on entry.
- **Surveillance hunter (Flock + Raven)** (`surveillance_hunter.cpp:264`) — Sig DB + deferred queue + JSONL+CSV. Protect: deferred-queue pattern (drone_remoteid should adopt it too).
- **Defensive monitor 7-class** (`defensive_monitor.cpp:637`) — Time-sliced WiFi promisc + NimBLE scan with multi-class detector. Protect: detector class set; refactor MUST NOT remove classes during D15 fix.
- **Triton's Argus gotchi mood sprite system** (`argus.cpp`) — 48×48 RGB565 with 10 moods + procedural overlays. Protect: SRAM cache mechanism (TX-cache invariant), retry on `RADIO_BLE` switch.
- **Sub-GHz signal library + jam-detect + spectrum-bar/waterfall/oscope** — Three POSEIDON-only sub-GHz features. Protect: signals_data.h baked DB; jam-detect 10s warmup + 15 dBm trigger + siren; spectrum modes.
- **LoRa SX1262 with PI4IOE5V6408 direct I²C antenna switch** (`lora_hw.cpp:106`) — Bypasses `M5.getIOExpander` LoadProhibited panic. Protect: direct I²C 0x43 path, errata 0x8B5 + boostedGain init.
- **Meshtastic LongFast leaf node** (`meshtastic_node.cpp` 591 LOC) — AES-CTR + hand-written protobuf + roster + position. Protect: protocol decoder; refactor (D7 split) must preserve crypto + protobuf surface.
- **Drone RemoteID passive ASTM F3411-22a 0xFFFA scanner** (`drone_remoteid.cpp:227`) — JSONL + observer GPS, gated correctly on `g.valid`. Protect: GPS-off gate; SD log under deferred queue after D14 fix.
- **Satcom tracker with SGP4 + baked TLE fallback** (`satcom.cpp` + `feat_satcom.cpp:222`) — 14 NORAD favorites, polar skyplot, pass predict. Protect: SD cache, baked fallback.
- **Screensaver pool + ambient layer + 6-palette theme system** — 10 painters, per-theme ambient, NVS-backed live preview. Protect: shuffle mode, pick-without-commit pattern.
- **Hat manager probe surface** (`hat_manager.h`) — Even though `detect()` is stubbed (D04-adjacent), the header defines `pre_init`/`post_init`/`park_*` statics; useful to actually implement. Protect: API shape (or delete entirely per sys-013).
- **CC1101 RMT TX/RX precision** (`cc1101_rmt.cpp`) — IDF 5.5 new-API at 1 MHz with 3 µs filter. Microsecond-accurate vs Bruce's bit-bang loop. Protect: per-call alloc/free of channels (lets WiFi/BLE reclaim RAM).
- **C5 satellite v3 protocol + ESP-NOW transport** — Pure-C IDF firmware with packed structs, magic, version, types. Protect: wire compat across S3/C5 sides; bump to v4 (not v3 mutation) if new commands land.

## Divergent approaches worth re-examining

Features marked ~ where divergence has a real tradeoff:

1. **Sanity-check bypass** — POSEIDON uses linker stub (`wifi_sanity_override.cpp:33`) returning 0 always; Bruce uses per-call `esp_wifi_80211_tx(..., false)` flag; PORKCHOP uses `-Wl,-zmuldefs` linker flag. POSEIDON's is more robust on stock blobs but conflicts with selective TX (no per-frame flag). Recommendation: keep linker-stub for POSEIDON's broad raw-TX use, but verify `-zmuldefs` is in `platformio.ini` per memory invariant (wifi-035 backlog).

2. **Cooperative single-loop vs FreeRTOS-task-per-feature** — PORKCHOP runs everything from `update()` ticks (no `xTaskCreate` for attacks). POSEIDON spawns ~12 background tasks (wardrive, PMKID, global-clients, spectrum, beacon-spam, deauth-extras, targeted-deauth, scan-task, gps, ir-watchdog, serial-test, defensive_monitor). Tradeoff: tasks allow CPU concurrency but compound rc=-1 silent-fail under low heap (NimBLE init eats 30 KB) and force every feature exit to dance the teardown. Recommendation: long-term, migrate to PORKCHOP-style cooperative ticks where feature semantics permit (Triton already did). Out of scope for v0.7 across-the-board.

3. **Static menu tree vs dynamic registry** — POSEIDON has both: the static `MENU_*` tree in `menu.cpp` (used at runtime) AND the unused `menu_registry` infrastructure. Bruce dispatches feature classes; Evil-M5 has a flat `menuItems[]` switch-case. Recommendation: D04 (POS-AUDIT-004) decides — delete the registry OR migrate the static tree to use it. The registry path eliminates the "edit 3 places per feature" friction.

4. **Evil portal: per-brand inline templates vs URL-proxied vs SD-loadable** — POSEIDON ships 16 inline HTML templates (works offline, rebuild-to-update). Ghost URL-proxies external HTML (operator hosts page; ops-light, network-required). Evil-M5 loads from SD `/evil/sites/` (58 templates, drop-in updates). POSEIDON's offline guarantee is a feature for drop-box deployment; the SD-loadable mode (POS-AUDIT-116) is a strict addition rather than a replacement. Recommendation: keep inline as fallback, add SD enumeration.

5. **Sub-GHz pulse engine: RMT (POSEIDON) vs bit-bang (Bruce SmartRC fork)** — POSEIDON uses IDF 5.5 RMT TX/RX at 1 MHz resolution. Bruce inherits SmartRC's `digitalWrite` loop. POSEIDON is more accurate but allocates RMT channels per call (overhead). Recommendation: keep RMT; per-session pulse-buffer reuse cuts the alloc cost (rf-opt).

6. **Sub-GHz replay file picker: root-only (POSEIDON) vs subdir browser (broadcast picker)** — `subghz_replay.cpp:67` only iterates `/poseidon/` root, missing recordings under `/poseidon/signals/custom/` written by record/scan. `subghz_broadcast.cpp` has the right pattern. Recommendation: rf-007 backlog — descend into subdirs OR pivot to broadcast's picker.

7. **WiFi+BLE teardown order: explicit PORKCHOP sequence vs POSEIDON `radio_switch` chain** — PORKCHOP enforces stop scan/adv → `NimBLEDevice::deinit(true)` → `delay(100)` → `yield()` → `delay(50)` → `WiFi.mode(WIFI_STA)`. POSEIDON's `teardown_current()` does deinit then `delay(100)` but no yield, and the BLE-up path does NOT pre-yield WiFi-side bufs. Recommendation: adopt PORKCHOP's full recipe in `radio.cpp` (POS-AUDIT-008 fix).

8. **Mesh ESP-NOW: open broadcast (POSEIDON PigSync) vs PMK+LMK encrypted (PORKCHOP pigsync)** — POSEIDON broadcasts HELLO unencrypted with GPS-if-fixed. PORKCHOP uses encrypted bidirectional capture sync. Tradeoff: POSEIDON's plaintext is simpler ops but leaks coordinator presence + GPS. Recommendation: backlog the encryption upgrade if PigSync ever ships beyond single-user pairing.

9. **C5 ESP-NOW HELLO cadence (5 s unconditional) vs PigSync backoff after pairing** — `c5_node/main/main.c:69-85` broadcasts every 5 s forever. PORKCHOP backs off to 30 s once paired. Tradeoff: 5 s gives faster reattach but constant radio traffic. Recommendation: backoff to 30 s when `c5_peer_count > 0`.

10. **GPS poller architecture: always-on at boot (POSEIDON) vs opt-in** — `main.cpp:32-38,143-144` spawns the GPS task at boot regardless of user pref. OPSEC invariant says GPS should be OFF unless toggled (`feedback_poseidon_gps_off_by_default.md`). Recommendation: gate `gps_begin()` behind NVS `gps_enabled` flag, default OFF (POS-AUDIT sys-015).

11. **Defensive monitor radio time-slice (POSEIDON 3 s/2 s WiFi/BLE cycle) vs single-radio focus** — Re-initing NimBLE every 5 s grows TLSF fragmentation. PORKCHOP avoids cycling at all. Recommendation: widen windows to ≥30 s or switch to coexist pattern that doesn't re-init (POS-AUDIT-015 fix).

12. **Mimir/Trident CDC exclusivity gate** — BadUSB checks both `g_mimir_cdc_active` and `g_trident_cdc_active` before claiming USB. But neither Mimir nor Trident check each other on entry. Tradeoff: BadUSB protects itself; Mimir+Trident concurrent entry corrupts both flag states. Recommendation: cross-check both flags on entry in Mimir + Trident (mim-002, trd-002).
