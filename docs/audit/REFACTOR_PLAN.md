# POSEIDON v0.6.1 → v0.7 Refactor Plan

Phase ordering reflects dependency chains: hygiene first, then hardware safety (so refactors are not chasing moving targets), then module splits, then capability gaps from cross-ref, then new capabilities not driven by audit.

## Phase 0 — Hygiene

Scope: build/lib pinning verification, partition cleanup, dead-code removal, .gitignore audit, doc/map errata fixes. No behaviour changes intended; reduce surface area before substantive work begins.

Specific items:

- **Dead code removal** — Delete `src/menu_registry.cpp` + `src/menu_registry.h` (~280 LOC, ~110 KB BSS reservation) OR migrate static `MENU_*` tree to actually use it (per POS-AUDIT-004; decision required before Phase 2).
- **`hat_manager.cpp:27-36` stub** — Either implement probe sequences (SX1262 v05 read, CC1101 PARTNUM=0x80, NRF24 STATUS register, etc.) OR delete the class and remove `HatType` references from the inventory (POS-AUDIT sys-013).
- **Map errata fixes** — `_audit/maps/poseidon.md` line 28 says splash is a "metallic trident sprite, title bloom, scanline sweep"; actual implementation in `splash.cpp:55-148` is a Hokusai Great Wave fade (POS-AUDIT sys-022). Update line 28. Also fix SD CS reference: the map says SD CS=10 (internal) while PORKCHOP/some comments reference 12 — confirm the correct pin and reconcile across `sd_helper.cpp`, `cc1101_hw.cpp:12`, `nrf24_hw.cpp:11`, `lora_hw.cpp:118-124`.
- **`hat_manager.h` claim audit** — Header advertises probe-based detection; if not implementing, delete the static helpers `park_for_lora` / `park_for_cc1101` / `park_for_nrf24` (no callers).
- **`subghz_signals_data.h` (761 LOC)** — Move to `.cpp` (TU-internal) with a small public extern struct in the header (rf-023). Prevents future ODR risk.
- **`ir_extras_data.h` (1028 LOC monolith)** — Split into `ir_codes_tvs.h`, `ir_codes_projector_soundbar.h`, `ir_codes_ac.h`, `ir_pranks.cpp` (real .cpp not inline header) per ir-003 / rf-5. Forward decls of `blast_raw`, `send_samsung`, `send_lg`, `send_sony12`, `delay`, `millis` at file scope (lines 811-928) leak into any TU that includes this header — ODR-fragile.
- **`badusb_extras.h` (1389 LOC)** — Split per-OS into `badusb_win.h`, `badusb_mac.h`, `badusb_linux.h`, `badusb_android.h`, `badusb_chromeos.h`.
- **`saltyjack_splash.h` (2709 LOC baked 240×135 RGB565)** — Verify partition headroom; consider moving to LittleFS or compressed flash blob.
- **Lib pin justification doc** — Write a short `docs/lib-pins.md` noting why each lib pin (NimBLE 2.x, RF24 1.4.11, RadioLib ≥7.4.0, IRremoteESP8266 bmorcelli fork, SmartRC bmorcelli fork, rc-switch bmorcelli fork, AsyncWebServer ESP32Async, etc.) is held at current versions; Bruce-libs custom IDF zip note that upgrades require on-device regression sweep (per cross-ref).
- **`.gitignore` audit** — Verify `/_audit/` map/findings directories aren't tracked if they're generated; verify `.pio/`, `.vscode/` rules; verify no NVS dumps or credential files.
- **`version.h:19-20`** — Add `static` to `inline const char *poseidon_version()` and `poseidon_build_date()` to match Bruce's pattern (sys-027).
- **`-Wl,-zmuldefs` confirm** — Per memory invariant + wifi-035, verify the flag is in `platformio.ini build_flags`.
- **`splash.cpp` map fix** + comment audit — Already covered in errata above.
- **`nrf52_hw.cpp:24-26,46`** — Comment drift: header says UART1, code uses UART2 (rf-012). Fix comments.

**Dependencies:** none.

**Exit criteria:** clean build with no warnings; `menu_registry` decision applied (delete or activate); `hat_manager` decision applied; all map errata fixed; `subghz_signals_data.h` and `ir_extras_data.h` split; lib-pin justification documented; `.gitignore` validated; `version.h` static-inline; `-zmuldefs` confirmed in build_flags.

## Phase 1 — Hardware safety

Scope: SPI bus ownership corrections, pin parking gates, IR polarity fix, ESP-NOW channel locks, GPS UART pin re-arming on cc1101 failure paths, radio_switch state cleanup on every feature exit. Atomic per-defect. All CRIT and HIGH hardware/coexistence tickets land here.

Specific items (pulled from audit top-20 + supporting tickets):

- **IR LED polarity fix (D01 / POS-AUDIT-001)** — Flip all `digitalWrite(44, LOW)` off-state writes to HIGH in `main.cpp` (boot, watchdog, IR-feature exit), `menu.cpp:1323,1357`, `menu_carousel.cpp:377,391-415`, `ir_remote.cpp:110,127,149,196`, `ir_tvbgone.cpp:122,186`, `ir_clone.cpp:59,67-69,220,341`. Verify with phone-camera at boot.
- **`subghz_jam_detect.cpp` exit paths (D02 / POS-AUDIT-002)** — `radio_switch(RADIO_NONE)` on every early-return; verify GPS re-arms.
- **`wifi_ciw.cpp` AP rewrite (D03 / POS-AUDIT-003)** — Migrate to raw-IDF AP recipe + drop per-rotation softAP re-attach + change teardown from `WiFi.mode(APSTA)` to `esp_wifi_stop(); esp_wifi_deinit();`.
- **C5 zb_sniffer ISR queue (D05 / POS-AUDIT-005)** — Queue frame summary into FreeRTOS queue from ISR; drain in normal task.
- **`feat_ble_scan` bypass (D06 / POS-AUDIT-006)** — Replace direct `NimBLEDevice::init("")` with `radio_switch(RADIO_BLE)`.
- **BTDM release gating (D07 / POS-AUDIT-007)** — Gate `esp_bt_controller_mem_release(ESP_BT_MODE_BTDM)` in `wifi_portal.cpp:430`, `evil_twin.cpp:401`, `ap_signal_test.cpp:62` behind `esp_bt_controller_get_status()` probe + UI warning.
- **`radio.cpp:108-111` WIFI_OFF (D08 / POS-AUDIT-008)** — Replace `WiFi.mode(WIFI_OFF)` with `esp_wifi_stop()` leaving driver inited; gate `esp_wifi_deinit` on heap-healthy check.
- **DNS/HTTP non-blocking (D09 / POS-AUDIT-009)** — `wifi_portal.cpp`, `evil_twin.cpp` overlay split into non-blocking state machine.
- **Arduino softAP migration (D10 / POS-AUDIT-010)** — `saltyjack_dhcp_rogue.cpp:287`, `net_attacks.cpp:558`, `net_wpad.cpp:279,512`, `net_dhcp.cpp:327` switched to raw-IDF AP recipe.
- **BLE indefinite-scan vector growth (D11 / POS-AUDIT-011)** — `scan->setMaxResults(0)` in `ble_whisperpair.cpp`, `ble_extras.cpp`, `ble_karma.cpp`, `ble_toys.cpp`, `ble_finder.cpp` (×2 sites).
- **`cc1101_end` pin restore (D12 / POS-AUDIT-012)** — `pinMode(CC1101_GDO0, INPUT); pinMode(CC1101_CS, INPUT)` before return.
- **`subghz_record.cpp:175` 20 s TWDT (D13 / POS-AUDIT-013)** — Slice the RX wait into 1 s slices with `esp_task_wdt_reset()`, or `esp_task_wdt_delete()` for current task during window.
- **`drone_remoteid.cpp` log under portMUX (D14 / POS-AUDIT-014)** — Move `log_event` out of critical section; use deferred ring like surveillance_hunter.
- **`defensive_monitor.cpp` NimBLE churn + alert race (D15 / POS-AUDIT-015)** — Widen phase windows to ≥30 s; add `portENTER_CRITICAL` around `ble_on_adv` alert enqueue.
- **C5 wifi_attacker shared frame (D18 / POS-AUDIT-018)** — Move `s_frame[26]` to stack-local.
- **`wifi_pmkid.cpp:447-449` blocking notification (D19 / POS-AUDIT-019)** — Non-blocking tone sequence.
- **`mesh.cpp:139` raw-IDF probe (D20 / POS-AUDIT-020)** — Copy `c5_cmd.cpp:248-253` `esp_wifi_get_mode` pattern.
- **`mesh.cpp:120-124` peer-eviction race (net-003)** — Add `portMUX_TYPE s_mux` around `s_peers`/`s_peer_count` ops.
- **`lora_hw.cpp:124` park CS expansion (rf-005)** — Park CC1101 CS (after `gps_end` check) + nRF24 CS HIGH before LoRa NSS asserts.
- **`lora_spectrum.cpp:197` OOM leak (rf-013)** — Outer feature catches inner failure and calls `lora_end(); radio_switch(RADIO_NONE);`.
- **GPS off-by-default (sys-015)** — `main.cpp:32-38,143-144` gate `gps_begin()` + `gps_task` spawn behind NVS `gps_enabled` flag, default OFF.
- **C5 channel-hop mid-attack RESP_STATUS (sys-011)** — Move `send_status` into a separate task; query-on-demand from S3 side instead.
- **Triton freeze hypothesis cluster (D16 / POS-AUDIT-016)** — Iterate per hypothesis per `feedback_careful_iteration.md`: (a) portMUX around `s_q[]`; (b) trim CRITICAL window in WiFi RX cb; (c) suppress Argus draw during STORM-mode deauth bursts; (d) consolidate to single FATFS handle (close hashcat between bursts). Apply ONE fix, verify on hardware, then next. Do NOT stack changes.
- **GPS UART pin tri-state (gps-002)** — `gps_end()` explicit `pinMode(GPS_UART_TX_PIN, INPUT)`.

**Dependencies:** Phase 0 (clean baseline first).

**Exit criteria:** all CRIT and HIGH hardware tickets resolved with on-device verification; no `WiFi.softAP` / `WiFi.mode(WIFI_AP)` in source (grep clean); no `WiFi.mode(WIFI_OFF)` mid-session (grep clean except documented exits); every feature exit path returns radio_switch state to RADIO_NONE; IR LED HIGH at every park site verified by phone-camera; GPS off at boot until user enables; on-device sweep through every audit-cited feature confirms no regression.

## Phase 2 — Module splits

Scope: Files >800 LOC split into per-feature TUs; API boundary cleanup; gate consistency; consolidation of duplicated helpers (random-MAC, HID, OUI lookup, raw-IDF AP bring-up). Phase 1 must complete first so refactors don't chase moving hardware-safety surfaces.

Files >800 LOC requiring split:

- **`triton.cpp` (1457 LOC)** — Split into `triton.cpp` (FSM + UI loop), `triton_radio.cpp` (promisc cb + EAPOL parser + emit_pmkid/hs), `triton_mood.cpp` (mood state machine + face drawer + voice lines), `triton_brain.cpp` (RL channel weighting + persist). POS-AUDIT tri-001.
- **`menu.cpp` (1379 LOC)** — Split into `menu_tree.cpp` (static arrays), `menu_terminal.cpp` (renderer/runloop), `menu.cpp` (types/dispatch). OR migrate to `menu_registry`. POS-AUDIT sys-007.
- **`screensaver.cpp` (1090 LOC)** — Split into `screensaver/` subdir with one .cpp per painter + `screensaver_pool.cpp` (dispatcher/NVS/pool). POS-AUDIT sys-006.
- **`c5_scan.cpp` (967 LOC)** — Split into `c5_status.cpp`, `c5_scan_wifi.cpp` (scan_5g + deauth + nuke), `c5_scan_zb.cpp`, `c5_pmkid.cpp`, `c5_log.cpp` (shared helpers `draw_status_header` + `auth_short` + `save_hs_to_sd` + `save_pmkid_to_sd`). POS-AUDIT net-005.
- **`ui.cpp` (933 LOC)** — Split into `ui.cpp` primitives (~400 LOC) + `ui_anim.cpp` (~500 LOC of animations). POS-AUDIT sys-008.
- **`net_attacks.cpp` (906 LOC)** — Split into `net_uart.cpp`, `net_tcp_tunnel.cpp`, `net_honeypot.cpp`, `net_deaddrop.cpp`, `net_printer.cpp`, `net_ssdp_poison.cpp`. POS-AUDIT net-006.

Other split candidates (close to 800 LOC, recommended now to head off growth):

- **`nrf24_suite.cpp` (771 LOC)** — Split into `nrf24_sniffer.cpp`, `nrf24_mousejack.cpp`, `nrf24_ble_spam.cpp`, `nrf24_scanner.cpp`, `nrf24_jammer.cpp`, plus `nrf24_common.cpp` (BLE encode + CRC + whitening + ascii_to_hid + log_checksum + nrf_write_reg + fingerprint). Move `sniff_dev_t` to `nrf24_types.h`. POS-AUDIT rf-018.
- **`defensive_monitor.cpp` (769 LOC)** — Split into `defensive_monitor.cpp` (WiFi promisc + UI), `defensive_monitor_ble.cpp` (NimBLE callbacks + window tick), `defensive_monitor_state.cpp` (ring buffers + LRU tables + log writer). POS-AUDIT dfn-003.
- **`mimir.cpp` (745 LOC)** — Split into `mimir.cpp` (state machine + UI loop), `mimir_proto.cpp` (json_str/int/bool + command senders + handle_line), `mimir_screens.cpp` (draw_main/targets/attack/live/status). POS-AUDIT mim-001.
- **`ble_whisperpair.cpp` (715 LOC)** — Split into `ble_whisperpair_scan.cpp`, `ble_whisperpair_crypto.cpp`, `ble_whisperpair_probe.cpp`, `ble_whisperpair.cpp` (UI + entry only). POS-AUDIT ble-018a.
- **`wifi_portal.cpp` (664 LOC)** — Split into `wifi_portal_templates.cpp` (HTML + picker UI), `wifi_portal_server.cpp` (DNS+HTTP loop), `wifi_portal.cpp` (entry + run_portal). POS-AUDIT wifi-034.
- **`saltyjack_responder.cpp` (619 LOC)** — Split into `saltyjack_responder.cpp` (entry + UI), `saltyjack_responder_smb.cpp` (SMB2 state machine + NTLM Type-2 builder + Type-3 extractor), `saltyjack_responder_namesvc.cpp` (LLMNR + NBNS handlers). POS-AUDIT rf-4.
- **`ble_blueducky.cpp` (610 LOC)** — Split into `ble_blueducky_keymap.cpp` (ascii_to_hid + token_to_hid + bd_emit + bd_type_string + bd_exec_line + modbit), `ble_blueducky_hid.cpp` (bd_setup_hid + report descriptor), `ble_blueducky.cpp` (UI + entry only). POS-AUDIT ble-018b.
- **`mesh/meshtastic_node.cpp` (591 LOC)** — Split into `meshtastic_rx.cpp` (rx_task + handle_decoded_data), `meshtastic_tx.cpp` (mesh_tx_data + send_* + pack_header), `meshtastic_roster.cpp` (nodes + messages + snapshot/drain). POS-AUDIT net-007.

Consolidation targets:

- **Raw-IDF AP bring-up helper** — Three copies of `esp_wifi_init → mode_ap → set_config → start → set_channel → settle` recipe (`wifi_portal.cpp:417-481`, `evil_twin.cpp:183-226`, `ap_signal_test.cpp:38-116`). Extract to `wifi_ap_helpers.cpp::wifi_raw_ap_up(const char *ssid, uint8_t ch, bool open)`. POS-AUDIT wifi-042.
- **Promisc cb teardown helper** — 8 features set `esp_wifi_set_promiscuous_rx_cb` but only `wifi_probe.cpp` clears it. Extract `wifi_promisc_stop()`. POS-AUDIT wifi-033.
- **Random-MAC helper** — `mac[5] |= 0xC0; ble_hs_id_set_rnd(mac); NimBLEDevice::setOwnAddrType(...)` duplicated in 5 BLE files. Promote to `ble_random_addr()` in new `ble_helpers.h`. POS-AUDIT ble-019.
- **BLE HID common** — `ble_hid.cpp` + `ble_blueducky.cpp` share HID report descriptor, ascii_to_hid, send_key/bd_emit. Promote to `ble_hid_common.cpp` (~150 LOC drift). POS-AUDIT ble-020.
- **OUI lookup O(log n)** — `ble_db.cpp:300-305` is O(n) on ~200 entries, called per BLE adv. Sort `OUI[]` by `oui` at compile-time, binary-search. Same pattern for `sigdb_bt.h:86-91` `bt_mfr_name`. POS-AUDIT ble-008.
- **`ensure_feather`/`ensure_dongle`** — 4 near-identical implementations across `nrf52_suite.cpp`, `nrf52_ble_mitm_relay.cpp`, `nrf52_scout_strike.cpp`, `nrf52_wifi_ble_combo.cpp`. Move to `nrf52_hw.cpp::NRF52Hardware::ensure_or_prompt`. POS-AUDIT rf-033.
- **HSPI park-all-but helper** — `cc1101_park_others`, `nrf24_park_others`, inline park at `lora_hw.cpp:124` overlap. Single `hspi_park_all_but(int active_cs)` centralises. POS-AUDIT rf-31.
- **`sd_format` dedup** — `sd_helper.cpp:115-142` and `tools.cpp:28-69` both recursively delete SD root. Consolidate into single source of truth in `sd_helper.cpp`; document confirmation contract. POS-AUDIT sys-014.
- **Type-2 build + Type-3 readback (NTLM)** — `net_wpad.cpp` and `net_responder.cpp` SMB stub share. Single `ntlm_helpers.cpp` shared with saltyjack/. POS-AUDIT refactor item.
- **`feat_net_hijack` inline rogue-DHCP duplicate** — `net_dhcp.cpp:421-509` has 75 LOC duplicate of `rogue_dhcp_loop`. Replace with parameterised call. POS-AUDIT refactor item.
- **`prefs_helper.{cpp,h}`** — 5 parallel NVS lazy-read patterns (theme / screensaver / sfx / ui_ambient / menu_style). Consolidate into shared helper with per-namespace caches. POS-AUDIT sys-019.
- **Shared file picker UI** — `subghz_replay.cpp:67`, `subghz_broadcast.cpp:182`. Extract `ui_file_picker(const char *root, const char *ext, char *out)`. POS-AUDIT rf refactor.

**Dependencies:** Phase 1 (don't refactor unstable surfaces).

**Exit criteria:** no .cpp >800 LOC; public API headers documented for each split TU; OUI lookup is O(log n) or O(1); shared helpers (raw-IDF AP, promisc teardown, random-MAC, HID common, HSPI park, NVS prefs) extracted with all callers migrated; on-device sweep confirms no behavior regression vs Phase 1 baseline.

## Phase 3 — Feature parity

Scope: Close ✗ gaps from `CROSS_REF.md` "Gaps we should close". Every entry maps to a Phase 3 backlog ticket (POS-AUDIT-101 through POS-AUDIT-150). Sequenced by user-value; ops-relevant gaps first (LAN attacks, BLE depth), exotic later (RFID, JS, Wireguard).

Roughly the order recommended for v0.7:

1. **First wave (operator-essential gaps)** — Bruce HID Exploit Engine 9-tactic (POS-AUDIT-101), Bruce WhisperPair multi-variant (POS-AUDIT-102), Evil-M5 Network Hijack combo (POS-AUDIT-109), Evil-M5 DHCPAttackAuto (POS-AUDIT-110), Evil-M5 cookie siphon (POS-AUDIT-114), Evil-M5 NTLM dedup (POS-AUDIT-115), Evil-M5 portal SD templates (POS-AUDIT-116), mDNS poisoner reply (POS-AUDIT-142), BadUSB SD payload loader (POS-AUDIT-150).
2. **Second wave (capability deltas)** — Ghost DIAL/Chromecast (POS-AUDIT-103), Ghost TP-Link Kasa (POS-AUDIT-104), Ghost 802.11ax HE IE (POS-AUDIT-128), Ghost AP-list beacon spam (POS-AUDIT-129), Bruce nRF24 drone FHSS preset (POS-AUDIT-138), Bruce IR jammer (POS-AUDIT-139), Bruce IR RX capture (POS-AUDIT-140), Evil-M5 Switch DNS (POS-AUDIT-111), Evil-M5 SSDP fake-300 (POS-AUDIT-112), Evil-M5 UPnP NAT abuse (POS-AUDIT-113), Bruce WiGLE upload (POS-AUDIT-148), PORKCHOP wpasec upload (POS-AUDIT-149), Ghost PineAP detect (POS-AUDIT-131), Ghost Pwnagotchi detect (POS-AUDIT-132), Ghost BLE skimmer detect (POS-AUDIT-133), Ghost BLE wardrive (POS-AUDIT-134), Ghost BLE pcap (POS-AUDIT-135), Ghost EAPOL→PCAP (POS-AUDIT-137).
3. **Third wave (system robustness — adopt PORKCHOP patterns)** — Reservation Fence (POS-AUDIT-119), heap-pressure state machine (POS-AUDIT-118), watermark persistence (POS-AUDIT-120), `canGrow` gate (POS-AUDIT-121), BOAR BROS exclusion (POS-AUDIT-122), adaptive DNH hop (POS-AUDIT-123), NimBLE internal-only flag (POS-AUDIT-124), crash viewer (POS-AUDIT-125), diagnostics menu (POS-AUDIT-126), PORKCHOP / Bruce hidden-SSID reveal (POS-AUDIT-127), Evil-M5 boot-to-feature kiosk (POS-AUDIT-117).
4. **Deferred / large-effort gaps** — Bruce mquickjs JS bindings (POS-AUDIT-105), Bruce Wireguard/SSH/SOCKS/Telnet (POS-AUDIT-106), Bruce RFID/NFC (POS-AUDIT-107), Bruce W5500 wired LAN (POS-AUDIT-108), Ghost REST + SPA + Evil-M5 web admin (POS-AUDIT-136 + POS-AUDIT-147), Ghost Power Printer (POS-AUDIT-130), Bruce sub-GHz listen (POS-AUDIT-141), Bruce Ninebot scooter (POS-AUDIT-143), Evil-M5 Skyjack (POS-AUDIT-144), Evil-M5 LDAP (POS-AUDIT-146).
5. **Out of v0.7 scope (no hardware / niche)** — Evil-M5 IMSI catcher (POS-AUDIT-145, requires LTE radio).

**Dependencies:** Phase 2 (split TUs stabilised so new features can colocate cleanly).

**Exit criteria:** every CROSS_REF.md "Gaps we should close" entry either implemented or explicitly deferred with rationale documented in the corresponding POS-AUDIT ticket; cross-ref matrix re-run against new code shows reduced ✗ count.

## Phase 4 — New capabilities

Scope: Things no reference firmware has but POSEIDON should — driven by user-facing roadmap, not the audit. Items will be added here as the roadmap evolves; the audit-driven gaps are all covered in Phase 3.

Candidate items surfaced during cross-ref:

- **Multi-PSK Meshtastic channel support** — `meshtastic_node.cpp:27` hardcodes LongFast PSK + `MESH_CHANNEL_HASH=0x08`. User-requested private channels need a runtime API (NVS-backed `mesh_channel_t` struct + per-channel hash recompute). POS-AUDIT net-008.
- **BLE finder for non-MAC trackers** — Add SD-keyed AirTag tracking (Evil-M5 reads `/evil/FindMyEvil_keys.txt`; POSEIDON's `ble_findmy.cpp` could mirror). POS-AUDIT ble-022.
- **Surveillance hunter BLE side (Raven UUID 0x09C8)** — `surveillance_hunter.cpp:14-17` deferred. POS-AUDIT srv-002.
- **OPSEC USB VID/PID spoof for BadUSB** — Optional Logitech-like descriptor toggle. POS-AUDIT bdu-005.
- **Triton brain export / import** — `triton_learn_save` (`triton.cpp:131`) currently writes brain.bin to SD; an export/import / share workflow could enable cross-device training shareable via the Forge Discord.

Items not in the audit but flagged elsewhere (project memory):

- C5-side proto v4 (if Phase 3 adds new commands beyond v3 18-26, bump version and mirror across both sides atomically rather than mutating v3).
- C5 LED FX integration with Triton mood transitions.
- Delphin / Argus / Forge cross-device flows (out of POSEIDON repo scope but informs API stability constraints).

**Dependencies:** Phase 3.

**Exit criteria:** roadmap items shipped or explicitly deferred with rationale.

## Sequencing notes

- **One-fix-at-a-time on hardware-in-loop changes** (per `feedback_careful_iteration.md`) — Phase 1 work especially. Triton freeze cluster (D16) must iterate per hypothesis with verification before stacking. Do not bundle multiple defensive_monitor changes (D15) into one push without on-device validation between each.
- **POSEIDON release gate** (per `feedback_poseidon_github_gate.md`) — No push/tag/release without explicit user go-ahead for each version cut. `reference_poseidon_release_checklist.md` is the canonical pre-push list.
- **Bruce lib bump (20260123 → newer) is a Phase 0 evaluation, not an automatic change** — Requires on-device regression sweep through all WiFi raw-TX features (deauth, beacon spam, portal, evil-twin, AP signal test, CIW) + BLE NimBLE 2.x sweep (scan, spam, HID, whisperpair, blueducky) + sub-GHz sweep + LoRa sweep before commit. Do NOT bump as part of Phase 0 unless the user explicitly opts in.
- **C5 satellite proto v3 stays frozen across Phase 0–2; if Phase 3 adds new commands, bump to v4 and mirror across both sides atomically.** Don't ship a v3-mutation that an out-of-date C5 firmware might silently misinterpret.
- **BLE features depend on PORKCHOP-style WiFi+BLE init order** (Phase 1 hardware safety) — Feature parity on BLE (Phase 3, esp POS-AUDIT-101/102) gated on the init-order fix landing first. Until D08 (`radio.cpp` WIFI_OFF) and D15 (`defensive_monitor` NimBLE churn) ship, adding new BLE features compounds the heap-fragmentation surface.
- **Phase 2 splits must NOT change behavior** — Any per-feature TU split is a syntactic move; the audit-cited defects in the split units remain Phase-1 work, not Phase-2. (Example: splitting `triton.cpp` into 4 files does not fix the freeze cluster; the freeze cluster is D16/Phase 1.)
- **C5 v3 stub gap (cmds 18-26)** — UI menu entries for `c5_cmd_clients_*`, `c5_cmd_beacon_spam`, `c5_cmd_probe_sniff`, `c5_cmd_deauth_detect`, `c5_cmd_karma`, `c5_cmd_apclone`, `c5_cmd_spectrum`, `c5_cmd_ciw` will appear broken until C5 firmware catches up. Either gate menu entries on `c5_protocol_version() ≥ N` OR document the limitation. Per net-014, also remove or implement `c5_stas`/`c5_probes`/`c5_deauth_hits`/`c5_spectrum_get` which return 0 forever.
- **Hardware-in-loop dependencies** — Some tickets need hardware to verify (defensive_monitor freeze cluster, BTDM gating, Triton STORM-mode TX-cache). Plan a dedicated hw-validation pass between Phase 1 and Phase 2.
- **Token-discipline on diff reviews** (per `feedback_claude_code_operating_contract.md`) — Phase 2 splits will produce large diffs; review per-file or per-pair rather than batch-merging.
