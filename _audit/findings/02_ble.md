# BLE Module Audit

## Current state

POSEIDON's BLE bucket is 15 feature `.cpp` files + 1 lookup table `.cpp` + 3 headers (`ble_types.h`, `ble_db.h`, `sigdb_bt.h`), ~5.2 KLOC. All features target NimBLE-Arduino 2.x against IDF 5.5 on ESP32-S3; the host stack is brought up via `radio_switch(RADIO_BLE)` (`src/radio.cpp:154-167`) which calls `NimBLEDevice::init("")` once and reuses the host across feature transitions.

Feature entry points: `feat_ble_scan` (`ble_scan.cpp:343`) drives the master device picker — dual-pass active(8s)/passive(5s) scan with custom `setInterval(97)/setWindow(67)` Bruce timing (`ble_scan.cpp:286-287`), seeds `g_ble_target`, then hot-routes to child features by hotkey at line 521-534. `feat_ble_spam` (`ble_spam.cpp:177`) cycles Apple/Samsung/Google/Windows brand spam via cooperative `spam_tick` (`ble_spam.cpp:110-148`). `feat_ble_sourapple` (`ble_sourapple.cpp:409`) is the 7-mode multi-vendor spam (Apple POPUP/ACTION/AIRTAG + Samsung Buds/Watch + MS SwiftPair + Google Fast Pair). `feat_ble_hid` (`ble_hid.cpp:176`) and `feat_ble_blueducky` (`ble_blueducky.cpp:543`) bring up `NimBLEHIDDevice` keyboard GATT servers — both forcibly `esp_wifi_stop()/esp_wifi_deinit()` before init (`ble_hid.cpp:186-187`, `ble_blueducky.cpp:407-408`) for the ~30 KB headroom HID-on-NimBLE needs.

Probe/exploit suite: `feat_ble_whisperpair` (`ble_whisperpair.cpp:615`) implements CVE-2025-36911 — scans FE2C service data, optionally loads SD-baked anti-spoof pubkeys, runs real secp256r1 ECDH + AES-128 ECB on the KBP characteristic, decrypts the response, lifts BR/EDR MAC, logs verdict to `/poseidon/whisperpair.csv`. `feat_ble_blueducky` (`ble_blueducky.cpp:543`) is CVE-2023-45866 BlueDucky — Just-Works BLE HID against unpatched Android. `feat_ble_gatt` (`ble_gatt.cpp:220`) is the explorer (services/characteristics tree, R/W hex). `feat_ble_flood` (`ble_flood.cpp:82`), `feat_ble_karma` (`ble_karma.cpp:65`), `feat_ble_clone` (`ble_clone.cpp:52`), `feat_ble_findmy` (`ble_findmy.cpp:114`) — connection-flood, rotating-name karma decoy, MAC+name spoof, Apple Find My broadcast emulator. Trackers covered by `feat_ble_finder` / `feat_ble_finder_hunt_mac` (`ble_finder.cpp:307,400`) Geiger meter and `feat_ble_tracker` (`ble_extras.cpp:77`). `feat_ble_toys` (`ble_toys.cpp:223`) scans+controls Lovense brands. DB lookups in `ble_db.cpp` (OUI/Apple subtype/Fast Pair/SVC UUID/CHR UUID, all linear scans on flat tables).

Every offensive feature uses cooperative tick on the UI thread (no `xTaskCreate` around NimBLE host work), matching the BLE-coop invariant. Scan callbacks are static objects (`s_cb_obj`-style allocation) so repeat-entry doesn't churn heap. MAC randomization centralizes on `mac[5] |= 0xC0` (MSB top-2 bits = static-random) — there are explicit fixed-comment regressions in `ble_spam.cpp:96-105`, `ble_flood.cpp:40-46`, `ble_karma.cpp:44-52`, `ble_findmy.cpp:73-79`, `ble_sourapple.cpp:361-373` all noting the prior wrong-byte bug.

## Defects

1. **HIGH — `ble_scan.cpp:349-358` bypasses `radio_switch(RADIO_BLE)`.** `feat_ble_scan` calls `NimBLEDevice::init("")` directly with the comment "Skip radio_switch wrapper — call NimBLE init directly to isolate whether the hang is in radio.cpp's teardown chain or in NimBLE." This means if the previous radio domain was WiFi (typical — LAN attacks, scan, wardrive), WiFi is **still up** (no teardown_current was called); BLE Scan now runs alongside an active WiFi driver. Coexistence works on paper but radio time-slice contention, heap pressure, and the WiFi/BLE init-order PORKCHOP recipe (`stop scan/adv → deinit → 100ms → yield → 50ms → WiFi.mode`) are all skipped. Diagnostic block (line 371-390) confirms `wifi_mode = ?` is even queried. Fix: replace bypass with `radio_switch(RADIO_BLE)`; the comment about "isolating hang" is a months-old debug residue.

2. **HIGH — `ble_hid.cpp:186-187` + `ble_blueducky.cpp:407-408` `esp_wifi_stop()`/`esp_wifi_deinit()` directly without going through `radio_switch`.** Both features call `radio_switch(RADIO_NONE)` first (`ble_hid.cpp:178`, `ble_blueducky.cpp:547`) so this is a belt-and-suspenders teardown, but if the prior domain was already `RADIO_NONE` then esp_wifi was already torn down by `teardown_current()` — the second `esp_wifi_deinit()` returns `ESP_ERR_WIFI_NOT_INIT` and is silently ignored. Not a crash but the deinit-already-deinited pattern is what PORKCHOP's `wifi_utils.cpp:227-229` explicitly warns against on fragmented heap. Fix: gate on `esp_wifi_get_mode(&m) == ESP_OK` before stop/deinit, or push this logic into a `radio_force_wifi_off()` helper in `radio.cpp`.

3. **HIGH — `ble_whisperpair.cpp:615-679` runs `radio_switch(RADIO_BLE)` then immediately starts an indefinite `scan->start(0, false)` without checking `NimBLEDevice::isInitialized()`.** `radio_switch` will init NimBLE if not already up, but if the prior domain was BLE already and NimBLE was deinit'd by `ble_hid`/`blueducky` exit (those features delete `s_hid` but leave NimBLE running per their comments), the state can be inconsistent. Also, the scan is started before `scan->setMaxResults(0)`, so NimBLE keeps an internal `m_scanResults` vector for the lifetime of the indefinite scan — every adv (`s_adv_seen_total++`) grows that. After 5+ minutes idle, watch heap. Fix: `scan->setMaxResults(0)` after `setScanCallbacks` (also applies to all indefinite-duration BLE scans).

4. **MED — `ble_flood.cpp:82-135` no cleanup on connect-in-flight.** When user presses ESC mid-flood, line 134 `ble_gap_conn_cancel()` is called but `s_flood_alive = false` only stops further `flood_tick` calls. If `ble_gap_connect()` was issued at the moment ESC was polled, the callback `flood_cb` (`ble_flood.cpp:26-35`) may still fire after function return — by then `target` on the stack is gone. `target` is `static ble_addr_t` (line 56) so survives, but the cancel won't terminate the new conn if BLE_GAP_EVENT_CONNECT arrives between the cancel and function exit. Fix: in the exit path, loop `delay(50)` + `ble_gap_conn_cancel()` until `s_flood_last_rc != 0` or 500 ms elapsed.

5. **MED — `ble_clone.cpp:65 / clone reuses target MAC w/o collision detection.** `mac[5] |= 0xC0` overwrites the top 2 bits of the **target's** MSB — if the target's real MAC was a public address with byte 5's top bits not 11, the cloned MAC has different top bits and is therefore a *different address* on the air than the target. The comment at line 60 says "g_ble_target.addr is stored in NimBLE's native little-endian" but `ble_scan.cpp:148` does `memcpy(x.addr, addr.getBase()->val, 6)` — getBase()->val IS the same little-endian form, so this is consistent. However, the OR-with-0xC0 means: (a) for already-random target → no behavior change (already 11); (b) for public target → clone MAC is **not** equal to target. No collision actually occurs in case (b), defeating the feature's stated purpose. Fix: when `g_ble_target.is_public`, abort with `ui_toast("public MAC — clone needs random",...)` rather than producing a silent semantic bug; or document that "clone" only works on random-addressed peripherals.

6. **MED — `ble_db.cpp:18-136` has duplicate / colliding OUI entries.** `0x10417F` appears twice for Apple on line 31. `0x001A11` is mapped to "Google" (line 55) AND "Xiaomi" (line 87) — first match wins (Google), Xiaomi entry is dead. `0x0013A9` collides between "Sony" (line 64) and "Sony PS" (line 127). `0x0017A4` collides between "Dell" (line 118) and "HP" (line 120) — Dell wins, HP entry for that OUI is dead. Fix: dedup the table; for genuine OEM-vs-OEM splits (e.g., `0x0013A9` was reassigned), pick the more current owner.

7. **MED — `sigdb_bt.h:25-83` has duplicate manufacturer IDs.** `0x0131` Cypress at lines 55 and 75. `0x0157` "Anhui Huami (Mi Band)" at line 56 and "Mi-Fit / Huami" at line 76 (same vendor, different string — second is dead code via first-match-wins). The PROGMEM table burns flash for unreachable rows. Fix: dedup.

8. **MED — `ble_db.cpp:300-305` (`ble_db_oui`) is O(n) over ~200 entries, called per BLE adv in `ble_scan.cpp:88` AND per scan-result loop pass in `ble_extras.cpp` sniff callback (`ble_extras.cpp:175-191`).** Map sniffer is `addv hex dump only` — doesn't call OUI — but `classify()` in `ble_scan.cpp:69-130` does the OUI lookup on every dedup-miss adv. With dual-pass 8s+5s active scan and busy environments hitting `BLE_MAX_DEVS=48`, this is ~5000+ table walks. Not a measured crash, but on the hot path. Fix: sort `OUI[]` by `oui` at compile time and binary-search; or build a small open-addressing hash on first call.

9. **MED — `ble_db.cpp:307-353` (`ble_db_identify`) early-returns "Samsung" or "MS Swift Pair" before checking the OUI fallback** (lines 322-341). Apple OK (most-specific via Continuity subtype). But if a device sends `mfg[0..1] = 0x75 0x00` (Samsung CID) and also has a legit OUI of `0x0007AB` (Samsung) — the manufacturer-data path produces "Samsung"; the OUI path would have, too. Fine. Real bug: if a *random-MAC* Samsung phone advertises with the Samsung CID + a SmartTag-shaped sub-prefix that ISN'T 0x42 0x09, the routine returns "Samsung" even though the device is just an unrelated Samsung phone. Cosmetic — but the "is_public" badge in `ble_scan.cpp:217-218` will color it WARN (red) suggesting public when it's random. Fix: factor the badge color decision into `addr.is_public` purely, decouple from the classify type string.

10. **MED — `ble_whisperpair.cpp:626-636` calls `scan->setScanCallbacks(&s_cb, true)` then `scan->start(0, false)` without first calling `scan->stop()` if a previous BLE feature left a scan running.** If user enters Whisper from BLE Scan via 'W' hotkey through `feat_ble_whisperpair_from_target` (`ble_whisperpair.cpp:685`), the prior scan from `ble_scan` is `scan->stop()`'d at `ble_scan.cpp:546` only if user exits the device-detail screen via ESC. The hotkey path at `ble_scan.cpp:526` jumps directly into the child feature WITHOUT stopping scan — so `feat_ble_whisperpair_from_target` skips its scan setup (it goes straight to `run_probe`) but `feat_ble_whisperpair` (the top-level menu entry) doesn't have a stop. Fix: `if (scan->isScanning()) scan->stop(); delay(20);` before reconfiguring.

11. **MED — `ble_whisperpair.cpp:425-429` `run_probe` busy-waits 3 s with `delay(50)` while a GATT notify is pending.** Cooperative tick philosophy applies to UI loops, but here it's a one-shot probe and 3 s is in-spec. Not a defect, more a watchdog reminder: NO `esp_task_wdt_reset()` is called during the wait, and IDF default TWDT can be 5 s. If user adds longer wait windows or layered probe variants, this becomes a panic. Fix: add `esp_task_wdt_reset()` inside the wait loop OR raise the probe-tick granularity.

12. **MED — `ble_hid.cpp:243-253` doesn't deinit NimBLE or stop the server on exit.** Comment at line 240 explicitly defers cleanup to `radio_switch()`. But the next feature the user picks might re-enter `feat_ble_hid` (or another BLE feature), and the NimBLE server with its HID services, characteristics, descriptors is **still registered** in the host's GATT table. NimBLE doesn't expose per-server deregistration cleanly. Result: every Bad-KB entry without a domain change in between leaks ~1.5 KB of GATT structures. Mitigation at line 252 (`delete s_hid`) is partial — the `NimBLEService`/`NimBLECharacteristic` registered in `setup_hid` are owned by the server, not the HIDDevice. Fix: force `radio_switch(RADIO_NONE)` then `radio_switch(RADIO_BLE)` on exit OR document Bad-KB as "one-shot per session".

13. **LOW — `ble_spam.cpp:192` casts an enum return to `int` and tests `< 0`.** `spam_kind_t` is a C-style enum with `SPAM_COUNT = 5`. `pick_kind` returns `(spam_kind_t)-1` on ESC (line 172). Modern compilers default enums to `int` so this works, but adding `: uint8_t` later (size optimization) would silently break ESC. Fix: use a sentinel inside the enum (`SPAM_NONE = -1`) or change pick_kind to `int`.

14. **LOW — `ble_spam.cpp:131-132` `raw_len` claims 11 bytes for Google Fast Pair build, but `build_google` writes 11 bytes (`tmpl[] = {0x03,0x03,0x2C,0xFE,0x06,0x16,0x2C,0xFE,X,Y,Z}` — 11 entries) and `build_windows` writes 11 — matches.** But `build_apple` (`ble_spam.cpp:36-48`) writes 8 bytes of template then fills [8..30] with random — so raw_len 31. Re-check: line 47 fills `i=sizeof(tmpl)=8 to 31` exclusive → bytes 8..30 → 23 bytes random. Adv data length byte at `adv[0] = 30`. Total wire bytes = 31 (length-prefixed). OK. No bug — verified.

15. **LOW — `ble_findmy.cpp:99` `dwell = (s_fm_tags <= 1) ? 60000 : 3000` flips on every tick.** `s_fm_tags` is set once before the loop (line 142), so this is constant within a session; no logic bug. But the read of `s_fm_tags` is uncached and reads a `volatile int` every tick — minor cycles. Fix: snapshot into a local once.

16. **LOW — `ble_finder.cpp:212-305` `beep_tracker` stops scan, creates client, dumps services to Serial, attempts write — but if the device disconnects mid-walk the for-loops over `getServices()`/`getCharacteristics()` (line 256, 258) operate on freed vectors.** `s_client` is local (line 229) so dangling pointer scope is bounded. NimBLE 2.x `getServices(true)` returns a `const std::vector&` whose lifetime equals the client — fine until `disconnect()` at line 287. Not actually a defect for current code path, but if anyone refactors to a longer-lived client this is a trap.

17. **LOW — `ble_blueducky.cpp:421` `new NimBLEHIDDevice(srv)` not paired with explicit cleanup of the GATT services attached to `srv`.** Same as defect 12 — the underlying GATT services persist in NimBLE host state until full deinit. The `delete s_hid` at line 608 frees the HIDDevice wrapper but the `NimBLEService` rows on the server are leaked until next `NimBLEDevice::deinit(true)`. Fix: call `srv->removeService(s_hid->getHidService(), true)` before delete (NimBLE 2.x supports this).

18. **LOW — `ble_karma.cpp:103-110` does `adv->stop(); delay(5); random_mac(); setOwnAddrType()` but the data carrier name set via `data.setName(name)` (line 113) is not unique per device — using `BLE_GAP_CONN_MODE_UND` (line 116) advertises as connectable, meaning each rotation invites real connect attempts. No GATT server is registered, so the connect attempts hit empty surfaces and disconnect immediately with no notification to the user. Wastes cycles. Fix: either spin up a one-char generic GATT like clone does, or set `BLE_GAP_CONN_MODE_NON` for pure beacon karma.

19. **LOW — `ble_extras.cpp:178-179` `s_sniff_file.printf` writes byte order `a[5], a[4], a[3], a[2], a[1], a[0]` — display-friendly MSB-first.** But `ble_scan.cpp:457-459` and `ble_blueducky.cpp` write `x.addr[0], x.addr[1], ... x.addr[5]` — LSB-first. Inconsistent MAC byte order across CSV outputs makes cross-correlation between blescan.csv and blesniff.csv a manual reverse exercise. Fix: pick one (display-order MSB-first is standard) and apply across all SD writers.

20. **LOW — `ble_db.cpp:135` sentinel `{0, nullptr}` works because no real OUI is 0x000000, but the `OUI[]` array initializer relies on this for loop termination in `ble_db_oui` (line 302).** Tabulating: array has ~190 entries plus sentinel. If someone appends an OUI of `0x000000` (unlikely but possible — many vendors have 24-bit OUIs whose top byte is 0x00), the loop terminates early. Fix: use `sizeof(OUI)/sizeof(OUI[0])` array iteration with explicit count instead of sentinel-terminated.

## Cross-ref deltas

### vs Bruce

| Feature | POSEIDON | Bruce | Verdict | Notes |
|---|---|---|---|---|
| BLE scan | `ble_scan.cpp:343` | `ble_common.cpp:169` | par | Both use NimBLE 2.x; POSEIDON adds dual-pass active+passive (`ble_scan.cpp:317-336`); Bruce uses single `getResults(timeout)` only |
| BLE spam (multi-brand) | `ble_sourapple.cpp:409` | `ble_spam.cpp:482` | we_have_better | POSEIDON: 7 modes incl. Fast Pair + per-mode counters. Bruce: 5 modes, no FP. POSEIDON's MAC-randomize-per-adv fix (`ble_sourapple.cpp:361-373`) is documented and explicit; Bruce uses NimBLE setOwnAddrType default. |
| BLE flood/DoS | `ble_flood.cpp:82` | `BLE_Suite.cpp:2725` | parity | Bruce has multi-target (`connectionFlood`, BLE_Suite.cpp:2242); POSEIDON single-target only |
| Sour apple | `ble_sourapple.cpp:409` | `ble_spam.cpp:156` | we_have_better | POSEIDON merges sour-apple into the multi-vendor flood; Bruce keeps it as an enum branch |
| BLE finder (Geiger RSSI) | `ble_finder.cpp:307` | — | poseidon_only | Bruce has no beacon-distance hunter |
| BLE findmy (AirTag emit) | `ble_findmy.cpp:114` | — | poseidon_only | Bruce has no Find My emulator |
| BLE karma | `ble_karma.cpp:65` | — | poseidon_only | Bruce karma is WiFi-only |
| WhisperPair | `ble_whisperpair.cpp:615` | `BLE_Suite.cpp:1115` | bruce_better_kinda | Bruce's `WhisperPairExploit::execute` (BLE_Suite.cpp:1115) implements multiple attack variants (`sendProtocolAttack`, `sendStateConfusionAttack`, `sendCryptoOverflowAttack` at :1004/1045/1074). POSEIDON has only the standard ECDH+AES path with optional BR/EDR decrypt — no protocol-state-confusion or crypto-overflow variants. POSEIDON has SD-loaded pubkey table; Bruce reads pubkeys from compiled fastpair_models. |
| BLE clone | `ble_clone.cpp:52` | `BLE_Suite.cpp:2091` | bruce_better | Bruce `AuthBypassEngine` is a full identity spoof + name; POSEIDON copies MAC+name but applies `mac[5] \|= 0xC0` which breaks collision on public-MAC targets (see defect 5) |
| BLE GATT explorer | `ble_gatt.cpp:220` | `BLE_Suite.cpp:214` | parity | Both walk services+chars; Bruce has wider read/write/notify subscribe; POSEIDON has hex parse on write |
| BLE HID (Bad-KB) | `ble_hid.cpp:176` | `BLE_Suite.cpp:921` | bruce_better | Bruce's `HIDExploitEngine` has 9 OS-specific tactics (BLE_Suite.cpp:462-825); POSEIDON has single-tactic plain HID kbd + 6 disguise names |
| BlueDucky CVE-2023-45866 | `ble_blueducky.cpp:543` | `ducky_typer.cpp:454` | parity | Both use NimBLE HID kbd + Just-Works; POSEIDON 30-50 kps cadence (`BD_KEY_GAP_MS=22`); Bruce reuses USB ducky path with ble=true flag |
| BLE MITM relay | (out of scope, `nrf52_ble_mitm_relay.cpp`) | — | poseidon_only | n/a |
| BLE Ninebot scooter | — | `ble_ninebot.cpp:111` | bruce_only | Not in POSEIDON |

### vs Ghost ESP

| Feature | POSEIDON | Ghost | Verdict | Notes |
|---|---|---|---|---|
| BLE scan | `ble_scan.cpp:343` | `ble_manager.c:465` | we_have_better | Ghost has handler chains via callbacks; POSEIDON has type classification + child-feature routing |
| BLE spam DETECTOR | (none, ours is TX) | `ble_manager.c:348` | ghost_only_detector | Ghost ONLY detects spam, never transmits |
| BLE airtag SCAN | `ble_extras.cpp:77` (tracker) | `ble_manager.c:389` | par | Ghost scan-only; POSEIDON adds attempted "beep" anti-stalking write (`ble_finder.cpp:209`) |
| BLE wardrive CSV | — | `callbacks.c:769` | ghost_only | POSEIDON has no BLE wardrive output (WiFi wardrive only) |
| BLE skimmer detect | — | `ble_manager.c:746` | ghost_only | POSEIDON has no skimmer-specific advert scorer |
| BLE TX-side spam/clone/HID/etc. | All BLE TX features | absent | we_have_better | Ghost is observe-only BLE |
| BLE pcap | — | `pcap.c:PCAP_CAPTURE_BLUETOOTH` | ghost_only | POSEIDON writes CSV blesniff only |

### vs PORKCHOP

| Feature | POSEIDON | PORKCHOP | Verdict | Notes |
|---|---|---|---|---|
| BLE scan | `ble_scan.cpp:343` | `piggyblues.h:94` | par | Both continuous async; PORKCHOP heap-stable-aware (heap_gates) |
| BLE spam (4-brand) | `ble_spam.cpp:177` | `piggyblues.cpp` (sendAppleJuice/sendAndroidFastPair/sendSamsungSpam/sendWindowsSwiftPair) | we_have_better | POSEIDON has Fast Pair AND multi-mode "all" cycling; PORKCHOP fires by explicit user-pick |
| BLE finder/karma/whisperpair/clone/GATT/HID/BlueDucky/findmy | All | absent in PORKCHOP | we_have_better | PORKCHOP intentionally minimal BLE |
| WiFi+BLE init recipe | `radio.cpp:99-176` | `network_recon.cpp:794-822` + `piggyblues.cpp:594` | porkchop_better | PORKCHOP enforces stop scan/adv → deinit → 100ms → yield → 50ms → WiFi.mode + inverse for BLE-after-WiFi. POSEIDON's `teardown_current()` does deinit then `delay(100)` but no `yield()` between, and the BLE-up path does NOT pre-yield WiFi-side bufs. |
| Cooperative tick architecture | All BLE features cooperative | All modes cooperative | par | Both correct on the rc=-1 silent fail mitigation |
| Heap gating | none | `heap_gates.h` `canTls`, `canGrow` | porkchop_better | POSEIDON's HID features just blindly `esp_wifi_deinit()` for headroom — no progressive shedding |
| PSRAM disabled / NimBLE internal-only | implicit (no explicit force) | `platformio.ini:27-28` explicit | porkchop_better | POSEIDON should add the same NimBLE internal-allocation cflags to guarantee PSRAM-broken behavior |

### vs Evil-M5

| Feature | POSEIDON | Evil-M5 | Verdict | Notes |
|---|---|---|---|---|
| BLE Name Flood | `ble_spam.cpp` + `ble_sourapple.cpp` | `.ino:308` ~`BLEDevice::init` :28939 | we_have_better | Evil uses old BLEDevice (bluedroid), POSEIDON uses NimBLE 2.x w/ 7 modes |
| AirTag / FindMy | `ble_findmy.cpp:114` | `.ino:29927 FindMyEvilTx` + SD keys | par | Evil reads `/evil/FindMyEvil_keys.txt`; POSEIDON uses purely random key bytes (`ble_findmy.cpp:71` `for(i<22)key[i]=esp_random()`) — Evil's flock is keyed, POSEIDON's is random. POSEIDON could add SD-key load to match. |
| BLE scan / GATT / HID / clone / karma / whisperpair / flood / finder | All present | absent | we_have_better | Evil-M5 BLE is purely emit-only |

## Optimisation opportunities

- **`ble_db.cpp:300-305` `ble_db_oui` O(n) on ~200 entries, called per BLE adv.** Sort `OUI[]` by `oui` at compile-time (constexpr / staticly-sorted), binary-search. Or alternately: insertion-sorted on first call into a RAM mirror. Expected gain: ~50% CPU on `classify()` hot path during 13 s dual-pass scan with 30+ devs.
- **`ble_db.cpp:154-164` `APPLE_PP[]` is ~20 entries linear scan in `ble_db_apple` (line 176-180).** Convert to small lookup-by-index table indexed by subtype value (0x00..0x1F). Expected gain: minor — only ~5-10 lookups per dev — but removes a loop.
- **`sigdb_bt.h:86-91` `bt_mfr_name` linear scan over 57 entries called per ADV in `ble_scan.cpp:98-100`** as the fallback path after `ble_db_identify` misses. Same sort+binary-search opt as `ble_db_oui`.
- **`ble_scan.cpp:317-336` dual-pass active(8s)+passive(5s) is 13 s — long for the user.** PORKCHOP/Bruce both use single-pass with longer interval/window. Could reduce to active(5s) + passive(3s) = 8 s with same coverage; or expose as user-tunable. Expected gain: 5 s/scan UX.
- **`ble_whisperpair.cpp:425-429` 3 s busy wait** could be turned into ble_gap_event-driven wakeup via NimBLE's mbuf semaphore, but that's a NimBLE host-task refactor — not worth it for current usage.
- **`ble_blueducky.cpp:482-521` `bd_run_payload` loops over script with `strchr` per line.** For multi-KB payloads this is O(n^2). Cache the EOL pointer or pre-tokenize. Expected gain: tens of ms on prank payloads but negligible.

## Refactor targets

- **`ble_whisperpair.cpp` is 715 LOC — split candidate.** Suggested split:
  - `ble_whisperpair_scan.cpp` (lines 124-182, scan callback + target table)
  - `ble_whisperpair_crypto.cpp` (lines 211-318, ECDH + AES + pubkey load)
  - `ble_whisperpair_probe.cpp` (lines 320-451, run_probe + log_verdict)
  - `ble_whisperpair.cpp` (UI + entry points only)

- **`ble_blueducky.cpp` is 610 LOC — split candidate.** Suggested split:
  - `ble_blueducky_keymap.cpp` (lines 108-190, ascii_to_hid + token_to_hid + bd_emit + bd_type_string + bd_exec_line + modbit)
  - `ble_blueducky_hid.cpp` (lines 398-434, bd_setup_hid + report descriptor)
  - `ble_blueducky.cpp` (UI + entry only)

- **`ble_scan.cpp` is 547 LOC** — borderline. Could pull out `classify()` (61 LOC) into `ble_classify.cpp` shared with future features that need the same type-tag (drone_remoteid already does its own version).

- **`ble_db.cpp` 353 LOC lookup table** — the table itself is fine, but the duplicates (defects 6,7) should be deduped and the file annotated with "source: BT SIG yyyy-mm-dd snapshot" so future refreshes are mechanical.

- **`ble_hid.cpp` + `ble_blueducky.cpp` share HID report descriptor, ascii_to_hid, send_key/bd_emit logic.** Promote to a shared `ble_hid_common.cpp` to eliminate ~150 LOC drift.

- **Random-MAC pattern (`mac[5] |= 0xC0; ble_hs_id_set_rnd(mac); NimBLEDevice::setOwnAddrType(...)`)** is duplicated in `ble_spam.cpp:92-106`, `ble_flood.cpp:37-47`, `ble_karma.cpp:43-53`, `ble_findmy.cpp:73-79`, `ble_sourapple.cpp:371-375`. Promote to `ble_random_addr()` helper in a new `ble_helpers.h`.

- **`ble_extras.cpp` mixes tracker + sniffer + iBeacon — 3 features in 1 file (286 LOC).** Split into `ble_tracker.cpp`, `ble_sniff.cpp`, `ble_ibeacon.cpp` for menu-symmetry with other features.

## Backlog items

- **ble-001** [HIGH] `ble_scan.cpp:349-358` — feat_ble_scan bypasses radio_switch(RADIO_BLE). Fix: replace bypass with `radio_switch(RADIO_BLE)`. Effort: S.
- **ble-002** [HIGH] `ble_hid.cpp:186-187` / `ble_blueducky.cpp:407-408` — direct esp_wifi_stop/deinit can double-deinit. Fix: gate on `esp_wifi_get_mode()` ok before call OR centralize via `radio_force_wifi_off()` helper. Effort: S.
- **ble-003** [HIGH] `ble_whisperpair.cpp:626-636` — indefinite scan w/o `setMaxResults(0)` grows internal vector. Fix: add `scan->setMaxResults(0)` before start. Apply same to `ble_extras.cpp:88,211`, `ble_karma.cpp:77`, `ble_toys.cpp:233`, `ble_finder.cpp:323,423`. Effort: S.
- **ble-004** [MED] `ble_flood.cpp:134` — ESC during conn-in-flight may leak conn. Fix: drain loop on exit. Effort: S.
- **ble-005** [MED] `ble_clone.cpp:65` — `mac[5]\|=0xC0` breaks clone of public-MAC targets. Fix: abort with toast on public, OR explicitly copy random target MAC without modification. Effort: S.
- **ble-006** [MED] `ble_db.cpp:18-136` — duplicate / colliding OUI rows. Fix: dedup, pick most-current owner, regenerate with `sort -u`. Effort: S.
- **ble-007** [MED] `sigdb_bt.h:25-83` — duplicate manufacturer IDs (0x0131, 0x0157). Fix: dedup. Effort: XS.
- **ble-008** [MED] `ble_db.cpp:300-305` — O(n) OUI lookup on adv hot path. Fix: pre-sort + binary search. Effort: M.
- **ble-009** [MED] `ble_db.cpp:307-353` — `ble_db_identify` Samsung/MS shortcuts return on prefix-match w/o secondary verification. Fix: tighten manufacturer-data branch decoders. Effort: M.
- **ble-010** [MED] `ble_whisperpair.cpp:685-715` — child-from-target path skips scan-stop on entering scan. Fix: ensure caller-side stop OR add unconditional `scan->stop(); delay(20);` at function head. Effort: S.
- **ble-011** [MED] `ble_whisperpair.cpp:425-429` — 3 s busy wait without WDT reset. Fix: `esp_task_wdt_reset()` per iteration. Effort: XS.
- **ble-012** [MED] `ble_hid.cpp:240-253` / `ble_blueducky.cpp:600-609` — GATT services leak on repeat-entry. Fix: `srv->removeService(s_hid->getHidService(), true)` before delete. Effort: M.
- **ble-013** [LOW] `ble_spam.cpp:172,192` — enum-as-int sentinel pattern. Fix: add `SPAM_NONE=-1` enum value. Effort: XS.
- **ble-014** [LOW] `ble_findmy.cpp:99` — volatile read of stable value per tick. Fix: cache to local. Effort: XS.
- **ble-015** [LOW] `ble_extras.cpp:178-179` vs `ble_scan.cpp:457-459` — MAC byte order inconsistent across CSVs. Fix: canonicalize to display-order MSB-first. Effort: S.
- **ble-016** [LOW] `ble_karma.cpp:116` — connectable advertising w/o GATT server invites empty connects. Fix: switch to `BLE_GAP_CONN_MODE_NON` or add minimal generic service. Effort: S.
- **ble-017** [LOW] `ble_db.cpp:135` — sentinel-terminated table fragile to legitimate 0x000000 OUI. Fix: explicit count. Effort: XS.
- **ble-018** [LOW] Refactor — split `ble_whisperpair.cpp` (715) and `ble_blueducky.cpp` (610) per section above. Effort: M.
- **ble-019** [LOW] Refactor — promote random-MAC helper (`ble_random_addr()` in `ble_helpers.h`). Effort: S.
- **ble-020** [LOW] Refactor — promote shared HID descriptor / ascii_to_hid / emit helpers to `ble_hid_common.cpp`. Effort: M.
- **ble-021** [LOW] Refactor — split `ble_extras.cpp` into per-feature `ble_tracker.cpp`/`ble_sniff.cpp`/`ble_ibeacon.cpp`. Effort: S.
- **ble-022** [LOW] `ble_findmy.cpp:71` — purely random 22-byte keys per rotation; Evil-M5 reads SD-baked keys (`/evil/FindMyEvil_keys.txt`). Fix: optional SD-key load (`/poseidon/findmy_keys.bin`). Effort: M.
- **ble-023** [LOW] PORKCHOP NimBLE-internal-only allocation cflags. Fix: add `-DCONFIG_BT_NIMBLE_MEM_ALLOC_MODE_INTERNAL=1` to platformio.ini and verify PSRAM bypass. Effort: S.
