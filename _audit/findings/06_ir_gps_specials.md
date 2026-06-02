# IR + GPS + Specials Module Audit

## Current state

**IR family (3 features, ~940 LOC + 1028-line data header).** `ir_remote.cpp:152` is the Samsung Smart Remote with single-letter key bindings; carrier is bit-banged 38 kHz on GPIO 44, active-LOW polarity (LOW=on, HIGH=off, idle parked LOW within feature but exits to HIGH-park elsewhere). The file's comment at `ir_remote.cpp:93` explicitly explains why LEDC was abandoned (idle level stuck duty=0 to LOW on active-LOW = stuck-on). `ir_remote.cpp:97` defensively stops three LEDC channels on entry to wipe contamination from `feat_ir_test` (which DOES use LEDC + `output_invert=1` at `ir_remote.cpp:220`). `ir_tvbgone.cpp` cycles 6 baked codes from a FreeRTOS task (`ir_tvbgone.cpp:139`). `ir_clone.cpp:275` provides 3 profile pickers (Samsung/LG/Sony) + 10 prank entry points (`feat_ir_prank_*`) that drive bit-bang via the shared `blast_raw()` and `send_samsung/lg/sony12`.

**GPS (`gps.cpp` 172 LOC).** UART1 on G15-RX/G13-TX, baud cycle of 6 rates (`gps.cpp:15`), GGA+RMC parse only. `gps_snapshot()` (`gps.cpp:59`) gates on `s_fix.valid`, and `parse_rmc()` correctly leaves `.valid` untouched (`gps.cpp:124-129`). Fix ages out after 10s without update (`gps.cpp:169`). `s_pause_poll` allows baud-cycle to fence concurrent polling.

**BadUSB (`badusb.cpp` 413 LOC + `badusb_extras.h` 1389 LOC + `badusb_pranks_data.h` 792 LOC).** ESP32-S3 native USB-HID via `USBHIDKeyboard` (`badusb.cpp:26`). DuckyScript-lite parser at `badusb.cpp:132-167` covers REM/DELAY/STRING/ENTER/TAB/ESC/SPACE/BKSP/GUI/CTRL/ALT/SHIFT/COMBO. CDC exclusivity gate at `badusb.cpp:243` checks both `g_mimir_cdc_active` AND `g_trident_cdc_active`. Per-OS payload libraries plus a categorised pranks browser (`badusb.cpp:391`).

**Specials (drone, surveillance, defensive, triton, trident, mimir, saltyjack).** `drone_remoteid.cpp:227` passive ASTM F3411-22a 0xFFFA scanner with Message-Pack 0xF support (`drone_remoteid.cpp:134`), JSONL to SD with observer GPS. `surveillance_hunter.cpp:264` channel-hops 1-13 in promisc, scoring against `sigdb_surveillance.h` (FLOCK_OUI T1/T2 + SSID patterns + DeFlockJoplin probe heuristic); CSV+JSONL output with deferred-queue draining via main loop (`surveillance_hunter.cpp:154-237`). `defensive_monitor.cpp:637` time-slices WiFi promisc (3s) and NimBLE scan (2s) with 7 detector classes (DEAUTH_FLOOD / EVIL_TWIN / BEACON_SPAM / BCAST_DEAUTH / WIFI_KARMA / BLE_SPOOF / BLE_FLOOD). `triton.cpp:1006` autonomous 4-mode handshake hunter (HUNT/STEALTH/SURGICAL/STORM) with RL channel weighting + Argus mood + parallel wardrive CSV; cooperative tick replaces the older xTaskCreate. `trident.cpp:209` USB-CDC PC bridge with scanline framebuffer streaming (`s_line[240]` at `trident.cpp:22`). `mimir.cpp:553` JSON-over-CDC C2 client for MIMIR drop-box; sets `g_mimir_cdc_active` at entry (`mimir.cpp:555`). SaltyJack subdir is a 6-attack Evil-M5 port: `dhcp_starve` 320 LOC, `dhcp_rogue` (STA+AP, 425 LOC), `responder` (LLMNR+NBNS+SMB NTLMv2, 619 LOC), `wpad` (PAC+407, 437 LOC), `ntlm_crack` (on-device MD4+HMAC-MD5 wordlist, 496 LOC), plus 495-LOC menu + 52-LOC info page.

## Defects

### IR

- `ir_remote.cpp:110` `carrier_on()` writes `digitalWrite(IR_PIN, LOW)` after `pinMode(OUTPUT)`. On the active-LOW LED that's the **LED ON** state until the first `mark()` is called — feature entry leaks ~10-200ms of LED-ON light. Comment at `ir_remote.cpp:228-232` even acknowledges the exit-time bug but the entry path has the same shape. Should park HIGH on entry, mark()/space() then drive LOW only during a mark.
- `ir_remote.cpp:127` `space()` writes `LOW`. Same polarity bug — between bits the LED stays LIT for the entire NEC 1.6 ms-1.7 ms space. Receivers may still demodulate (carrier absent) but board draws ~50× the intended idle current and on a USB-bus-powered Cardputer this is a brownout vector. Verified by reading the comment at `ir_clone.cpp:67-69` about brownouts during 9000us mark. Same defect across `ir_tvbgone.cpp:122` and `ir_clone.cpp:59`.
- `ir_remote.cpp:149` `send_samsung()` parks LOW after final mark — LED stays on between key presses. Same at `ir_remote.cpp:196` (function exit) and `ir_tvbgone.cpp:186`, `ir_clone.cpp:220-221` (`send_btn()` after each press), `ir_clone.cpp:341` (`blast_raw()` exit). The main.cpp boot-time pinparking is HIGH per the map, so these features actively undo that invariant on exit.
- `ir_clone.cpp:152` `SAMSUNG_BTNS` Power cmd is `0x40` — but `ir_remote.cpp:32` comment explicitly says the correct toggle is `0x02` and 0x40 was the old wrong value. The two files disagree on the power code; ir_clone's preset is stale.
- `ir_tvbgone.cpp:155` spawns `xTaskCreate(blaster_task, 3072)` with no failure-handling. If pdFAIL the user sees the UI loop with idle LED but blaster_task never ran. No deauth-style sentinel.
- `ir_tvbgone.cpp:139` `blaster_task` deletes itself on `s_running=false` but the calling `feat_ir_tvbgone()` only `delay(300)` then returns at `ir_tvbgone.cpp:185`. If the task is mid-`blast()` at 8000us mark the polarity-on-exit park at line 186 fires while task is also driving the pin — race window.
- `ir_extras_data.h:1027` is a 1028-line monolith mixing raw codes (sec 1-2), command tables (sec 3-4), AC captures (sec 5), and inline prank drivers (sec 6). The forward decls of `blast_raw`, `send_samsung`, `send_lg`, `send_sony12`, `delay`, `millis` (lines 811-928) are at file scope — including this header in any other TU pollutes that TU. ODR risk if/when refactored.
- `ir_extras_data.h:215` Sharp NEC entry pads `cmd 0x4A = 01010010` but the bit comment says `0 1 0 0 1 0 0 1` — bit layout matches but the comment string is wrong (MSB vs LSB). Cosmetic doc bug.
- `ir_extras_data.h:846` `prank_channel_scramble` fires Samsung+LG Ch+ then `delay(80)` — but `send_samsung()` blocks ~70ms per code, so the actual cadence is ~150 ms not the documented ~80 ms. Mute torture is even worse (`prank_mute_torture` at line 905 — 38 iterations × 800 ms gives ~30 s, matches doc).
- `ir_remote.cpp:115` `mark()` uses `delayMicroseconds(13)` for both half-periods on a 38 kHz target. That's 13+13 = 26 us → 38.46 kHz, fine. But `delayMicroseconds()` is non-preemptible: a single 8000us mark on `lg_power` blocks core-1 for 8 ms, and the FreeRTOS scheduler tick is 10ms — under TCP/wifi load this can drop frames. Comment-only; not a freeze cause but a latency notice.

### GPS

- `gps.cpp:147` `gps_poll()` reads UART byte-by-byte with no max-byte gate. Under sustained 115200 baud with 8 sentences/sec, each call can drain ~1500 bytes and block ~10 ms. Triton calls this every loop. Should cap iteration count per call.
- `gps.cpp:160` overflow path discards entire line silently — a noisy `last NMEA` will look fine in diag even when buffer was overflowing.
- `gps.h:1-13` documents shared UART1 pins (G15 RX, G13 TX) but **G13 is the CC1101 CS pin** per `cc1101_hw.h` and the poseidon map. `gps_end()` is called from `radio_switch(RADIO_SUBGHZ)` per the map's HSPI-park tag, but `gps_end()` only stops the UART (`gps.cpp:29`). The TX pin (G13) was driven HIGH by UART idle and after `s_uart.end()` it's released — that's correct, but there's no `pinMode(13, INPUT)` to actively tri-state it. If the UART driver leaves the pin as OUTPUT-HIGH and CC1101 then drives it LOW for CS-assert, a momentary back-drive happens. Add explicit pin float on `gps_end()`.

### BadUSB

- `badusb.cpp:38` `type_string()` uses `s_kbd.write((uint8_t)*s)` which is HID-USB-keycode interpretation — non-ASCII bytes (>127) get rejected silently. Any extended-ASCII or UTF-8 STRING payload (Latin-1, etc.) drops characters with no error.
- `badusb.cpp:144` `strtok(buf, " ")` for cmd, then `strtok(nullptr, "")` for the rest. The empty-delimiter strtok returns the remainder — but if the line is `STRING   leading-spaces`, the leading spaces get included literally. Acceptable for hashbang scripts but bad for spec compliance.
- `badusb.cpp:124-129` `run_modifier_combo` only supports ONE additional key. `CTRL ALT t` is the only multi-modifier case (special-cased at `badusb.cpp:157`). No general `COMBO` support despite the file comment promising `COMBO CTRL ALT T` at line 16. The `COMBO` keyword is undocumented but doesn't actually exist in the parser; user-facing docs/comment lie.
- `badusb.cpp:120` `kbuf[16]` truncates modifier tail. Any tail >15 chars silently corrupts.
- `badusb.cpp:32` `USB.begin()` is called per-payload but the underlying USB stack only allows one begin per boot. Subsequent feature entries re-trigger enumeration delay (400 ms) — see `delay(400)` at line 34 — visible as a UI freeze on every payload run.
- **No path validation**: README/comment at `badusb.cpp:6` mentions SD `/poseidon/ducky/*.txt` but `feat_badusb` (line 240) does NOT read SD; only built-in payloads. The promised SD path is unused. Either the doc lies or the feature is half-built. Path-traversal isn't possible because the code never opens user-supplied paths.
- `badusb.cpp` doesn't customise USB VID/PID/manufacturer/product strings (no `USB.VID`/`USB.PID`/`USB.productName` calls anywhere in tree per grep). The device enumerates with Espressif defaults — fingerprintable as ESP32-S3-DevKitC, NOT as POSEIDON, which is actually good OPSEC. But the user might WANT to set a Logitech-like descriptor; flag as an OPSEC opportunity.
- `badusb.cpp:243` checks `g_mimir_cdc_active` and `g_trident_cdc_active` — good. But the Trident path at `trident.cpp:211` sets `g_trident_cdc_active = true` BEFORE checking for a host (`Serial` truthy), so any concurrent feature sees CDC-busy even if the desktop side isn't connected. Cosmetic, not data corruption.

### Drone RemoteID

- `drone_remoteid.cpp:140` `(size_t)(m + 25 - data) > len` — pointer arithmetic on `(m + 25 - data)` is the chunk's END offset; check is correct but order of operations means each chunk is 25 bytes including its own header byte. Per F3411-22a §A.2.4 Message Pack the chunk size is 25 (1 header + 24 payload). When `len < 27`, the loop body decodes a partial chunk. Bounds OK but worth a comment.
- `drone_remoteid.cpp:67` `s_mux` is `portMUX_INITIALIZER_UNLOCKED` and held in `decode_astm()` via `portENTER_CRITICAL(&s_mux)` — but `decode_astm` calls `log_event()` while INSIDE the critical section (`drone_remoteid.cpp:197`), and `log_event()` does `s_log.printf` + `s_log.flush()` — FATFS calls under a portMUX. Hardlock risk if SD blocks.
- `drone_remoteid.cpp:108` `log_event` calls `gps_snapshot(&g)` while still inside the NimBLE callback's critical section (called from line 197). GPS snapshot is heap-safe but `Serial.printf` for SD takes the FATFS mutex — under critical section that's a deadlock vector. Same pattern as the surveillance feature got right with deferred queueing; drone_remoteid did NOT adopt that pattern.
- `drone_remoteid.cpp:217` `getBase()->val` direct member access — fine, but the NimBLE 2.x `getAddress()` returns a value type. Lifetime of `getBase()->val` past the call statement is technically UB; copy via memcpy is correct but cosmetic risk.
- `drone_remoteid.cpp:184` Altitude `((float)alt_raw * 0.5f) - 1000.0f` matches F3411-22a §A.2.1.2 only for the pressure-alt field at byte 13-14; the code applies it to byte 15-16 (geodetic alt). Per the comment block lines 167-178 the byte offsets are reversed: the code uses geodetic but the math is intended for one or the other field. Verify against spec.
- GPS-off gate: `log_event` writes `obs_lat`/`obs_lon` only when `gps_snapshot()` true (`drone_remoteid.cpp:108`). PASSES the GPS-off invariant.

### Surveillance Hunter

- `surveillance_hunter.cpp:184` SSID tag-len bound is `tags[1] <= (tag_len - 2)` but `tag_len = len - (tags - p) - 4`. The `-4` is for the FCS trailer; if `sig_len` excludes FCS (driver-dependent), the `-4` is wrong and you may read 4 bytes past the buffer. Worth a comment.
- `surveillance_hunter.cpp:225-236` ISR-side ring writes do NOT take `s_mux`. Concurrent enqueue while the main-loop drain reads `s_hitq_head/tail` is OK because of single-producer-single-consumer pattern but the `s_hitq` cast `((char *)s_hitq[head].ssid)` is writing to volatile struct fields without barriers. ESP32-S3 has no reordering for aligned scalar writes so it works in practice — but flag as fragile.
- `surveillance_hunter.cpp:288` `esp_wifi_set_promiscuous(true)` followed by `esp_wifi_set_promiscuous_rx_cb(promisc_cb)` — the order is BACKWARDS per IDF docs (should set callback first, then enable promisc). Window where promisc is on without callback set could panic if a frame lands.
- `surveillance_hunter.cpp:286` `memset(s_dedup, 0, sizeof(s_dedup));` AFTER `xTaskCreate(hop_task)` would race; but it's BEFORE (line 286 vs 293). OK.
- `surveillance_hunter.cpp:289` `esp_wifi_set_channel(s_current_ch, ...)` is called before `xTaskCreate(hop_task)` — but hop_task ALSO sets channel at 250ms cadence (`surveillance_hunter.cpp:257`). Single-writer pattern is preserved.

### Defensive Monitor

- `defensive_monitor.cpp` 769 LOC — **TOP-3 SPLIT CANDIDATE in bucket** (after triton at 1457). Suggested split: `defensive_monitor.cpp` (WiFi promisc), `defensive_monitor_ble.cpp` (NimBLE callbacks), `defensive_monitor_state.cpp` (ring buffers + log writer).
- `defensive_monitor.cpp:601-635` `enter_wifi_phase()` calls `NimBLEDevice::deinit(true)` then `delay(80)` then `radio_switch(RADIO_WIFI)`. `enter_ble_phase()` does the inverse with `delay(60)`. Reinitializing NimBLE every 5s (`DM_PHASE_WIFI_MS+DM_PHASE_BLE_MS`) is HEAP-EXPENSIVE — NimBLE init eats ~30KB per `[feedback_poseidon_ble_cooperative.md]`. Repeating that init/deinit cycle every 5s grows TLSF fragmentation linearly; long sessions will silently fail one of the inits. PORKCHOP's "never WiFi.mode(WIFI_OFF) mid-run" warning applies symmetrically here.
- `defensive_monitor.cpp:201-228` `enqueue_alert` is called from BOTH the WiFi ISR-class promisc_cb AND the NimBLE main-loop `ble_on_adv` — but the function does NOT use `portENTER_CRITICAL_ISR`. promisc_cb's caller DOES wrap it (`defensive_monitor.cpp:355` enters critical), but `ble_on_adv` (line 268) does not. The ring head/tail volatile uint8_t writes are SPSC-safe per core but interleaved BLE-task + WiFi-ISR enqueues to the same ring are MPSC — not safe.
- `defensive_monitor.cpp:159-163` `find_deauth_src` linear scan with `memcmp` on `volatile dm_deauth_src_t.src` — accessing through volatile pointer-to-array via `memcmp` is technically allowed but UBSAN unhappy. Same pattern at lines 170-178, 181-199, 248-266, 286-313.
- `defensive_monitor.cpp:364` `static const uint8_t k_bcast_zero[6] = {0}` is reasonable for BCAST detection but real Spacehuhn-style attacks use a random BSSID, NOT all-zero. The class will under-trigger on contemporary deauther firmware.
- `defensive_monitor.cpp:484-499` Evil-twin detector marks BSSID-change as an alert AND updates the slot — so the next time the same SSID appears on a third BSSID it re-fires. But it never alerts on the SECOND beacon from the original BSSID after the alert. Detection is one-shot per slot-mutation; rapid AP-clone hopping between 3+ BSSIDs spams the channel.
- `defensive_monitor.cpp:444-466` WiFi-Karma alert tracks `first_seen_ms` for 3s — but the LRU eviction at lines 433-437 may evict the karma-candidate before 3s elapse if 16+ unique probe-resp BSSIDs arrive (which a real Karma attacker generates). Threshold should be 32 not 16, or the LRU should preserve non-alerted slots.
- `defensive_monitor.cpp:680-685` phase-switch check runs every loop iteration (~20 ms). The `delay(80)` inside `enter_wifi_phase` blocks the main loop with `s_running=true` — a key press during the transition is queued in TCA8418 hardware but won't be read until phase setup completes. Cosmetic input lag.
- GPS-off gate: `log_alert_row` calls `gps_snapshot(&g)` (`defensive_monitor.cpp:520`) and writes `g.lat_deg`/`g.lon_deg` even when GPS returned false — writes `0.0,0.0` literally. The check `have_gps ? g.lat_deg : 0.0` (line 530) IS the gate. PASSES, but a missing-GPS log row contains lat=0,lon=0 which downstream tooling may interpret as Null Island. Should write `null` or omit the field per `[feedback_poseidon_gps_off_by_default.md]`.

### Triton

- `triton.cpp` 1457 LOC — **TOP SPLIT CANDIDATE in entire bucket**. Suggested 4-file split: `triton.cpp` (FSM + main loop), `triton_radio.cpp` (promisc cb + EAPOL parser + emit_pmkid/hs), `triton_mood.cpp` (mood state machine + face drawer + voice lines), `triton_brain.cpp` (RL channel weighting + persist).
- **Freeze regression hypothesis (paper-only)**:
  - `triton.cpp:1068` `s_file = SD.open("/poseidon/hashcat.22000", FILE_APPEND)` opens BEFORE the wardrive file (`triton.cpp:332` opens lazily). Both files belong to the same SD volume; if FATFS reaches its max-open-handle limit (4 by default) when a third feature also has a file open, the wdr_flush silently no-ops. Not a freeze cause but a write-loss vector.
  - `triton.cpp:188-198` `capture_enqueue` uses `portENTER_CRITICAL(&s_capq_mux)` inside the WiFi RX callback. This is documented as safe but stacked with `cb` → `handle_eapol` → `emit_hs` → `capture_enqueue`, the callback CRITICAL window holds for the entire 1024-byte strncpy. On a heavy-traffic EAPOL burst this gates WiFi RX for ~50-100us per frame; sustained under STORM mode this could trigger the freeze.
  - `triton.cpp:1068` `s_file` is checked for truthiness at line 223 (`if (s_file)`) but `capture_flush` (called from cooperative tick) does NOT use a mutex. If `feat_triton` exits and closes s_file (line 1452) while a deferred drain from a queued capture runs, we use-after-close. The cooperative tick runs from the SAME task as the close — should be safe — but if `triton_learn_save` at line 1449 yields, races re-open.
  - `triton.cpp:583-588` `triton_pick_channel` reads `s_q[]` without a lock; `triton_reward` (line 142) writes from the WiFi RX callback. Float read-modify-write across cores is NOT atomic. Could explain occasional NaN propagation that wedges `softmax`.
  - `triton.cpp:1112` `esp_wifi_set_max_tx_power(78)` once per session — fine. But `triton.cpp:1117-1123` re-arms promisc + cb + channel in `triton_tick` at line 654-656. Re-registering the cb every burst window churns the IDF callback pointer; comment at line 652 says "no-op" but the cb register may still flush internal state.
  - `triton.cpp:182` `capture_t s_capq[8]` × 1024-byte lines = 8 KB BSS. Combined with `s_emit_line[1024]` + `s_emit_pmk[300]` + `s_bs[24]` + `s_wdr_q[8]` × ~200 bytes the BSS footprint is ~13 KB. Not a freeze cause but a memory-pressure signal.
  - `triton.cpp:1170` `next_tick = millis() + triton_dwell_for_mode(s_mode)` — `triton_dwell_for_mode` reads `s_q[s_ch]` without lock (`triton.cpp:680`). Same float-race as above.
- `triton.cpp:131-138` `triton_learn_save` does NOT check `sd_is_mounted()` before `SD.open` writing the brain file at session-end. If SD was unmounted during the session (e.g. PORKCHOP-style power glitch), this silently fails AND truncates the previous brain.bin to 0 bytes (FILE_WRITE = truncate).
- `triton.cpp:1136` `WiFi.scanDelete()` after `wifi_lean_sta_init` may fight the raw-IDF init — `scanDelete` calls into the Arduino WiFi state machine which expects WiFi.mode() to be set; raw IDF init didn't go through that path. Cosmetic but could trigger an internal assert in some Arduino versions.
- Argus mood sprite draw at `triton.cpp:1250` calls `argus_draw()` which `pushImage`s a 64×64 RGB565 (~8 KB) — and the map flags `argus.cpp` as `[TX-cache]`. The triton main loop redraws Argus inside the 120 ms `last_draw` cadence. During an active deauth burst at TM_STORM (`s_mode == TM_STORM`, 700 ms hunt period) the pushImage can land mid-`esp_wifi_80211_tx` window. Per `[feedback_poseidon_tx_layout_sensitivity.md]` this is the canonical sprite-scramble pattern. Hypothesis: in STORM mode the Argus sprite can scramble during a deauth burst.
- `triton.cpp:332-345` opens `s_wdr_file` lazily inside `wdr_flush` via `sdlog_open` — a SECOND SD file open while `s_file` is also open. Plus the main `s_file` `FILE_APPEND` mode keeps growing. Two concurrent SD writes with `s_capq_mux` + `s_wdr_mux` are MPSC-safe at the queue level but FATFS itself serializes — write throughput is halved. If user runs with GPS fix from session start, parallel CSV writes plus hashcat writes triple the SD load vs handshake-only.
- `triton.cpp:415` `if (m2_len > 256) m2_len = 256` — capping to 256 silently truncates the EAPOL M2 payload for hashcat; real M2 frames can be ~150 bytes so this is fine, but document the choice.
- GPS-off gate: `wdr_append` (line 292) requires `g.valid && g.sats > 0`. PASSES. But `triton.cpp:1107` calls `gps_begin()` unconditionally — could leak GPS UART state if the user runs Triton without a HAT. Cosmetic.

### Trident

- `trident.cpp:35` `readRect(0, y, 240, 1, s_line)` per scanline × 135 scanlines per frame, at 10 fps = 1350 readRects/sec. Each readRect goes through M5GFX → SPI DMA from the ST7789. During an active `esp_wifi_80211_tx` burst this contends for the same FSPI bus that SD also uses (`sd_helper.h` says SD CS=10 / Cardputer internal, SPI shared via `sd_get_spi()`). The map's `[TX-cache]` tag is on trident.cpp for this exact reason.
- `trident.cpp:215` `radio_switch(RADIO_NONE)` parks radio domain. Good — but does NOT call `gps_end()`. GPS UART remains live, consuming a UART driver slot and ~2-3KB of internal buffer.
- `trident.cpp:38` `Serial.write(...480)` sends a single scanline; with 135 scanlines + 1 JSON header per frame at 10 fps that's 1351 Serial.write calls/sec. Each Serial.write blocks until USB-CDC has buffer space; under host backpressure each scanline write can block ~1-5 ms, totaling 135-675 ms per frame. Causes frame-rate degradation but not corruption.
- `trident.cpp:33` `Serial.printf("{\"evt\":\"frame\"...` is fired BEFORE checking if the host is ready. If host disconnects mid-frame the header is sent but bytes aren't; protocol desync — host parser will treat next JSON line as RGB565 binary. No protocol fence (magic byte / length-prefix).
- `trident.cpp:111` `g_wdr_ap_count` extern read without lock — wardrive task may be concurrently mutating. Stale read OK; off-by-one count is cosmetic.
- `trident.cpp:144` `if (!sd_mount())` re-tries on every `loot` command — but does NOT call `sd_remount` on failure. If SD was unmounted by another feature mid-session, the loot command silently fails.
- `trident.cpp:157-176` Loot streaming uses Arduino `String row` with `+= (char)c` per byte → O(n²) reallocation. A 32KB credential log = 32K reallocs each growing by 1. Will block for seconds and may exhaust heap.
- `trident.cpp:211` `g_trident_cdc_active = true` set without checking `g_mimir_cdc_active` first. If user enters Trident while Mimir owns CDC, both flags get set true — BadUSB will refuse but neither owner detects the conflict. Should mirror badusb.cpp's check.
- `input_inject` ring overflow: `input.cpp:31` `s_injected[16]` ring, `input_inject` drops on overflow (`input.cpp:36-39`). Trident's `key` command calls `input_inject` unconditionally with no flow-control back to host — if host fires keys faster than the local feature consumes them, half drop silently. Trident should report drop-rate back to host.
- `trident.cpp:54-58` Maps "up"/"down"/"left"/"right" → ';' / '.' / ',' / '/' per the comment. Documented in code; good fix vs the older PK_UP/DOWN injection that the menu didn't consume.

### Mimir

- `mimir.cpp` 745 LOC — split candidate: `mimir.cpp` (UI + state machine), `mimir_proto.cpp` (JSON parser + command senders), `mimir_screens.cpp` (per-screen drawers).
- `mimir.cpp:62-87` Hand-rolled JSON parser: `json_str` uses `strstr` over the line buffer — pattern `\"key\":\"` will match across nested objects. E.g. if MIMIR sends `{"evt":"ap","ssid":"some {\"evt\":\"hello\"} thing"}` the `json_str(buf, "evt", ...)` matches the inner escaped string. Acceptable for trusted protocol; flagged.
- `mimir.cpp:583` `if (!Serial)` — same caveat as Trident: on ESP32-S3 USB-Serial-JTAG, `Serial` evaluates false until host opens port. The mimir feature DOES break out (line 583 `break`) before host connects, opposite to Trident's wait-for-host pattern. So entering Mimir before plugging USB-C-C to MIMIR brick will exit immediately with "disconnected" toast.
- `mimir.cpp:551` `g_mimir_cdc_active = false` global default; `mimir.cpp:555` sets true on entry. But `feat_mimir` does NOT check `g_trident_cdc_active` before setting — concurrent Mimir+Trident entry sets both. BadUSB's check catches the race but Mimir/Trident themselves don't.
- `mimir.cpp:741` `yield()` — comment says "delay(5) drops CDC frames". OK choice but yield with no delay risks runaway CPU when host idle.
- `mimir.cpp:184-194` `evt:ap` handler indexes `s_targets[s_target_count++]` with no per-field length check beyond JSON-string's own `out_sz`. Crafted MIMIR responses could grow the SSID to 32 bytes (fine — `sizeof(a.ssid) == 33`).
- `mimir.cpp:646-654` `MS_TARGETS` ENTER branch rebuilds the filter index every keypress (O(64×strncasecmp)). Trivial cost but cosmetic.
- `mimir.cpp:118` `send_select` injects `bssid` directly into JSON template — no escaping. MIMIR controls its own BSSID strings so safe in practice, but if `s_targets[].bssid` ever held a quote (e.g. from a corrupted event), Mimir would send malformed JSON to MIMIR.
- No SD-side gating: Mimir doesn't write any local SD log of its session — all state lives on the MIMIR drop-box. If MIMIR disconnects mid-attack the on-device record is gone.

### SaltyJack

- `saltyjack_responder.cpp:505` `pkt = (uint8_t *)malloc(smb_len)` for up to 4 KB per SMB session. Under heap pressure (e.g. concurrent NimBLE) this can fail; the fallback at line 506 closes the session. OK.
- `saltyjack_responder.cpp:425` `s_smb.session_id = ((uint64_t)esp_random() << 32) | esp_random()` — `esp_random()` returns uint32, but the shift `(uint64_t)esp_random() << 32` is fine. session_id collision after ~2^32 sessions; not a realistic concern.
- `saltyjack_wpad.cpp:178-185` allocates `proof_hex` + `blob_hex` separately for hashcat formatting; the comments at `responder.cpp:215` correctly cap nt_resp_len at 1024 but `wpad.cpp:163` only checks `if (nt_len < 16) return` — no upper bound. A 65535-byte nt_len triggers ~130KB allocation that fails on no-PSRAM S3, returning silently with no hash saved. Mirror responder's cap.
- `saltyjack_dhcp_starve.cpp:260` `s_udp.begin(68)` after `esp_netif_dhcpc_stop` — but if `dhcpc_stop` was never `dhcpc_start`ed (user wasn't on DHCP-assigned WiFi), the start at line 310 returns error and the next reconnect won't auto-renew. Edge case, acceptable.
- `saltyjack_dhcp_rogue.cpp:287` `WiFi.softAP("POSEIDON-SaltyJack")` — **uses Arduino softAP, NOT the raw-IDF AP recipe** per `[feedback_poseidon_wifi_ap_recipe.md]`. Map confirms saltyjack_dhcp_rogue.cpp tagged WiFi-AP-IDF in the [GOOD] column, but the actual code at line 287 calls the Arduino path. This contradicts the map; verify which one is right by checking if it stably AP-mode boots under Bruce libs. Hypothesis: this is the source of intermittent AP-mode rogue-DHCP failure.
- `saltyjack_wpad.cpp` similarly does NOT stand up its own AP — relies on STA mode already being joined (`saltyjack_wpad.cpp:377`). No Arduino-softAP issue here.
- `saltyjack_responder.cpp:547` `beginMulticast(IPAddress(224,0,0,252), 5355)` — multicast subscription requires IGMP join, which Arduino WiFiUDP handles. But if AP-mode is the underlying L2 (not STA), the multicast routing differs. Responder explicitly checks `WL_CONNECTED` at line 532 so it only runs from STA — good.
- `saltyjack_menu.cpp:494` `delay(20)` in idle loop — `SJ_IDLE_MS=30000UL` (`saltyjack_menu.cpp:168`); idle-screen-saver runs continuous frames at 50ms cadence. Comment-only.
- `saltyjack_splash.h` 2709 LOC contains a 240×135 RGB565 baked PNG sprite (~65 KB rodata). Flash budget concern; verify partition headroom.
- `saltyjack_ntlm_crack.cpp:222` footer says `ret=skip` but the parser handles `PK_ENTER` (line 352) — `ret` here is PK_ENTER, OK.
- `saltyjack_ntlm_crack.cpp:316` `String upper_user = user; upper_user.toUpperCase();` — Arduino String allocates from heap. Each crack_one call allocates 4-5 Strings; for a multi-hash session this is ~40 reallocs / hash. Acceptable.

## Cross-ref deltas

### vs Bruce

| Feature | POSEIDON | Bruce | Verdict | Notes |
|---|---|---|---|---|
| IR TV-B-Gone | 6 brands (Sony/Samsung/LG/Panasonic/Philips/Vizio) bit-banged | `TV-B-Gone.cpp:149`, `WORLD_IR_CODES.h` (entire world set) | Bruce-WINS | Bruce ships full WORLDcodes; we have a condensed 6 + 27 in `IR_EXTRA_POWER_TABLE`. |
| IR clone (RX capture) | v2-TODO per `ir_clone.cpp:11` | `ir_read.cpp:78` (works) | Bruce-WINS | We lack RX entirely. |
| IR remote (TX) | Samsung-only Smart Remote | `custom_ir.cpp:286` NEC/RC5/RC6/Samsung helpers | Bruce-WINS | We don't expose generic NEC/RC5/RC6 senders. |
| IR jammer | absent | `ir_jammer.cpp:698` `startIrJammer` | Bruce-only | |
| BadUSB USB-HID | DuckyScript-lite, per-OS libs, pranks browser | `ducky_typer.cpp:454` mquickjs JS bindings | tie-with-bias | Bruce has JS scripting; we have curated payloads. |
| Drone RemoteID | passive ASTM listener | absent | POSEIDON-only | |
| Surveillance hunter | Flock + Raven sigdb | absent | POSEIDON-only | |
| Defensive monitor | 7-class detector | partial (deauth detect inside Karma) | POSEIDON-WINS | |
| Triton (autonomous hunter) | 4-mode + RL + mood | absent (no equivalent) | POSEIDON-only | Bruce has no autonomous gotchi. |
| Trident (PC bridge) | USB-CDC framebuffer + keys | absent | POSEIDON-only | |
| Mimir (drop-box C2) | JSON-over-CDC | absent | POSEIDON-only | |
| SaltyJack DHCP/Responder/WPAD | full port | `DHCPStarvation.cpp` (wired W5500), `responder.cpp:113`, no WPAD | POSEIDON-WINS-WiFi | Bruce's class is wired (W5500), we're WiFi-side. Bruce has NetCut (ARP poison) we don't. |

### vs Ghost ESP

Ghost stack is ESP-IDF/LVGL on original Cardputer (not Adv). Nothing in this bucket overlaps significantly:
- IR / Drone-RID / BadUSB / Triton / Trident / Mimir / SaltyJack — ALL absent in Ghost.
- Ghost has a `pineap` rogue-AP detector (`wifi_manager.c:174`), `capture -pwn` pwnagotchi-detect (`callbacks.c:484`), and BLE skimmer detection (`ble_manager.c:117`). POSEIDON's `defensive_monitor.cpp` covers similar ground but does NOT detect pwnagotchi presence specifically — backlog opportunity.
- Ghost has WiGLE-format BLE wardrive (`callbacks.c:769`). POSEIDON wardrives WiFi only; drone_remoteid logs JSONL not WiGLE-CSV.

### vs PORKCHOP

Triton freeze cross-ref (paper hypotheses, per spec):
- PORKCHOP runs ALL attack modes from cooperative `update()` ticks, no `xTaskCreate` for attacks (`network_recon.cpp:946`, `porkchop.h:90`). POSEIDON Triton already migrated to cooperative tick (`triton.cpp:568` comment), GOOD match.
- PORKCHOP's **80KB reservation fence** before `WiFi.mode(WIFI_STA)` (`main.cpp:59-87`) forces WiFi driver buffers high in heap, leaving large contiguous below. POSEIDON's `wifi_lean_sta_init` (`triton.cpp:1038`) uses shrunk DMA buffers but does NOT pre-allocate-and-free the fence. Could explain why Triton works initially then drifts as TLSF fragments.
- PORKCHOP `heap_gates.h` / `heap_policy.h` (`kMinHeapForTls=35000`, `kPressureLevel3Free=30000`) — POSEIDON has no equivalent heap-pressure gating. Triton's `s_capq[8]×1024 = 8KB BSS` + `s_emit_line[1024]` + ~5KB BS cache sit in heap-or-BSS unconditionally. No back-pressure when free heap drops.
- PORKCHOP `HeapHealth::persistWatermarks` (`heap_health.h:50`) — POSEIDON's `TRITON_RAM_PROBE` macro (`triton.cpp:1008`) logs heap snapshots but doesn't persist watermarks across boots. Adding watermark-persist would surface the freeze trigger window without running a serial monitor.
- PORKCHOP explicitly bans `WiFi.mode(WIFI_OFF)` mid-run; uses NimBLE deinit + WiFi off in `defensive_monitor` `enter_*_phase` (`defensive_monitor.cpp:608, 623`). Doesn't match Triton (Triton stays WiFi-only) so not directly relevant — but flags `defensive_monitor` as inheriting PORKCHOP's documented FAIL pattern.
- PORKCHOP `NetworkRecon::isHeapStable()` thresholds at largest-block > 50KB (`network_recon.cpp:946`); Triton has no such gate. Would help: refuse STORM mode entry if largest-block < 40KB.

### vs Evil-M5

SaltyJack ports 6 specific attacks (per saltyjack.h:34-40). The evilm5 map enumerates Evil-M5's broader LAN suite (lines 13-35). **POSEIDON-lacks vs Evil-M5**:
- **Network Hijack combo** — Evil case 50 `networkHijacking` orchestrates DHCP+DNS+WPAD chain. SaltyJack has the pieces but no "fire-all" wrapper. **slt-002**.
- **DHCPAttackAuto** — Evil case 51 picks starve-or-rogue based on detected server. **slt-003**.
- **Switch DNS** flip — Evil case 49 toggles captive DNS AP↔STA bind (`Evil-Cardputer-v1-5-2.ino:17985`). SaltyJack has no captive DNS at all. **slt-004**.
- **SSDP fake/poisoner** — Evil case 71 `fakeSSDP` w/ 300 fake UPnP devices (`:27437`). SaltyJack absent (POSEIDON's `net_ssdp.cpp` is a SCANNER not a poisoner, per the map). **slt-005**.
- **UPnP NAT abuse** — Evil cases 77/78 `listUPnPMappings` + `upnpTargetNATWorkflow`. **slt-006**.
- **NTLM hash de-dup** — Evil case 62 `CleanNTLMHashes`. SaltyJack appends only, no dedup pass. **slt-007**.
- **ARP table scan integration** — Evil's `read_arp_table` (`:13509`) feeds selective targeting. SaltyJack responder is broadcast-only — no selective LLMNR replies. **slt-008**.
- **PCAP logging** for DHCP/responder/WPAD — Evil's `pcap_hdr_t` (`:10696`). SaltyJack writes hashcat-text only. **slt-009**.
- **Cookie siphon** (`/siphon`,`/logcookie`) — Evil pairs WPAD with cookie capture. **slt-010**.
- **mDNS poisoner (udp/5353)** — ABSENT in BOTH Evil-M5 AND POSEIDON. Shared gap. macOS+iOS unaffected by responder. **slt-011** (we_lack opportunity).
- **Web admin / remote review** — Evil exposes captured creds via HTTP dashboard (default password `7h30th3r0n3`). SaltyJack info page is on-device only. **slt-012**.
- **Skyjack (Parrot drone)** — Evil case 72 (`:27718`). POSEIDON has drone_remoteid passive listener but no Parrot SSID hunt. **slt-013**.
- **IMSI Catcher** — Evil's opportunistic IMSI log (`/evil/IMSI-catched.txt`). POSEIDON-absent. **slt-014**.

## Optimisation opportunities

- **opt-1**: BSSID→SSID cache in `triton.cpp:171` (`s_bs[24]`) — bump to 64 with LRU eviction. Current 24 fills in dense environments and subsequent EAPOLs lack ESSID hex for hashcat.
- **opt-2**: `surveillance_hunter.cpp:69` `s_dedup[32]` is a linear-scan ring; replace with a hash-bucketed table (8 buckets × 4 slots). Current scan is ~30 memcmp/frame in dense areas.
- **opt-3**: `defensive_monitor.cpp` time-slice with WiFi 3s/BLE 2s = constant NimBLE init/deinit churn. Switch to NimBLE passive scan that runs alongside promisc with phase-spike avoidance (BLE on advertising channels 37/38/39 ≈ WiFi ch 1/6/11 — explicit avoidance window).
- **opt-4**: `trident.cpp:33` Serial.printf JSON header per frame uses ~50 bytes. Batch multiple scanlines per Serial.write call (8 lines = 3840 bytes) to amortise USB-CDC framing cost. Should ~3× the achievable framerate.
- **opt-5**: `ir_extras_data.h:766` `IR_EXTRA_POWER_TABLE` shares pointers (Insignia/Element/Westinghouse all point to `insignia_power`); good. But `IR_EXTRA_POWER_N=27` blast cycle takes ~3 seconds per `prank_power_bomb`. Add a "fast" mode that fires only the top-5 most-likely brands for quick-fire scenarios.
- **opt-6**: `mimir.cpp:570-585` `pump_rx` and main UI redraw run on a single tick. Decouple: dedicate the loop to JSON drain and run redraw on a 100ms interval. Currently JSON arrives are gated by the redraw cadence.
- **opt-7**: `saltyjack_ntlm_crack.cpp:282` per-line allocations (`blob`, `msg`, `idbuf`) malloc/free every line. Hoist to a single per-feature scratch buffer (1KB) — saves ~3000 heap ops per hash line.
- **opt-8**: `gps.cpp:147` cap `gps_poll` to drain N bytes max per call (e.g. 256). Currently can block 10+ ms under 115200 baud with full sky.

## Refactor targets

- **rf-1** `triton.cpp` — 1457 LOC. **TOP PRIORITY**. Split into: `triton.cpp` (FSM + UI loop), `triton_radio.cpp` (cb + emit_pmkid/emit_hs), `triton_mood.cpp` (mood + face + voice), `triton_brain.cpp` (RL + persist).
- **rf-2** `defensive_monitor.cpp` — 769 LOC. Split: `defensive_monitor.cpp` (WiFi promisc + UI), `defensive_monitor_ble.cpp` (NimBLE adv callbacks + ble_window_tick), `defensive_monitor_state.cpp` (alert ring + LRU tables).
- **rf-3** `mimir.cpp` — 745 LOC. Split: `mimir.cpp` (state machine + UI loop), `mimir_proto.cpp` (json_str/int/bool + command senders + handle_line), `mimir_screens.cpp` (draw_main/targets/attack/live/status).
- **rf-4** `saltyjack_responder.cpp` — 619 LOC. Split: `saltyjack_responder.cpp` (entry + UI), `saltyjack_responder_smb.cpp` (SMB2 state machine + NTLM Type-2 builder + Type-3 extractor), `saltyjack_responder_namesvc.cpp` (LLMNR + NBNS handlers).
- **rf-5** `ir_extras_data.h` — 1028 LOC monolith. Split into: `ir_codes_tvs.h` (sections 1-3), `ir_codes_projector_soundbar.h` (sections 2 bottom + 4), `ir_codes_ac.h` (section 5), `ir_pranks.cpp` (section 6 as real .cpp not inline). Header inlines + extern forward-decls of `delay`/`millis` (lines 813, 928) are ODR-fragile.
- **rf-6** `badusb_extras.h` — 1389 LOC. Split per-OS into `badusb_win.h`, `badusb_mac.h`, `badusb_linux.h`, `badusb_android.h`, `badusb_chromeos.h`. The `payload_t` struct is shared (defined in `badusb.cpp:47`) so headers depend on that — acceptable convention but split cleans includes.
- **rf-7** `saltyjack_ntlm_crack.cpp:43-138` — embedded MD4 implementation (~100 LOC). Move to `crypto_md4.cpp` for reuse if any other future feature needs MD4.

## Backlog items

- **ir-001** HIGH `ir_remote.cpp:127` — `space()` writes LOW = LED-ON between bit-pairs (active-LOW polarity inversion). Fix: `digitalWrite(IR_PIN, HIGH)` in `space()` and at all park sites (`ir_remote.cpp:149,196`, `ir_tvbgone.cpp:122,186`, `ir_clone.cpp:59,220,341`). Verify with phone-camera test. Effort: S.
- **ir-002** MED `ir_clone.cpp:152` Samsung Power cmd `0x40` is stale per ir_remote's own comment (`ir_remote.cpp:32`). Update SAMSUNG_BTNS[0].cmd to `0x02`. Effort: S.
- **ir-003** LOW `ir_extras_data.h` — split monolith into 4 files per rf-5. Effort: M.
- **ir-004** LOW `ir_tvbgone.cpp:155` — check xTaskCreate return; toast on pdFAIL. Effort: S.
- **gps-001** MED `gps.cpp:147` — cap `gps_poll` byte-drain to ≤256 per call to bound block time. Effort: S.
- **gps-002** LOW `gps.cpp:29` `gps_end` — explicitly `pinMode(GPS_UART_TX_PIN, INPUT)` to tri-state vs CC1101 CS contention. Effort: S.
- **gps-003** LOW `gps.cpp:160` — surface overflow event in `s_diag` so wedged baud is debuggable. Effort: S.
- **bdu-001** MED `badusb.cpp:240` — implement promised SD `/poseidon/ducky/*.txt` loader; gate on a SD path-traversal sanitiser (no `..`, no absolute). Effort: M.
- **bdu-002** LOW `badusb.cpp:120` — bump `kbuf[16]` to 64, surface truncation. Effort: S.
- **bdu-003** LOW `badusb.cpp:38` — switch `type_string` to multibyte-aware iteration so STRING payloads with extended-ASCII work. Effort: M.
- **bdu-004** LOW `badusb.cpp` — implement real `COMBO` keyword per file-comment line 16; current parser ignores it. Effort: S.
- **bdu-005** LOW OPSEC: optionally set fake USB VID/PID/manufacturer via `USB.VID(0x046D)` etc., gated by a settings menu toggle. Effort: S.
- **drn-001** HIGH `drone_remoteid.cpp:197` — move `log_event` OUT of `portENTER_CRITICAL(&s_mux)` (lines 149-195); use deferred queue like surveillance_hunter. SD write under portMUX is a hardlock vector. Effort: M.
- **drn-002** MED `drone_remoteid.cpp:184` — verify altitude offset against F3411-22a §A.2.1.2; doc claims geodetic alt at bytes 15-16, code reads bytes 15-16 with pressure-alt scaling. Effort: S.
- **srv-001** MED `surveillance_hunter.cpp:288-289` — set rx_cb BEFORE enabling promisc. Effort: S.
- **srv-002** LOW Add BLE side (RAVEN_BLE manufacturer-ID 0x09C8 + RAVEN_UUID_16) — surveillance_hunter is WiFi-only per `surveillance_hunter.cpp:14-17` deferred. Effort: L.
- **srv-003** LOW Add Plume tooling-compatible JSONL `lat`/`lon` `null` instead of 0.0 when no fix. Effort: S.
- **dfn-001** HIGH `defensive_monitor.cpp:201` — `enqueue_alert` MPSC race between WiFi-ISR promisc_cb and NimBLE-task ble_on_adv. Add `portENTER_CRITICAL` around `ble_on_adv`'s enqueue call. Effort: S.
- **dfn-002** HIGH `defensive_monitor.cpp:608` — NimBLE init/deinit every 5s churns heap; switch to coexist pattern OR widen phase windows to ≥30s. Heap-fragmentation freeze candidate. Effort: M.
- **dfn-003** MED Split per rf-2. Effort: M.
- **dfn-004** LOW `defensive_monitor.cpp:364` zero-MAC BCAST detection misses random-MAC deauthers. Add `addr2 == FF:FF:FF:FF:FF:FF` and broadcast-RA heuristic. Effort: S.
- **tri-001** HIGH `triton.cpp` — split per rf-1. Effort: L.
- **tri-002** HIGH Triton freeze hypothesis cluster — paper-only per spec:
  - `triton.cpp:583-588` float RMW on `s_q[]` across cores → NaN risk, wedges softmax
  - `triton.cpp:188-198` long portENTER_CRITICAL in WiFi RX cb → RX queue overflow under STORM
  - `triton.cpp:1250` Argus pushImage during deauth burst → TX-cache scramble (matches `[feedback_poseidon_tx_layout_sensitivity.md]`)
  - `triton.cpp:1068` `s_file` FILE_APPEND + lazy `s_wdr_file` open = 2 concurrent FATFS handles + hashcat keeps growing
  - Effort: M to validate, S to apply each fix individually per `[feedback_careful_iteration.md]`.
- **tri-003** MED `triton.cpp:131` `triton_learn_save` truncates brain.bin on SD-fault. Use FILE_APPEND-then-rename or check size before commit. Effort: S.
- **tri-004** MED Add PORKCHOP-style 80KB reservation fence before `wifi_lean_sta_init` in main.cpp (not strictly in scope but Triton would benefit). Effort: M.
- **tri-005** LOW Bump `BS_N` from 24 to 64 (`triton.cpp:172`). Effort: S.
- **trd-001** HIGH `trident.cpp:35` readRect×135 + Serial.write×135 per frame at 10 fps under TX-cache contention. Batch scanlines (8 per Serial.write) per opt-4. Effort: M.
- **trd-002** MED `trident.cpp:211` — check `g_mimir_cdc_active` before claiming CDC. Effort: S.
- **trd-003** MED `trident.cpp:157` loot streaming uses Arduino String += per byte. Switch to fixed scratch buffer + line-batched send. Effort: S.
- **trd-004** LOW input_inject ring overflow drop: bump `s_injected[16]` to 32 (`input.cpp:31`) AND report drop count to Trident host on next `status`. Effort: S.
- **mim-001** MED Split per rf-3. Effort: M.
- **mim-002** MED `mimir.cpp:553` — check `g_trident_cdc_active` before setting `g_mimir_cdc_active=true`. Effort: S.
- **mim-003** LOW `mimir.cpp:62-87` — guard JSON parsers against nested-escape false matches; document protocol assumption. Effort: S.
- **slt-001** HIGH `saltyjack_dhcp_rogue.cpp:287` — `WiFi.softAP()` is the Arduino path which `[feedback_poseidon_wifi_ap_recipe.md]` says is BROKEN under Bruce libs. Migrate to raw-IDF AP recipe (see `wifi_portal.cpp`/`evil_twin.cpp` for the canonical pattern). Effort: M.
- **slt-002** HIGH Implement Network Hijack combo wrapper (DHCP + DNS + WPAD orchestrator). Effort: M.
- **slt-003** MED Implement `DHCPAttackAuto` — detect real server, pick starve-vs-rogue automatically. Effort: M.
- **slt-004** MED Implement captive-DNS resolver (wildcard or per-name spoof). Per Evil-M5 case 49 + the broader gap noted in evilm5 map. Effort: M.
- **slt-005** MED Implement SSDP poisoner (fake-300 UPnP devices). Effort: L.
- **slt-006** LOW Implement UPnP NAT abuse (`listUPnPMappings` + `upnpTargetNATWorkflow`). Effort: L.
- **slt-007** LOW `saltyjack_ntlm_crack.cpp` — add `CleanNTLMHashes` dedup pre-pass. Effort: S.
- **slt-008** MED Integrate ARP-scan targeting from a future `lwip/etharp.h` reader; responder selective replies. Effort: M.
- **slt-009** MED Add PCAP logging for DHCP/responder/WPAD flows. Effort: M.
- **slt-010** LOW Add cookie-siphon endpoint to WPAD/portal HTTP server. Effort: S.
- **slt-011** MED **NEW we_lack opportunity** — mDNS poisoner (udp/5353). ABSENT in both POSEIDON and Evil-M5. Builds on responder code. Effort: M.
- **slt-012** LOW Web admin / remote review HTTP dashboard for captured creds + hashes (Evil-M5 has this with default password `7h30th3r0n3`). Effort: L.
- **slt-013** LOW Skyjack Parrot drone SSID hunter. Effort: S.
- **slt-014** LOW IMSI catcher (opportunistic). Effort: M.
- **slt-015** MED `saltyjack_wpad.cpp:163` — cap nt_len at 1024 to mirror `responder.cpp:215` defence. Effort: S.
- **slt-016** LOW `saltyjack_responder.cpp:619 + wpad.cpp:437 + dhcp_rogue.cpp:425` — each over 400 LOC; consider per-attack split if either grows further. Effort: M each.
