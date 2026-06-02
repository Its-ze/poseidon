# Bruce Reference Map

Commit: 00eab49
Cloned: 2026-06-01
Repo: https://github.com/bmorcelli/Bruce

## Feature index

| Feature | File:line | Notes |
|---------|-----------|-------|
| WiFi scan | src/core/wifi/wifi_common.cpp:183 | Arduino `WiFi.scanNetworks()`, hidden toggle via `showHiddenNetworks` |
| WiFi deauth | src/modules/wifi/deauther.cpp:143 | `stationDeauth()`, raw 802.11 TX on STA iface; refs GANESH-ICMC esp32-deauther |
| WiFi beacon spam | src/modules/wifi/wifi_atks.cpp:977 | `beaconAttack()`; lists at :905 `beaconSpamList`, :944 `beaconSpamSingle` |
| WiFi probe | src/modules/wifi/karma_attack.cpp:909 | `isProbeRequestWithSSID()` — folded into Karma, no standalone probe spam |
| WiFi evil twin/portal | src/modules/wifi/evil_portal.cpp:61 | `EvilPortal::setup()`; AsyncWebServer + AP; gateway IP in config.h:73 |
| WiFi wardrive | src/modules/gps/wardriving.cpp:451 | Active `WiFi.scanNetworks()` + GPS; wigle upload `src/modules/gps/wigle.h` |
| WiFi PMKID | src/modules/wifi/karma_attack.cpp:2326 | Counter inside Karma, captured via promisc EAPOL frames |
| WiFi sanity override | src/modules/wifi/wifi_atks.cpp:157 | `esp_wifi_80211_tx(WIFI_IF_AP, ..., false)` — passes `false` for en_sys_seq to skip CSI bypass check |
| WiFi sniff/clients | src/modules/wifi/sniffer.cpp:696 | `sniffer_set_mode()`; promisc + PCAP writer task at :670 |
| WiFi handshake | src/modules/wifi/wifi_recover.cpp:235 | Pass-recovery: parses EAPOL from saved PCAP, no live cracker |
| BLE scan | src/modules/ble/ble_common.cpp:169 | `ble_scan()`; setup at :124, NimBLE 2.x guarded `NIMBLE_V2_PLUS` |
| BLE spam | src/modules/ble/ble_spam.cpp:482 | `spamMenu()`; types Microsoft/SourApple/AppleJuice/Samsung/Google at :41 |
| BLE flood (DoS) | src/modules/ble/BLE_Suite.cpp:2725 | `DoSAttackServiceClass::connectionFlood(target)`; multi at :2242 |
| BLE sour apple | src/modules/ble/ble_spam.cpp:156 | enum branch in `executeSpam`; trigger at :397 |
| BLE finder | — | not present (no beacon-distance finder) |
| BLE findmy | — | not present |
| BLE karma | — | WiFi-only Karma; no BLE karma variant |
| BLE whisperpair | src/modules/ble/BLE_Suite.cpp:1115 | `WhisperPairExploit::execute()`; FastPair KBP characteristic attack; crypto at fastpair_crypto.cpp |
| BLE clone | src/modules/ble/BLE_Suite.cpp:2091 | `AuthBypassEngine` — spoofs target name+addr, no full advertisement clone |
| BLE GATT | src/modules/ble/BLE_Suite.cpp:214 | `BLEAttackManager::connectToDevice()`; HID service discovery throughout |
| BLE HID | src/modules/ble/BLE_Suite.cpp:921 | `HIDExploitEngine::executeHIDInjection()`; 9 tactics at :462–:825 |
| BLE BlueDucky | src/modules/badusb_ble/ducky_typer.cpp:454 | `ducky_setup(hid, ble=true)`; menu entry "Bad BLE" BleMenu.cpp:36 |
| BLE MITM relay | — | not present |
| BLE Ninebot | src/modules/ble/ble_ninebot.cpp:111 | scooter unlock exploit |
| Sub-GHz scan | src/modules/rf/rf_scan.cpp:13 | `RFScan::setup()` + `fast_scan()` at :86, RCSwitch decode :170, raw :213 |
| Sub-GHz replay | src/modules/rf/rf_send.cpp:137 | `loopEmulate(RfCodes&)`; .sub TX at :276 `txSubFile` |
| Sub-GHz broadcast | src/modules/rf/rf_send.cpp:338 | `sendRfCommand` — generic emit, no separate "broadcast" mode |
| Sub-GHz jammer | src/modules/rf/rf_jammer.cpp:212 | `RFJammer::run_full_jammer()`; intermittent at :278 |
| Sub-GHz jam detect | — | not present |
| Sub-GHz bruteforce | src/modules/rf/rf_bruteforce.cpp:101 | `rf_bruteforce()` |
| Sub-GHz spectrum | src/modules/rf/rf_spectrum.cpp:28 | `rf_spectrum()`; RSSI :148, square :91, waterfall rf_waterfall.cpp:18 |
| Sub-GHz record | src/modules/rf/record.cpp:287 | `rf_raw_record()` |
| Sub-GHz signal library | — | uses Flipper-style `.sub` files on SD/LittleFS, no indexed library UI |
| Sub-GHz listen | src/modules/rf/rf_listen.cpp:26 | `rf_listen()` — pulse-based decode for audio passthrough |
| CC1101 init | src/modules/rf/rf_utils.cpp:356 | `initCC1101once(SPIClass*)`; uses `setSPIinstance()` + `setSpiPin()` from SmartRC fork |
| CC1101 RMT TX/RX | src/modules/rf/rf_send.cpp:579 / rf_send.cpp:544 | RCSwitch RAW timings; no RMT — bit-bang loop via `digitalWrite` in CC1101 driver |
| nRF24 init | src/modules/NRF24/nrf_common.cpp:29 | `nrf_start(NRF24_MODE)`; multi-bus negotiation w/ TFT/SD/CC1101 |
| nRF24 mousejack | src/modules/NRF24/nrf_mousejack.cpp:934 | `nrf_mousejack()`; promiscuous + HID injection |
| nRF24 sniff/jammer | src/modules/NRF24/nrf_jammer.cpp:755 / :808 / :930 | full/channel/hopper variants; drone FHSS profile at :61 |
| nRF24 spectrum | src/modules/NRF24/nrf_spectrum.cpp:57 | `nrf_spectrum()` |
| IR tvbgone | src/modules/ir/TV-B-Gone.cpp:149 | `StartTvBGone()`; codes in `WORLD_IR_CODES.h` |
| IR clone (read) | src/modules/ir/ir_read.cpp:78 | `IrRead::setup()`; raw decode + save |
| IR remote (send) | src/modules/ir/custom_ir.cpp:286 | `sendIRCommand()`; NEC/RC5/RC6/Samsung helpers :314– |
| IR jammer | src/modules/ir/ir_jammer.cpp:698 | `startIrJammer()` |
| Mesh / LoRa | src/modules/lora/LoRaRF.cpp:401 | `lorachat()`; RadioLib backend; init :119; no mesh/position/nodes/page |
| Net DHCP starvation | src/modules/ethernet/DHCPStarvation.cpp | W5500 wired class |
| Net ARP recon | src/modules/ethernet/ARPScanner.cpp | wired LAN host discovery |
| Net ARP spoof/poison | src/modules/ethernet/ARPoisoner.cpp / ARPSpoofer.cpp | wired |
| Net MAC flood | src/modules/ethernet/MACFlooding.cpp | wired |
| Net responder | src/modules/wifi/responder.cpp:113 | `buildNTLMType2Msg`; NetBIOS+SMB NTLMv2 capture, extract at :237 |
| Net SSDP/WPAD | — | not present |
| Net CCTV | — | not present |
| Net netcut | src/modules/wifi/netcut.cpp:336 | `netcutPoisonDevice()` — ARP poisoning over LwIP linkoutput |
| Net portal HTTP | src/modules/wifi/evil_portal.cpp:147 | `setupRoutes()` — AsyncWebServer captive |
| Drone RemoteID | — | only nRF24 drone FHSS jammer profile (nrf_jammer.cpp:61), no RemoteID parsing |
| Surveillance hunter | — | not present |
| BadUSB (USB HID) | src/modules/badusb_ble/ducky_typer.cpp:454 | `ducky_setup(hid, ble=false)`; USB path via `USB_as_HID` build flag |
| Satellite tracker / Satcom | — | not present |
| System main | src/main.cpp:1 | 561 lines; setup/loop boots `MainMenu` |
| System menu | src/core/main_menu.cpp:38 | `MainMenu::begin()` |
| System theme | src/core/theme.cpp:11 | `BruceTheme` struct-based, themeable colors |
| System input | src/core/mykeyboard.cpp | unified input layer |
| Screensaver | — | not present |
| Hat manager | — | not present (pin map per-board via boards/*/pins_arduino.h) |
| Pwnagotchi friend | src/modules/pwnagotchi/pwngrid.cpp:310 | promiscuous filter for grid frames; sniffer cb :235 |
| RFID/NFC | src/modules/rfid/* | PN532, ChameleonUltra, Amiibo, EMV, SRIX, RFID125 |
| QR code | src/modules/others/qrcode_menu.cpp | |
| U2F | src/modules/others/u2f.cpp | |
| JS interpreter | src/modules/bjs_interpreter/* | mquickjs; `wifi_js.cpp`, `ble_js.cpp`, `badusb_js.cpp` bindings |

## Hardware bring-up patterns

- CC1101 SPI setup: src/modules/rf/rf_utils.cpp:356 `initCC1101once(SPIClass*)` — passes ptr to `ELECHOUSE_cc1101.setSPIinstance()` (forked SmartRC); pins via `setSpiPin()` from `bruceConfigPins.CC1101_bus`. NRF24/CC1101/SD bus sharing negotiated dynamically in `nrf_common.cpp:49`. No bit-banged MISO in Reset (uses driver default).
- nRF24 init: src/modules/NRF24/nrf_common.cpp:29 `nrf_start()`; manual `pinMode` + CS-high parking at :44; runtime SPI bus selection (TFT/SD/CC1101 share); pipe address table inside `nrf_mousejack.cpp` (not common). Logitech promiscuous addrs at `nrf_mousejack.cpp:651`.
- WiFi raw frame TX: src/modules/wifi/wifi_atks.cpp:157 `esp_wifi_80211_tx(WIFI_IF_AP, buf, len, false)` — `en_sys_seq=false` is the sanity-bypass technique; rc unused for beacon/deauth, checked in Karma at karma_attack.cpp:64. Uses Arduino `WiFi.mode(WIFI_AP)` then raw TX (does NOT use the IDF-init recipe POSEIDON uses for AP). No special lib patch; relies on bmorcelli's custom `framework-arduinoespressif32-libs` zip (see lib pinning).
- BLE init: NimBLE-Arduino 2.5 (`h2zero/NimBLE-Arduino@2.5`); init order has explicit `wifiDisconnect()` + `stopBLEStack()` teardown in `ble_common.cpp:124` when `FORCE_RADIO_TEARDOWN_ON_SWITCH`. Compile-time `NIMBLE_V2_PLUS` switches API. NO xTaskCreate for BLE — all scans run blocking with `pBLEScan->getResults(timeout)`, i.e. cooperative.

## Build / lib pinning notes

- `platformio.ini` flags: `-Os -flto -ffunction-sections -fdata-sections -Wl,--gc-sections`, no `-zmuldefs`. CDC mode is per-board (`USB_as_HID=1` flag in board ini). Custom platform: pioarduino 55.03.36 (Arduino 3.3.6 / IDF 5.5).
- Custom `framework-arduinoespressif32-libs` from bmorcelli/esp32-arduino-lib-builder (bruce_esp32-arduino-libs-20260123-153546.zip) — this is where raw 80211 TX, mbedtls extras (ECDH/AES/CTR_DRBG for WhisperPair) are baked in.
- Lib pins: NimBLE-Arduino@2.5, RF24@1.4.11, RadioLib@^7.4.0, IRremoteESP8266 (bmorcelli fork), SmartRC-CC1101-Driver-Lib (bmorcelli fork), rc-switch (bmorcelli fork), mquickjs@0.0.6, Adafruit BusIO (emericklaw bruce fork 1.17.2), Adafruit PN532 (bruce fork 1.3.3), ArduinoJson, ESPAsyncWebServer (ESP32Async), FastLED@^3.10.3, LibSSH-ESP32.
- Partition (8MB Cardputer): `custom_8Mb.csv` — single app slot 0x4E0000 + spiffs 0x300000 (LittleFS via `board_build.filesystem=littlefs`). No OTA pair. Coredump 64KB.
- `CONFIG_ASYNC_TCP_STACK_SIZE=4096`, `CONFIG_ASYNC_TCP_RUNNING_CORE=1`, WDT disabled for AsyncTCP.

## Anything Bruce does that POSEIDON does NOT appear to do (high-level only)

- Full BLE attack engine: HIDExploitEngine (9 OS-specific tactics), AuthBypassEngine spoof, HFP/CVE-2025-36911 chain, DoS connectionFlood multi-target.
- WhisperPair FastPair KBP crypto attack (mbedtls ECDH/AES baked into custom IDF libs).
- Wired LAN attack family on W5500: ARP scan/spoof/poison, DHCP starvation, MAC flood (separate from SaltyJack WiFi class).
- NTLMv2 responder over WiFi (NetBIOS challenge + SMB hash extraction).
- mquickjs JS scripting engine with WiFi/BLE/BadUSB bindings.
- LoRa chat over RadioLib (not Meshtastic protocol — point-to-point text).
- nRF24 jammer with per-profile (BT/Drone/RC/Video) tuned hop strategies.
- Pwnagotchi-grid (pwngrid) friend-mode advertise + handshake sniffer.
- RFID/NFC stack: PN532, ChameleonUltra, Amiibolink, EMV reader, SRIX, RFID125, MFRC522.
- Custom IDF lib builder zip — Bruce ships its own Arduino-lib build to enable raw 80211 + extra mbedtls; POSEIDON uses stock pioarduino.
- Wireguard client (sayacom WG fork) + SSH (LibSSH-ESP32) + SOCKS4 proxy + Telnet client built-in.
- Wigle.net upload + WDGoWars CSV connector for wardriving.
- Bruteforce engine for sub-GHz remotes.
- Ninebot scooter BLE unlock.
- IR jammer (multi-protocol noise floor).
