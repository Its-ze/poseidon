# POSEIDON v0.6.1 — Audit Report

Source: 5 inventory maps + 6 module findings (`_audit/maps/`, `_audit/findings/`), commit f5bfc66, 2026-06-01.

## Executive summary

POSEIDON v0.6.1 is a mature, broad firmware — 38 KLOC across 100+ files spanning WiFi, BLE, sub-GHz (CC1101 + nRF24 + nRF52 + LoRa SX1262), IR, GPS, BadUSB, satellite tracking, Meshtastic, ESP-NOW, and a separately-built ESP32-C5 satellite. Across the four reference codebases (Bruce, Ghost ESP, M5PORKCHOP, Evil-M5Project), no other firmware on the same hardware class covers this surface. POSEIDON wins on raw feature breadth — Triton's autonomous handshake hunter, BLE WhisperPair (CVE-2025-36911) with real ECDH+AES, drone RemoteID, surveillance hunter (Flock/Raven), nRF52 dual-radio MITM, multi-mode defensive monitor, satcom tracker, full Meshtastic stack — have no peers in the reference set.

The biggest risk surfaces are concentrated in three buckets. First, **hardware-coordination defects**: IR LED polarity is silently inverted at boot (`main.cpp:63-67,80-81,138-139` parks LOW = ON, contradicting `feedback_poseidon_ir_led_active_low.md`); CC1101 → GPS pin-13 contention can leak permanently if a sub-GHz feature errors out (`subghz_jam_detect.cpp:78,114,194`); `radio.cpp:108-111` calls `WiFi.mode(WIFI_OFF)` mid-session, which PORKCHOP explicitly bans as the canonical heap-fragmentation crash trigger. Second, **API-discipline drift in the WiFi family**: `wifi_ciw.cpp` is the lone surviving user of `WiFi.mode(WIFI_AP)` + `WiFi.softAP()` — the exact "Arduino AP" pattern known to crash under pinned Bruce libs (`ieee80211_hostap_attach +0x2c`); `saltyjack_dhcp_rogue.cpp:287` and three sites in `net_attacks/net_wpad/net_dhcp` also use Arduino softAP despite the map's claim that they use raw-IDF. Third, **architecture cruft**: `menu_registry.cpp/.h` is ~280 LOC of dead code reserving ~110 KB static BSS that no caller ever executes; `hat_manager.cpp:27-36` is a stub that always returns `HatType::NONE`; the splash sprite is a Hokusai Great Wave fade, not the trident the map describes.

The top three things to fix before any other work: **(1)** Flip IR park polarity at every site listed in `sys-001` / `ir-001` (one-line per site, but it's CRIT and affects boot through every feature exit and watchdog tick). **(2)** Add `radio_switch(RADIO_NONE)` to every error/exit path in `subghz_jam_detect.cpp` (otherwise a failed cc1101_begin leaves GPS dead until reboot). **(3)** Rewrite `wifi_ciw.cpp`'s AP bring-up onto the raw-IDF recipe from `wifi_portal.cpp:417-481` and drop the per-rotation `WiFi.softAP()` re-attach; this single file is responsible for three CRIT/HIGH tickets and is the documented thrash pattern that causes `ESP_ERR_NO_MEM (257)` cascades.

PORKCHOP is the canonical reference for system-level robustness on this hardware (same broken-PSRAM Cardputer constraint). POSEIDON should adopt PORKCHOP's heap-pressure state machine, 80 KB reservation fence, watermark persistence, ban on `WiFi.mode(WIFI_OFF)`, and strict WiFi+BLE init-order recipe. Bruce wins on BLE attack depth (HIDExploitEngine 9-tactic, AuthBypassEngine, WhisperPair multi-variant) and JS scripting (mquickjs). Ghost contributes operator-facing patterns (URL-proxied portal, REST/SPA control surface, DIAL/Chromecast hijack, TP-Link Kasa). Evil-M5 still has LAN-attack breadth POSEIDON lacks (Network Hijack combo, SSDP fake-300, cookie siphon, web admin).

## Per-module scorecard

| Module | Memory | Hardware | Perf | Quality | Overall |
|--------|:------:|:--------:|:----:|:-------:|:-------:|
| WiFi | B | C | B | B | B |
| BLE | B | B | B | B | B |
| Sub-GHz + RF | B | C | B | B | B |
| Net attacks + comms | B | C | B | B | B |
| System / UI / c5_node | C | D | C | C | C |
| IR / GPS / specials | C | D | C | C | C |

Grading rubric: A excellent, B solid with minor issues, C works but notable defects, D significant defects, F broken. Justifications:

- **WiFi B**: 15 features all working, raw-IDF AP recipe correct in 3 sites; held back by `wifi_ciw.cpp` Arduino softAP CRIT cluster, BTDM one-way release, blocking notification waits inside DNS/HTTP loops.
- **BLE B**: 15 features cooperative (BLE-coop invariant honoured), WhisperPair + BlueDucky real exploits land; held back by `feat_ble_scan` bypassing `radio_switch(RADIO_BLE)`, indefinite-scan vector growth without `setMaxResults(0)`, OUI table duplicates + O(n) lookup on hot path.
- **Sub-GHz + RF B**: All four radio domains drive cleanly, RMT TX/RX precise, jam-detect unique to POSEIDON; held back by `subghz_jam_detect.cpp` exit-path radio leaks (CRIT), `cc1101_end()` not restoring GDO0/CS, `subghz_record.cpp` 20 s RX exceeds TWDT.
- **Net attacks + comms C/B**: Full LAN suite (DHCP/Responder/WPAD/CCTV/SSDP/lanrecon), C5 satellite v3 protocol, Meshtastic + ESP-NOW dual mesh; held back by Arduino softAP in 4 sites, `mesh.cpp` `WiFi.getMode()` check fails after raw-IDF, peer-table eviction race, blocking 8 s TLE fetch.
- **System / UI / c5_node C**: 6 themes + screensaver pool + ambient + carousel polish; held back by IR polarity CRIT, dead `menu_registry`, `sfx.cpp` blocking input loop (~45 ms ENTER latency), `radio.cpp` WIFI_OFF teardown, GPS always-on at boot (OPSEC), C5 zb_sniffer ISR doing `esp_now_send` + heavy work.
- **IR / GPS / specials C/D**: Broad feature catalogue (TV-B-Gone, drone RID, surveillance, defensive monitor, Triton autonomous hunter, SaltyJack 6-attack port); held back by IR active-LOW polarity bug at 8+ sites, `drone_remoteid.cpp` SD writes inside `portENTER_CRITICAL`, `defensive_monitor.cpp` NimBLE re-init every 5 s (heap-fragmentation freeze candidate), Triton 1457 LOC with multiple lockless float RMW + TX-cache scramble hypotheses, `saltyjack_dhcp_rogue.cpp:287` Arduino softAP.

## Top 20 defects

Ranked by severity then blast radius. Dedupes the IR-polarity bug (appears in both `sys-001` and `ir-001`-class findings) into a single entry citing both sources.

### D01 — [CRIT] `src/main.cpp:63-67,80-81,138-139` + `menu.cpp:1323,1357` + `menu_carousel.cpp:377` + `ir_remote.cpp:110,127,149,196` + `ir_tvbgone.cpp:122,186` + `ir_clone.cpp:59,67-69,220,341` — IR LED parked LOW (= ON) at every park site

- **Module:** System / IR
- **What:** Code writes `digitalWrite(44, LOW)` at boot, watchdog tick, menu transitions, and feature exits. Wiring is anode→3V3, cathode→GPIO 44 — LOW = ON. Comments throughout claim "park HIGH"; the implementation is inverted.
- **Why it matters:** IR LED is silently lit from first instruction onward, drawing current, visible on phone camera, contradicting `feedback_poseidon_ir_led_active_low.md` invariant. Also causes brownout vector during long marks (`ir_clone.cpp:67-69`).
- **Fix:** Flip every off-state write to HIGH; verify with phone-camera test at boot.
- **Backlog:** POS-AUDIT-001 (merges sys-001 + ir-001).

### D02 — [CRIT] `src/features/subghz_jam_detect.cpp:78,114,194,202-205` — Exit paths skip `radio_switch(RADIO_NONE)`

- **Module:** Sub-GHz + RF
- **What:** All early-return paths (cc1101_begin fail, user-ESC during warmup, user-ESC during monitor) return without calling `radio_switch(RADIO_NONE)`. `s_active` stays `RADIO_SUBGHZ`, GPS stays dead, pin 13 driven HIGH.
- **Why it matters:** Next radio user thinks "nothing to tear down"; GPS UART permanently broken until reboot. Cascades — next CC1101 feature skips full re-park.
- **Fix:** Explicit `radio_switch(RADIO_NONE)` on every exit; also fix `pick_freq()` ESC return at line 73.
- **Backlog:** POS-AUDIT-002.

### D03 — [CRIT] `src/features/wifi_ciw.cpp:238-239,265,295-296` — Arduino `WiFi.mode(WIFI_MODE_AP)` + `WiFi.softAP()` violates [WiFi-AP-IDF] invariant

- **Module:** WiFi
- **What:** Lone surviving user of the forbidden Arduino AP path. Re-calls `WiFi.softAP()` every `rotateInterval` ms (in-loop AP thrash) and ends teardown with `WiFi.mode(APSTA)`.
- **Why it matters:** Documented to crash `ieee80211_hostap_attach +0x2c` under pinned Bruce libs; thrash pattern triggers `ESP_ERR_NO_MEM (257)` and `LoadProhibited 0x2c`; APSTA teardown leaves the next radio user in a corrupted state.
- **Fix:** Lift `wifi_raw_ap_up()` helper from `wifi_portal.cpp:417-481`; mutate SSID via `esp_wifi_set_config(WIFI_IF_AP,...)` only; teardown via `esp_wifi_stop(); esp_wifi_deinit();`.
- **Backlog:** POS-AUDIT-003 (merges wifi-001 + wifi-002 + wifi-003).

### D04 — [CRIT] `src/menu_registry.cpp` + `src/menu_registry.h` — Dead code (~280 LOC + 110 KB BSS reservation)

- **Module:** System / UI
- **What:** `MenuRegistry::add`, `::add_submenu`, `::build`, `::root`, `REGISTER_FEATURE` macro all have zero references repo-wide. Map's claim of "dynamic self-registration" is false.
- **Why it matters:** Wastes 110 KB BSS (32×704 + 128×688) that competes with WiFi driver allocations + carries 280 LOC of code that runs in CI and complicates refactors.
- **Fix:** Either delete entirely, or migrate static `MENU_*` tree to actually use the registry.
- **Backlog:** POS-AUDIT-004.

### D05 — [CRIT] `c5_node/main/zb_sniffer.c:28-56` — ESP-NOW send + `led_fx_set` from ISR context

- **Module:** System / c5_node
- **What:** `esp_ieee802154_receive_done` fires in ISR. Handler builds 240 B `posei_msg_t` on stack, calls `proto_send_to` → `esp_now_send` (NOT IRAM-safe) and `led_fx_set`.
- **Why it matters:** Random reboots during heavy Zigbee traffic / coordinator beacon storms; cache-miss crash during concurrent flash op.
- **Fix:** Queue frame summary into FreeRTOS queue from ISR; drain in normal task that calls `esp_now_send` + `led_fx_set`.
- **Backlog:** POS-AUDIT-005.

### D06 — [HIGH] `src/features/ble_scan.cpp:349-358` — `feat_ble_scan` bypasses `radio_switch(RADIO_BLE)`

- **Module:** BLE
- **What:** Calls `NimBLEDevice::init("")` directly with a stale debug comment about "isolating the hang". If prior domain was WiFi, WiFi is still up — BLE Scan runs alongside an active WiFi driver, skipping the PORKCHOP init-order recipe.
- **Why it matters:** Radio time-slice contention, heap pressure, init-order coex contract violated.
- **Fix:** Replace bypass with `radio_switch(RADIO_BLE)`; delete the debug comment.
- **Backlog:** POS-AUDIT-006.

### D07 — [HIGH] `src/features/wifi_portal.cpp:430` + `evil_twin.cpp:401` + `ap_signal_test.cpp:62` — Unconditional `esp_bt_controller_mem_release(ESP_BT_MODE_BTDM)` is one-way

- **Module:** WiFi
- **What:** Three features release BTDM every entry, killing BLE until power cycle. Triton already removed this; portal family did not.
- **Why it matters:** User has no way to recover BLE without unplug; UI doesn't warn.
- **Fix:** Check `esp_bt_controller_get_status()` first; only release if BLE was never inited; or gate behind "kill BLE for session" prompt.
- **Backlog:** POS-AUDIT-007 (merges wifi-004 + wifi-005 + wifi-006).

### D08 — [HIGH] `src/radio.cpp:108-111` — `WiFi.mode(WIFI_OFF)` during teardown

- **Module:** System
- **What:** Every WiFi feature exit funnels through `radio_switch()` which calls `WiFi.mode(WIFI_OFF)`. PORKCHOP explicitly bans this (`wifi_utils.cpp:227-229`: "triggers esp_wifi_deinit/init 257 errors on fragmented heap").
- **Why it matters:** Eventually deadlocks WiFi driver into the `init 257` state; subsequent re-init returns ESP_ERR.
- **Fix:** `esp_wifi_stop()` + leave driver inited (PORKCHOP playbook), or gate `esp_wifi_deinit` on heap-healthy check (largest free > 35 KB threshold à la PORKCHOP `kMinHeapForTls`).
- **Backlog:** POS-AUDIT-008.

### D09 — [HIGH] `src/features/wifi_portal.cpp:539-540,551` + `evil_twin.cpp:329-330,337` — DNS+HTTP serviced from UI loop with 1.2 s `ui_action_overlay` blocking

- **Module:** WiFi
- **What:** `s_dns->processNextRequest()` + `s_http->handleClient()` called synchronously inside the UI loop with `delay(5)` between iterations; the blocking 1.2 s overlay halts DNS+HTTP service entirely.
- **Why it matters:** Captive-portal probe phones can time out and abort the redirect.
- **Fix:** Spawn dedicated task for DNS+HTTP, or run overlay as non-blocking state machine.
- **Backlog:** POS-AUDIT-009 (merges wifi-007 + wifi-008).

### D10 — [HIGH] `src/features/saltyjack/saltyjack_dhcp_rogue.cpp:287` + `net_attacks.cpp:558` + `net_wpad.cpp:279,512` + `net_dhcp.cpp:327` — `WiFi.softAP()` (Arduino) in 5 sites

- **Module:** Net attacks + comms / SaltyJack
- **What:** Five additional sites use the forbidden Arduino softAP path. Map's [WiFi-AP-IDF] tag for saltyjack_dhcp_rogue.cpp is wrong — the actual code calls Arduino.
- **Why it matters:** Same instability surface as D03; intermittent AP-mode rogue-DHCP failure suspected to trace back here.
- **Fix:** Migrate all 5 sites to raw-IDF AP recipe from `wifi_portal.cpp:417-481`.
- **Backlog:** POS-AUDIT-010 (merges net-002 + slt-001).

### D11 — [HIGH] `src/features/ble_whisperpair.cpp:626-636` + 6 other indefinite-scan sites — `scan->start(0, false)` without `setMaxResults(0)`

- **Module:** BLE
- **What:** Indefinite-duration scans (Whisperpair, ble_extras, ble_karma, ble_toys, ble_finder ×2) start without capping the internal `m_scanResults` vector. Every advertisement grows it.
- **Why it matters:** Heap exhaustion after 5+ minutes idle in busy RF environments.
- **Fix:** `scan->setMaxResults(0)` after `setScanCallbacks` in every indefinite-duration scan.
- **Backlog:** POS-AUDIT-011.

### D12 — [HIGH] `src/cc1101_hw.cpp:91` — `cc1101_end()` doesn't restore GDO0 or release CS lines

- **Module:** Sub-GHz + RF
- **What:** `cc1101_end()` calls `setSidle` + `goSleep` but does not reset GDO0 to INPUT or de-park CS lines 12/13/6/5. Sub-GHz → LoRa transition could see CC1101 CS still driven HIGH and GDO0 left OUTPUT from previous TX path.
- **Why it matters:** Latent — but adding LoRa parking (rf-005) without this fix opens the contention window.
- **Fix:** `pinMode(CC1101_GDO0, INPUT); pinMode(CC1101_CS, INPUT)` before return in `cc1101_end()`.
- **Backlog:** POS-AUDIT-012.

### D13 — [HIGH] `src/features/subghz_record.cpp:175` — `cc1101_rmt_rx` 20 s timeout exceeds TWDT

- **Module:** Sub-GHz + RF
- **What:** RX path blocks on `ulTaskNotifyTake` for the full 20 s; no `yield()` / `esp_task_wdt_reset()` inside. IDF default TWDT can be 5 s.
- **Why it matters:** Idle-task watchdog may panic.
- **Fix:** Slice the 20 s wait into 1 s slices with `esp_task_wdt_reset()`, or `esp_task_wdt_delete()` for current task during RX window.
- **Backlog:** POS-AUDIT-013.

### D14 — [HIGH] `src/features/drone_remoteid.cpp:108,197` — `log_event` calls SD I/O inside `portENTER_CRITICAL`

- **Module:** IR / GPS / specials
- **What:** `decode_astm()` holds `portENTER_CRITICAL(&s_mux)` and calls `log_event()` which does `s_log.printf` + `s_log.flush()` — FATFS calls under a portMUX.
- **Why it matters:** Hardlock vector if SD blocks (the surveillance feature got this right with deferred queue; drone_remoteid did not adopt the pattern).
- **Fix:** Move `log_event` out of critical section; use deferred ring like surveillance_hunter.
- **Backlog:** POS-AUDIT-014.

### D15 — [HIGH] `src/features/defensive_monitor.cpp:608,623` + `:201-228` — NimBLE init/deinit every 5 s + MPSC race on alert ring

- **Module:** IR / GPS / specials
- **What:** `enter_wifi_phase`/`enter_ble_phase` cycle NimBLE init/deinit every 5 s (~30 KB heap churn per cycle). Separately, `enqueue_alert` is called from WiFi-ISR-class promisc_cb AND NimBLE-task `ble_on_adv` without portMUX on the BLE side.
- **Why it matters:** Long sessions silently fragment heap until one of the inits fails (heap-fragmentation freeze candidate). MPSC ring corruption under concurrent enqueue.
- **Fix:** Widen phase windows to ≥30 s, or switch to coexist pattern; add `portENTER_CRITICAL` around `ble_on_adv`'s enqueue.
- **Backlog:** POS-AUDIT-015 (merges dfn-001 + dfn-002).

### D16 — [HIGH] `src/features/triton.cpp` 1457 LOC — Freeze regression hypothesis cluster

- **Module:** IR / GPS / specials
- **What:** Multiple stacked risk patterns: (a) lockless float RMW on `s_q[]` across cores → NaN risk wedges softmax (`:583-588`, `:680`); (b) long `portENTER_CRITICAL` inside WiFi RX cb holding 1024-byte strncpy (`:188-198`); (c) Argus `pushImage` during deauth burst in STORM mode = canonical TX-cache scramble pattern (`:1250`); (d) `s_file` FILE_APPEND + lazy `s_wdr_file` open = 2 concurrent FATFS handles.
- **Why it matters:** Matches the active Triton freeze regression `project_triton_freeze_debug.md` — 14 hardware-in-loop iterations have ruled out the TX path. Hypothesis cluster needs paper review then careful iter-by-iter validation per `feedback_careful_iteration.md`.
- **Fix:** Iterate per hypothesis; portMUX around `s_q[]` access; trim CRITICAL window; suppress Argus draw during deauth bursts.
- **Backlog:** POS-AUDIT-016.

### D17 — [HIGH] `src/sfx.cpp:38-44` — NVS handle never closed; `src/input.cpp:71-90` SFX blocking in input loop

- **Module:** System
- **What:** `sfx_init` does `s_prefs.begin("sfx", false)` and never `.end()` — handle leak across process lifetime. Separately, `sfx_click`/`sfx_select`/`sfx_back` blocking calls (14-45 ms) fire from `input_poll_raw` on every keypress.
- **Why it matters:** ENTER-to-feature latency ~45 ms; rapid mnemonic nav throttled to ~70 keys/sec; uncommitted writes lost on crash.
- **Fix:** Scope NVS per call (`Preferences p; p.begin; p.put; p.end;`); dispatch SFX into cooperative scheduler tick or tiny `sfx_player` task with tone-event queue.
- **Backlog:** POS-AUDIT-017 (merges sys-004 + sys-005).

### D18 — [HIGH] `c5_node/main/wifi_attacker.c:38-45,79-80,160-161` — Shared `s_frame[26]` between two tasks

- **Module:** System / c5_node
- **What:** Single static `s_frame[26]` is mutated by both `deauth_targeted_task` and `deauth_bcast_task`. Currently serialised by one-CMD-at-a-time dispatch, but back-to-back CMDs and a 4 KB task push mean the earlier task may still be looping over the buffer when the next mutates it.
- **Why it matters:** Stale frame data in flight; mid-burst MAC corruption.
- **Fix:** Move `s_frame` to a stack-local in each task; pass pointer.
- **Backlog:** POS-AUDIT-018.

### D19 — [HIGH] `src/features/wifi_pmkid.cpp:447-449` — Blocking tone+delay notification while RX task fires `promisc_cb`

- **Module:** WiFi
- **What:** `draw_notification` calls `M5Cardputer.Speaker.tone()` + `delay(130)` × 2 (~600 ms total) while WiFi RX task continues queueing EAPOL into 8-slot `dynamic_rx_buf` pool.
- **Why it matters:** Sustained EAPOL traffic can overflow the pool before UI returns.
- **Fix:** Non-blocking tone sequence (state machine), or move notification to dedicated low-prio task.
- **Backlog:** POS-AUDIT-019.

### D20 — [HIGH] `src/mesh.cpp:139` — `WiFi.getMode() == WIFI_OFF` check fails after raw-IDF init

- **Module:** Net attacks + comms
- **What:** Arduino `WiFi.getMode()` lies when raw-IDF init is in use (C5 has the documented fix at `c5_cmd.cpp:248-253`). `mesh_begin` after any raw-IDF feature hits `WiFi.mode(WIFI_STA)` which double-creates the netif and asserts in `esp_netif_create_default_wifi_sta`.
- **Why it matters:** Crash on entry to mesh status if user just exited Triton/portal/deauth.
- **Fix:** Copy `c5_cmd.cpp:248-253` `esp_wifi_get_mode` probe pattern.
- **Backlog:** POS-AUDIT-020.

## Strengths

POSEIDON exceeds all 4 reference firmwares on the following — protect these surfaces during refactor:

- **Triton autonomous handshake hunter** — 4-mode FSM (HUNT/STEALTH/SURGICAL/STORM) with RL channel weighting, Argus gotchi mood state machine, parallel wardrive CSV. No peer firmware has an autonomous hunter at all.
- **BLE WhisperPair CVE-2025-36911 real exploit** — SD-loaded pubkey table + ECDH (secp256r1) + AES-128 ECB + KBP characteristic write + decryption. Bruce has more attack variants (multi-protocol confusion) but POSEIDON's standard path with SD-side keys is a closer fit to ops workflow.
- **BLE TX-side suite breadth** — finder (Geiger RSSI), findmy (AirTag flock), karma, clone, GATT, HID, BlueDucky, MITM-via-nRF52, sourapple multi-vendor — Ghost is observe-only, PORKCHOP is intentionally minimal, Evil-M5 uses old bluedroid.
- **Sub-GHz signal library + jam-detect + LoRa spectrum** — categorised baked signals (cars/pranks/tesla/home/custom), RSSI-anomaly jam detector with siren alert, single-bin LoRa spectrum waterfall — none of the four refs ship these.
- **Drone RemoteID + surveillance hunter** — ASTM F3411-22a 0xFFFA passive scanner with MessagePack, Flock/Raven signature DB with deferred queue draining — POSEIDON is the only one with either.
- **Defensive monitor multi-class** — 7 detector classes (DEAUTH_FLOOD / EVIL_TWIN / BEACON_SPAM / BCAST_DEAUTH / WIFI_KARMA / BLE_SPOOF / BLE_FLOOD), JSONL log with audio alerts. Ghost has partial detection patterns; nobody else.
- **Satcom tracker + Meshtastic full stack** — SGP4 TLE fetch from Celestrak + SD cache + 8 NORAD favorites + live polar skyplot; full Meshtastic leaf node (AES-CTR decrypt + hand-written protobuf + node roster + position + paging). Bruce has point-to-point LoRa chat only.
- **Screensaver pool + ambient layer + theme system** — 10 painters, per-theme ambient, 6 curated RGB565 palettes with NVS persistence, live preview, Hokusai splash. Bruce has BruceTheme struct; nobody else.

## Known blind spots

- **Hardware-in-loop validation** — paper audit only. Backlog tickets (especially Triton freeze cluster D16, NimBLE init/deinit churn D15, BTDM release D07) need on-device verification before close.
- **Library-internal issues** — auditors did not read the Bruce-libs custom `framework-arduinoespressif32-libs-20260123` binary archive. Anything beneath the lib boundary (raw 80211 TX path, mbedtls extras) surfaces as "suspected upstream" only.
- **UX flow / menu navigation** — static read only. No on-device assessment of carousel feel, terminal vs carousel parity, hotkey collisions, or feature-discovery friction.
- **Satellite firmwares excluded** — `argus_node`, `proteus_node`, `siren_node` (and the Cardputer companion brain `delphin`) were out of scope. `c5_node` was the only satellite firmware audited.
- **Runtime perf measurement** — no flash/profile. All performance findings (e.g., 50% reduction estimates on `wifi_clients` row-redraw, `lora_spectrum` waterfall) are static analysis.
- **C5 v3 protocol drift** — S3 side exposes senders for CMDs 18-26; C5 explicitly stubs them as "not implemented yet". Backlog tracks but the C5 firmware lifecycle is out of scope for this audit.
- **Bruce lib bump** — 20260123 → newer was not exercised; risk that a lib refresh introduces fresh raw-80211 TX regressions.
