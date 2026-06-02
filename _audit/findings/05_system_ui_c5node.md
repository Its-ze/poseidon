# System / UI / c5_node Module Audit

## Current state

The system / UI / c5_node bucket spans **5,923 LOC of S3 code** (18 .cpp)
plus **1,759 LOC of pure-C ESP-IDF firmware** for the C5 satellite
(8 .c/.h). Entry points: boot lives in `main.cpp:73` (IR park → SX1262
RST hold → `M5Cardputer.begin()` → TCA8418 driver swap → SD mount →
sfx/theme init → GPS UART + background poller + IR watchdog tasks →
serial test harness → `ui_init` → optional autotest → `ui_splash`).
`loop()` (`main.cpp:184`) just calls `menu_run()`. Menu tree is the
compile-time static `MENU_ROOT_CHILDREN[]` (`menu.cpp:935`); the
`menu_registry.cpp` self-registration class is fully built but **never
called from anywhere** — `MenuRegistry::build()` and `::root()` have zero
references repo-wide. Runtime renderers: `draw_menu`+`run_submenu`
(`menu.cpp:1081/1262`, terminal style) and `carousel_run_submenu`
(`menu_carousel.cpp`), dispatched by `menu_style_get()` NVS pref.
Keypress entry into a feature stashes `g_current_feature_item`
(`menu.cpp:1185`) so the IR-watchdog can recognise IR features by label.

UI primitives in `ui.cpp` (933 LOC): status-bar gradient with C5 satellite
badge, cached on `(radio, extra, heap_bucket, c5_n)` to avoid flicker
(`ui.cpp:154-170`); footer, toast, body clear with strobe-suppression
heuristic (`ui.cpp:64-78`); slide / spinner / ripple / waves / radar /
hexstream / glitch / eq_bars / dashboard_chrome / action_overlay /
matrix_rain animations. Theme system (`theme.cpp`, 191 LOC) holds six
RGB565 palettes (POSEIDON / MATRIX / E-INK / SYNTHWAVE / PHANTOM / BLOOD),
NVS-persisted in namespace `pui` key `theme`; `theme_preview()` is a
RAM-only hot-swap to avoid flash thrash during arrow-browse.
`ui_ambient.cpp` (153 LOC) draws per-theme ambient backgrounds (TRON
grid+motes+packet for POSEIDON, matrix rain for MATRIX, no-op for E-INK).
Input (`input.cpp`, 144 LOC) reads `M5Cardputer.Keyboard` (TCA8418
driver), injects keys via 16-entry ring from TRIDENT, calls `sfx_click`
on every press, and tracks `s_last_input_ms` for screensaver idle.

Screensaver (`screensaver.cpp`, 1,090 LOC) has 10 in-file painter
functions dispatched from `s_pool[]` (`:1020`); each holds the full
screen and exits on `input_poll() != PK_NONE`. SFX (`sfx.cpp`, 238 LOC)
is a tone-only library on `M5Cardputer.Speaker.tone()` with `delay()`
sequencing — non-blocking is **not** what it does. Argus (`argus.cpp`,
175 LOC) renders 48×48 RGB565 mood sprites with one cached SRAM copy
plus optional procedural overlays (lightning / zzz / scan-line /
glitch). Splash (`splash.cpp`, 148 LOC) is a Hokusai Great Wave fade
with magenta sweep and matrix-rain side-gutters — NOT the trident sprite
the map describes. Radio domain switcher (`radio.cpp`, 179 LOC) is
straightforward: teardown previous → bring up new; `wifi_lean_sta_init`
runs the raw-IDF init with shrunk buffers (4 TX / 4 RX, no AMPDU/AMSDU)
sized for the cramped ~52 KB DMA budget. `sd_helper.cpp` (142 LOC) owns
a dedicated HSPI `SPIClass` and walks 20/10/4 MHz mount fallbacks;
`sd_format` walks-and-unlinks (no real FAT format). `hat_manager.cpp`
(64 LOC) — header advertises probe-based detection but the implementation
returns `HatType::NONE` unconditionally (`hat_manager.cpp:34`). The
`deauth_autotest.cpp` (156 LOC) is a dev-only 3-stage stress test gated
on `-DPOSEIDON_AUTO_DEAUTH_TEST`; main.cpp calls it before splash.
`serial_test.cpp` (71 LOC) always launches a 4 KB FreeRTOS task that
listens for `K<hex>`/`S`/`R`/`?` over USB CDC.

The **C5 satellite firmware** (pure-C IDF, `c5_node/main/`) is a single
`app_main` (`main.c:215`) that inits WiFi STA+`country='US'`+band AUTO,
sets per-band protocol bitmaps, locks ESP-NOW to ch 1 for HELLO sync,
then dispatches CMD_* opcodes received over ESP-NOW broadcast. Wire
protocol v3 (`proto.h`) defines `posei_msg_t` (packed header + 230 B
payload + len) and 16 packed payload structs; magic `POSEI_MAGIC = "POSE"
(0x504F5345)`, version 3, types 1/10-26/40-49. Implementations:
`wifi_scanner.c` (112 LOC, scan), `wifi_attacker.c` (200 LOC, 5 GHz
targeted/broadcast deauth via `esp_wifi_80211_tx(WIFI_IF_STA,...)`),
`pmkid_capture.c` (234 LOC, promisc EAPOL-Key M1 KDE parse),
`hs_capture.c` (276 LOC, full M1+M2 with replay-counter pairing),
`zb_sniffer.c` (127 LOC, IEEE 802.15.4 raw frames via
`esp_ieee802154_receive_done` ISR callback) and `led_fx.c` (216 LOC,
WS2812 RMT controller with 6 modes on a separate FreeRTOS task).
`g_pause_hello` (`main.c:67`) freezes the HELLO beacon during scan/
capture/attack so the C5 radio can hop or sit on a remote channel
without ESP-NOW slamming it back to ch 1.

## Defects

1. **[CRIT]** `main.cpp:63-64,80-81,138-139` — IR LED is parked **LOW**
   at boot, then re-parked LOW on every watchdog tick (`:64-67`). Per
   memory `feedback_poseidon_ir_led_active_low.md` and confirmed by
   `ir_remote.cpp:97-110` (`carrier_on` writes LOW + comment "active-LOW
   LED: HIGH = OFF"), the wiring is anode→3V3, cathode→GPIO 44 — so
   **LOW = LED ON**. Comments throughout `main.cpp:75-79` say "park
   HIGH" but the code writes LOW. Result: the IR LED is silently lit
   from first instruction until forever, drawing current and visible on
   phone camera. The `feat_ir_test` diagnostic in `ir_remote.cpp` exists
   precisely because of this confusion. **Fix:** flip every
   `digitalWrite(44, LOW)` outside of an active mark to
   `digitalWrite(44, HIGH)`; same in `menu.cpp:1323,1357` and
   `menu_carousel.cpp:377`. Effort: S.

2. **[CRIT]** `menu_registry.cpp` (entire 201 LOC) + `menu_registry.h`
   (80 LOC) — Dead code. No call to `MenuRegistry::add`, `::add_submenu`,
   `::build`, or `::root` anywhere outside the file. The
   `REGISTER_FEATURE` macro is unused. The map's claim that POSEIDON has
   "dynamic self-reg" is wrong. The whole pair is a ~280 LOC dead weight
   carrying 32×704 + 128×688 = 110 KB of static BSS reservation that
   never executes. **Fix:** delete both files OR migrate the static
   MENU_* tree in menu.cpp to actually use the registry. Effort: M.

3. **[HIGH]** `c5_node/main/zb_sniffer.c:28-56` — `esp_ieee802154_
   receive_done` is documented to fire in ISR context. The handler
   builds a `posei_msg_t` on stack (sizeof = 240 B, well over recommended
   ISR stack budget), calls `proto_send_to` → `esp_now_send` (NOT
   IRAM-safe in ISR — taken from non-IRAM segment, may cause cache-miss
   crash during flash op), and calls `led_fx_set` (volatile store is
   fine, but the FreeRTOS task it wakes is not). Symptom: random reboots
   during high Zigbee traffic / coordinator beacon storms. **Fix:** queue
   the frame summary into a FreeRTOS queue from the ISR, drain it in a
   normal task that calls `esp_now_send` and `led_fx_set`. Effort: M.

4. **[HIGH]** `sfx.cpp:38-44` + `feature_sfx_settings.cpp` — `sfx_init`
   does `s_prefs.begin("sfx", false)` (RW open) and **never calls
   `.end()`**, so the NVS handle stays open for the lifetime of the
   process; every subsequent `s_prefs.putUChar`/`putBool` uses the same
   handle, bypassing the close-on-write idiom every other namespace in
   this bucket follows (theme/screensaver/ui_ambient/menu). Risk:
   uncommitted writes lost on crash; nvs handle leak on repeat `sfx_init`
   calls. **Fix:** scope `Preferences p; p.begin(...); p.put...; p.end();`
   per call like every other consumer. Effort: S.

5. **[HIGH]** `input.cpp:71-90` — `sfx_click` is called from
   `input_poll_raw` on EVERY printable keypress, control keypress,
   space, tab, etc. `sfx_click` does `note(2800,6) + delay(3) +
   note(2200,5)` = ~14 ms blocking on the input-poll caller. `sfx_select`
   is even worse: `sweep(3800,2400,45)` = 45 ms blocking on ENTER.
   `sfx_back` blocks 39 ms on ESC. Result: ENTER-to-feature latency adds
   45 ms over and above the screen redraw; rapid letter mnemonic
   navigation is throttled to ~70 keys/sec. **Fix:** dispatch SFX into a
   one-deep cooperative scheduler tick (state machine in `loop()`), or
   `xTaskCreate` a tiny "sfx_player" task that consumes a queue of
   tone events. Effort: M.

6. **[HIGH]** `screensaver.cpp` (1,090 LOC) — Single file holds **10
   painter functions** (`run_wardrive_cinema`, `run_matrix_rain`,
   `run_eink_breathing`, `run_deep_scan`, `run_port_scan`,
   `run_hex_cascade`, `run_terminal_crack`, `run_neural_arc`,
   `run_glitch_bsod`, `run_tide_waves`) plus a tiny dispatch table
   (`:1020`). One file per painter (each ~100 LOC) would let
   contributors add new screensavers without re-rebuilding the whole
   unit. Refactor target. Effort: L.

7. **[HIGH]** `menu.cpp` (1,379 LOC) — Static menu tree is hand-coded as
   17 nested `static const menu_node_t MENU_*[]` arrays
   (`:204-1001`). Adding a feature touches `menu.cpp` (forward decl +
   entry insertion + hint string) — three edits per feature, every PR
   collides here. PORKCHOP/Bruce both have the same problem. Since the
   `menu_registry.cpp` infrastructure (defect 2) is already built,
   migrating MENU_* to data-driven registration is the right path.
   Effort: L.

8. **[HIGH]** `ui.cpp` (933 LOC) — Drawing primitives (status / footer /
   toast / text / cleared body) live next to twelve full-screen animations
   (`ui_slide_transition`, `ui_spinner`, `ui_notify_slide`, `ui_ripple`,
   `ui_matrix_rain`, `ui_waves`, `ui_radar`, `ui_hexstream`, `ui_glitch`,
   `ui_eq_bars`, `ui_dashboard_chrome`, `ui_action_overlay`,
   `ui_freq_bars`). Animations should be a separate file (`ui_anim.cpp`)
   so the primitives are not buried. Effort: M.

9. **[MED]** `radio.cpp:108-111` — Calls `WiFi.mode(WIFI_OFF)` during
   teardown. PORKCHOP explicitly bans this pattern
   (`wifi_utils.cpp:227-229` in PORKCHOP, "triggers esp_wifi_deinit/init
   257 errors on fragmented heap"). Every WiFi feature exit funnels here
   via `radio_switch(RADIO_BLE)` etc., so a successful WiFi→BLE switch
   from fragmented heap will eventually deadlock the WiFi driver into
   the `init 257` state and subsequent re-init returns ESP_ERR. **Fix:**
   `esp_wifi_stop()` + leave driver inited (per PORKCHOP playbook), or
   gate the `esp_wifi_deinit` on a "heap is healthy" check (largest
   free > 35 KB threshold à la PORKCHOP `kMinHeapForTls`). Effort: M.

10. **[MED]** `c5_node/main/wifi_attacker.c:38-45,79-80,160-161` — A
    single static `s_frame[26]` is mutated by BOTH `deauth_targeted_task`
    and `deauth_bcast_task`. Currently they are serialised by the
    "one CMD at a time" dispatch in `main.c:on_recv`, but the C5
    accepts back-to-back CMDs and the broadcast task malloc's its arg
    and pushes a 4 KB task, then proceeds to mutate `s_frame` while the
    earlier targeted task may still be looping over the same buffer if
    it hadn't yet hit its `xTaskGetTickCount() < end` check. **Fix:**
    make `s_frame` a stack-local in each task and pass a pointer.
    Effort: S.

11. **[MED]** `c5_node/main/wifi_attacker.c:52-67` — `send_status`
    midway through deauth task hops to ch 1, transmits RESP_STATUS, then
    hops back. During the 2 ms `vTaskDelay`, NO deauth TX is happening.
    At 250 ms cadence that's 0.8% attack downtime, which is fine — but
    every channel hop also reinitialises WiFi PHY state and may drop
    the next 2-3 TX attempts. Symptom: attack rate is non-monotone
    near status emission. **Fix:** queue the status into a normal task,
    have the requester ping for it on a slow heartbeat instead.
    Effort: M.

12. **[MED]** `screensaver.cpp:1085-1089` — `screensaver_check_idle`
    is called from inside menu paint loops (`menu.cpp:1286`,
    `menu_carousel.cpp` similar). On wake, ONLY the menu repaint is
    forced; feature-side idle wake is unhandled (a feature that itself
    blocks on `input_poll` like `feat_about` will exit screensaver
    cleanly, but a feature running its own animation loop without
    polling idle status will have its display state silently clobbered
    by the screensaver painter). **Fix:** add `screensaver_check_idle`
    polls to long-running feature loops, OR move idle dispatch into
    `input_poll` itself so any feature waiting on it benefits. Effort: M.

13. **[MED]** `hat_manager.cpp:27-36` — `detect()` is a stub: comment
    "Future Implementation: Actively probe SPI/I2C buses to detect the
    hat" but the function unconditionally assigns and returns
    `HatType::NONE`. Map line 37 claims active probing for
    CAP-LoRa1262 / HYDRA_RF_424 / ESP32_C5_NODE / FEATHER_NRF52 — that
    claim is false. Side effect: `park_for_lora` / `park_for_cc1101`
    / `park_for_nrf24` are static helpers callable on demand, but no
    code uses them (greps clean for `HatManager::park_for_`). The
    actual hat-aware parking happens ad-hoc in `cc1101_hw.cpp` and
    `nrf24_hw.cpp`. **Fix:** delete the stub or actually implement
    probe sequences (SX1262 v05 read, CC1101 PARTNUM=0x80, etc).
    Effort: M (to implement) or S (to delete).

14. **[MED]** `sd_helper.cpp:115-142` (`sd_format`) — Walks root with
    `openNextFile` and unlinks every file/dir, no real FAT format. Comment
    at `:127-128` acknowledges this. But: `feat_tool_sd_format` in
    `tools.cpp:28-69` runs its own deep walk again — there are TWO
    parallel implementations of "wipe SD by deletion". `sd_format()` is
    NOT called from anywhere outside the SD format menu. Confirmation
    gate (`YES`-typed) is in `tools.cpp`, NOT in `sd_format()` — anyone
    calling `sd_format()` directly bypasses confirmation. **Fix:** make
    `sd_format()` the single source of truth (move the recursive `nuke`
    helper from tools.cpp into sd_helper.cpp), document the confirmation
    contract. Effort: S.

15. **[MED]** `main.cpp:32-38` (`gps_task`) + `:144` — GPS background
    poller is **always-on** from boot, regardless of whether the user
    has enabled GPS. Per memory `feedback_poseidon_gps_off_by_default.md`,
    GPS should be OFF unless the user explicitly toggles it. The poller
    just calls `gps_poll()` which reads UART NMEA — no GPS tag is
    *written* anywhere just from polling — but the UART is active,
    drawing current, and `gps_begin()` (`:143`) brings up the UART pins
    13/15. Pin 13 conflicts with CC1101 CS — `radio_switch(RADIO_SUBGHZ)`
    calls `gps_end()` at `radio.cpp:171` to release it, but on cold boot
    GPS owns pin 13 until the user picks a sub-GHz feature. **Fix:**
    move `gps_begin()` + the `gps_task` spawn behind a "gps_enabled" NVS
    flag, default OFF. Effort: S.

16. **[MED]** `main.cpp:164-179` — Two `#ifdef POSEIDON_AUTO_DEAUTH_TEST`
    blocks calling `poseidon_autotest_show_last_crash` then
    `poseidon_deauth_autotest`. Per `deauth_autotest.cpp` this is
    explicitly a dev-only stress test that runs before splash. It has
    been in main since v0.5; it pollutes boot timing for nothing in the
    no-flag case (zero cost when ifdef false, fair), but the file +
    code is still cruft if the test is never run. **Fix:** consider
    moving the autotest to `test/` directory or behind a runtime flag
    set via serial_test_init's `?` command. Effort: S.

17. **[MED]** `serial_test.cpp:46-49` — `R` command resets the device
    over USB CDC with no authentication. The harness was designed for
    `scripts/test_all_features.py`, but it's also exposed when any USB
    CDC peer (TRIDENT bridge, accidental hot-plug, etc.) writes "R\n"
    to the port. Plus `serial_test_init` always spawns a 4 KB task at
    boot — wasted heap if the user never connects a debug host.
    **Fix:** gate the task behind first contact (lazy-start on first
    incoming byte) and require "RESET\n" (full string) for the reset
    path. Effort: S.

18. **[MED]** `menu.cpp:1090-1113` — Root-menu C5 status panel reads
    `c5_any_online()` + `c5_peer_count()` on every paint frame (~33 ms
    cadence per `:1296`). Each call walks `c5_cmd.cpp`'s peer table and
    iterates `c5_last_seen_ms`. Not catastrophic but visible in heap-
    pressure traces. Same data is re-read by `ui_draw_status` in
    `ui.cpp:152` for the small badge. **Fix:** cache `c5_n` once per
    poll tick and reuse. Effort: S.

19. **[MED]** `ui_ambient.cpp:31-42` + `theme.cpp:154-165` + `screensaver.
    cpp:54-66` + `menu.cpp:23-36` — Four different "first-touch lazy
    NVS read" patterns for the `pui` / `pamb` / `pscr` / `sfx`
    namespaces, each with its own `s_loaded` flag and its own
    `Preferences p; p.begin(); p.getX(); p.end();` boilerplate. There's
    no shared helper. Compare to PORKCHOP's `core/settings_manager`.
    **Fix:** consolidate into `prefs_helper.{cpp,h}` with `prefs_read_*` /
    `prefs_write_*` and per-namespace caches. Effort: M.

20. **[LOW]** `radio.cpp:97-133` (`teardown_current`) — `delay(100)`
    after every teardown is unconditional. Most paths don't need it;
    it adds a perceptible lag every time the user opens a feature.
    **Fix:** gate the delay on the actual radio domain (e.g.
    NimBLEDevice::deinit needs the wait, WiFi STA stop does not).
    Effort: S.

21. **[LOW]** `ui.cpp:64-78` (`ui_clear_body`) — Strobe-suppression
    heuristic: if cleared >4 times in 80 ms, skip. This is brittle —
    a fast feature that legitimately needs to clear 5 times in a row
    (e.g. a swap-out animation) gets silently dropped. Better:
    expose `ui_clear_body_force` (`:84` already exists) AND audit
    every caller to use force when appropriate. Effort: S.

22. **[LOW]** `splash.cpp:55-148` — Map description "metallic trident
    sprite, title bloom, scanline sweep" is wrong; the actual splash is
    a Hokusai Great Wave fade (`splash_data` from `splash_sprite.h`,
    public-domain Wikimedia). Update map. Effort: docs-only.

23. **[LOW]** `argus.cpp:134-145` — RAM sprite allocation is gated on
    `s_ram_sprite_tried` flag (good defence against alloc spam in low-
    heap), but on alloc fail it permanently falls back to flash
    `pushImage`. The whole reason for the RAM cache is the TX-cache
    invariant (`feedback_poseidon_tx_layout_sensitivity.md`). If the
    cache alloc fails, Argus + WiFi TX will scramble the sprite. There
    is no retry mechanism on future calls. **Fix:** allow a retry after
    `radio_switch(RADIO_BLE)` (i.e. when the feature opens that doesn't
    do WiFi TX) so the cache can come back. Effort: M.

24. **[LOW]** `menu_carousel.cpp:391-415` — Carousel letter-mnemonic
    feature execution path is missing the `pinMode(44, OUTPUT);
    digitalWrite(44, LOW)` defensive IR park that the ENTER path has at
    `:376-377`. Inconsistent. Whatever the right polarity should be (see
    defect 1), both paths need it. Effort: S.

25. **[LOW]** `c5_node/main/main.c:222-298` (`app_main`) — No error
    handling on the per-band protocol set call (`:282`). If it returns
    `WIFI_ERR_NOT_SUPPORTED` (which it does on early-revision C5 with
    older IDF), the 5 GHz scan path silently degrades. Currently logged
    but boot continues. **Fix:** ESP_ERROR_CHECK or at least visibly
    mark `s_has_5g = false` so HELLO advertises capability honestly.
    Effort: S.

26. **[LOW]** `c5_node/main/main.c:198-208` — v3 CMDs 18-26 are
    explicitly stubbed ("not implemented yet, dropping seq=%u"). S3-side
    `c5_cmd.cpp` exposes senders for all of them via `c5_cmd.h`. UI
    paths that call e.g. `c5_cmd_beacon_spam` will sit waiting for
    RESP frames that never arrive. **Fix:** either implement on C5, or
    gate the S3-side feature menu entries behind "C5 firmware ≥ X.Y".
    Effort: docs/menu (S) up to feature impl (L).

27. **[LOW]** `version.h:19-20` — `inline const char *poseidon_version()`
    declared `inline` (not `static inline`) in a header. C++ ODR-linkage
    permits this but it relies on linker dedup; under `-flto` certain
    toolchains warn. Bruce uses the `static inline` pattern. **Fix:** add
    `static`. Effort: trivial.

## Cross-ref deltas

### vs Bruce

| Feature | POSEIDON | Bruce | Verdict | Notes |
|---|---|---|---|---|
| Menu system | 1379 LOC static tree + unused registry | 561 LOC `main.cpp` boots `MainMenu::begin` | Bruce wins | Bruce dispatches feature classes; POSEIDON should activate its already-written registry |
| Theme | 191 LOC, 6 palettes, NVS, live-preview | `core/theme.cpp` `BruceTheme` struct | Parity | POSEIDON's six themes > Bruce's BruceTheme |
| UI primitives | 933 LOC monolith | split across `core/display.cpp` + `ui/*.cpp` | Bruce wins on org | POSEIDON should split out anim |
| Screensaver | 10 painters, NVS shuffle, ~1090 LOC | not present | POSEIDON wins | Distinctive feature |
| Hat manager | stub, returns NONE always | not present | n/a | Bruce uses per-board pin tables; POSEIDON's runtime probe would be better if implemented |
| SD helper | dedicated HSPI bus, 3-speed fallback | `bruceConfig.sd_bus` shared with CC1101/NRF | POSEIDON wins | Less coupling |
| Splash | Hokusai wave fade-in (148 LOC) | `splash.h/.cpp` simple | POSEIDON wins on craft | |
| Boot sequence | IR park first, board detect, single SD mount | `setup()` runs Bruce config + display init | Mostly parity | POSEIDON's IR active-LOW handling is broken (defect 1) |
| Crash viewer | autotest NVS marker only | `ui/crash_viewer.cpp` SD-backed | Bruce wins | Adopt as future work |

### vs Ghost ESP

| Feature | POSEIDON | Ghost | Note |
|---|---|---|---|
| UI stack | M5GFX direct draws, custom widgets | **LVGL** + display_manager | Ghost has real widget library; POSEIDON paints by hand |
| Web control | none | REST API on port 80 + 3988-line embedded SPA | Big gap — full HTTP API to drive features from a browser |
| Settings | per-namespace `Preferences` everywhere | `settings_manager.c` single source | Ghost has shared NVS helpers |
| Command line | not present | 33-command CLI in `commandline.c` + serial dispatcher | POSEIDON has only `serial_test`'s 4-cmd harness |
| PCAP writer | not present in this bucket | `vendor/pcap.c` for both BT + WiFi | Move PCAP writer into helper unit |
| RGB FX | C5-only `led_fx.c` | `rgb_manager.c` + on-board APA106 with full mode table | Ghost's pattern would help on C5 side |

Porting opportunities: (a) Ghost's `settings_manager` pattern collapses
defect 19; (b) Ghost's `commandline.c` could replace `serial_test.cpp`
with a real shell; (c) Ghost's LVGL is overkill but `display_manager.c`'s
screen-stack pattern (push/pop screens explicitly) is cleaner than the
recursive `run_submenu`.

### vs PORKCHOP (PRIMARY)

PORKCHOP runs on the same M5Cardputer hardware with the same broken-PSRAM
constraint. Every system pattern below is a direct port candidate.

| PORKCHOP pattern | POSEIDON state | Verdict |
|---|---|---|
| `heap_policy.h` 47 named thresholds (`kMinHeapForTls=35000`, `kPressureLevel3Free=30000`, `kKnuthRatioWarning=0.70`) | Not present — no heap policy module at all | **ADOPT** |
| 4-level pressure state machine (Normal/Caution/Warning/Critical) with feature-shedding | Not present | **ADOPT**, gate `argus_draw` and `ui_ambient_tick` on it |
| Reservation Fence (80 KB malloc-then-free at boot to force WiFi driver allocs high) | Not present — main.cpp goes straight to `M5Cardputer.begin()` | **ADOPT** — could solve cramped 14 KB DMA budget in `wifi_lean_sta_init` |
| EMA-smoothed display percent (`kDisplayEmaAlphaDown=0.10` / `Up=0.20`) | Heap percent in status bar is quantised to 4 KB bucket (`ui.cpp:151`) — flicker fix, not pressure tracking | **ADOPT** for status |
| Heap watermark persistence (SD min-free / min-largest across reboots) | Not present | **ADOPT** under a `/poseidon/heapwatermark.log` path |
| Knuth 50% rule fragmentation telemetry | Not present | Could go in diagnostics |
| `canGrow(minFree, minFrag)` vector-growth gate | Not present — features `push_back` blindly | **ADOPT** for `g_wdr_aps`, `triton` deauth list, etc. |
| Ban on `WiFi.mode(WIFI_OFF)` during teardown | **POSEIDON VIOLATES** at `radio.cpp:109` | See defect 9 |
| Single cooperative `update()` loop, no per-mode `xTaskCreate` | POSEIDON spawns ~12 xTaskCreates (`main.cpp:144,148`, splash, deauth, wardrive, triton, …) | Long-term refactor — POSEIDON's free-form task model is the root cause of several BLE-coop violations (per memory `feedback_poseidon_ble_cooperative.md`) |
| Strict pre-WiFi-init teardown (stop scan, `NimBLEDevice::deinit(true)`, delays, yield) | `radio.cpp:117` does deinit but with no yields/delays | **ADOPT** the full recipe |
| WiFi+BLE init order coex (NEVER WiFi.mode(OFF) before NimBLE init) | Followed-by-accident; not enforced | **ADOPT** as explicit policy |
| Crash viewer reading SD coredump | autotest marker only | **ADOPT** — port `crash_viewer.cpp` |
| First-class diagnostics menu (heap, PSRAM, Knuth ratio, WiFi state) | Per-feature ad-hoc | **ADOPT** as a System submenu |

This is the longest "should adopt" list across the four reference
codebases. PORKCHOP is the **canonical reference** for system-level
robustness on this hardware; POSEIDON's system bucket is the place to
import these patterns.

### vs Evil-M5

| Aspect | POSEIDON | Evil-M5 |
|---|---|---|
| Menu architecture | hierarchical nested `menu_node_t` | flat `menuItems[]` :233, dispatched by `switch(case)` :2622 |
| File layout | 18 .cpp + features/ subdir | single 38 k LOC `.ino` |
| Theme | enum + struct table, NVS | INI from SD (`/evil/theme.ini`) |
| Boot-to-feature | not present | `caseToStartAtBoot` setting (Evil-M5 wins on auto-launch) |
| Web admin panel | not present | inline HTML on port 80 with 8 tabs |
| SD config | per-namespace Preferences (NVS) | `/evil/config/config.txt` INI |

POSEIDON's submenu-driven architecture is structurally better than
Evil-M5's flat list, but Evil-M5's auto-launch ("boot directly into
feature N") is a feature worth porting for kiosk / drop-box deployments.

## Optimisation opportunities

- Boot path (`main.cpp:73-182`): 4× `digitalWrite(44, LOW)`; once polarity bug fixed, 3 are redundant — keep first + watchdog.
- Status bar repaint (`ui.cpp:145-220`): heap-bucket compute + C5 peer walk every paint call. Cache per 1 s.
- C5 HELLO cadence (`main.c:69-85`): 5 s broadcast unconditional. Adopt PigSync-style backoff to 30 s after pairing.
- Ambient layer (`ui_ambient.cpp:62-129`): 8 motes + grid + packet at 30 fps. Cap to 15 fps under heap pressure.
- Argus RAM sprite (4,608 B SRAM): static reservation; lazy-evict after N seconds unchanged mood.
- C5 `s_seen` (`pmkid_capture.c:54-69`) + `s_m1` (`hs_capture.c:60`) + `s_done` (`hs_capture.c:66`): heap-alloc per session, free on stop.

## Refactor targets

1. Split `screensaver.cpp` → `screensaver/` subdir, one .cpp per painter + `screensaver_pool.cpp` registry/NVS/dispatcher.
2. Split `menu.cpp` → `menu_tree.cpp` (static arrays) + `menu_terminal.cpp` (renderer/runloop) + `menu.cpp` (types/dispatch). Or activate registry (target 6).
3. Split `ui.cpp` → `ui.cpp` primitives (~400 LOC) + `ui_anim.cpp` (~500 LOC of animations).
4. `prefs_helper.{cpp,h}` — consolidate 5 NVS lazy-load patterns (theme/screensaver/sfx/ui_ambient/menu_style) + close-on-write safety.
5. Adopt PORKCHOP `heap_policy.h` + heap-pressure state machine; gate Argus + ambient + ble-spam on level.
6. Activate `menu_registry`: every feature uses `REGISTER_FEATURE(...)` at file scope, retire static MENU_* tree.

## Backlog items

- **sys-001** [CRIT] `main.cpp:63-67,80-81,138-139` + `menu.cpp:1323,1357` + `menu_carousel.cpp:377` — IR LED parked LOW (= ON). Fix: flip off-state writes to HIGH. Effort: S.
- **sys-002** [CRIT] `menu_registry.cpp/.h` — Dead code, ~280 LOC + 110 KB static BSS reservation never executes. Fix: delete OR activate via main.cpp + migrate static MENU_* to REGISTER_FEATURE. Effort: M / L.
- **sys-003** [HIGH] `c5_node/main/zb_sniffer.c:28-56` — ESP-NOW send + led_fx_set from ISR. Fix: queue → normal task. Effort: M.
- **sys-004** [HIGH] `sfx.cpp:38-44` — NVS handle leaked process-lifetime. Fix: scope per call. Effort: S.
- **sys-005** [HIGH] `input.cpp:71-90` — Blocking `sfx_*` from input_poll, 45 ms ENTER latency. Fix: non-blocking SFX player task + tone queue. Effort: M.
- **sys-006** [HIGH] `screensaver.cpp` (1090 LOC) — Split 10 painters into `src/screensaver/*.cpp`. Effort: L.
- **sys-007** [HIGH] `menu.cpp` (1379 LOC) — Split static tree OR migrate to `menu_registry`. Effort: L.
- **sys-008** [HIGH] `ui.cpp` (933 LOC) — Move ~500 LOC of animations to `ui_anim.cpp`. Effort: M.
- **sys-009** [MED] `radio.cpp:109` — `WiFi.mode(WIFI_OFF)` violates PORKCHOP. Fix: stop, don't deinit unless heap healthy. Effort: M.
- **sys-010** [MED] `c5_node/main/wifi_attacker.c:38,79-80,160-161` — Static `s_frame[26]` shared by two tasks. Fix: stack-local. Effort: S.
- **sys-011** [MED] `c5_node/main/wifi_attacker.c:52-67` — Channel hop mid-attack for RESP_STATUS. Fix: separate sender task. Effort: M.
- **sys-012** [MED] `screensaver.cpp:1085` — Idle wake races; features without idle poll get clobbered. Fix: move idle-detect into `input_poll`. Effort: M.
- **sys-013** [MED] `hat_manager.cpp:27-36` — `detect()` is a stub returning NONE. Fix: implement OR delete the class. Effort: S/M.
- **sys-014** [MED] `sd_helper.cpp:115-142` vs `tools.cpp:28-69` — Two SD-wipe implementations. Fix: consolidate. Effort: S.
- **sys-015** [MED] `main.cpp:32-38,143-144` — GPS poller always-on at boot. Fix: NVS-gated `gps_enabled` flag, default OFF. Effort: S.
- **sys-016** [MED] `main.cpp:164-179` + `deauth_autotest.cpp` — Autotest cruft in production main. Fix: move to `test/` or behind serial cmd. Effort: S.
- **sys-017** [MED] `serial_test.cpp:46-49,68-71` — Unauth `R\n` reset + always-on 4 KB task. Fix: lazy-start + full `RESET` string. Effort: S.
- **sys-018** [MED] `menu.cpp:1090-1113` + `ui.cpp:152` — Double C5 peer-walk per repaint. Fix: cache. Effort: S.
- **sys-019** [MED] Five parallel NVS-lazy-read patterns. Fix: shared `prefs_helper`. Effort: M.
- **sys-020** [LOW] `radio.cpp:132` — Unconditional `delay(100)` after teardown. Fix: per-domain gating. Effort: S.
- **sys-021** [LOW] `ui.cpp:64-78` — Strobe-suppression drops legit clears. Fix: caller opt-in. Effort: S.
- **sys-022** [LOW] `splash.cpp` — Map says trident sprite, code is Hokusai wave. Fix: update map. Effort: docs.
- **sys-023** [LOW] `argus.cpp:134-145` — No retry after RAM alloc fail. Fix: retry on `radio_switch(RADIO_BLE)`. Effort: M.
- **sys-024** [LOW] `menu_carousel.cpp:391-415` — Letter mnemonic path missing defensive IR park (cf. ENTER at `:376-377`). Effort: S.
- **sys-025** [LOW] `c5_node/main/main.c:282-287` — No error gating on per-band protocol set. Fix: mark `has_5g=false` on err. Effort: S.
- **sys-026** [LOW] `c5_node/main/main.c:198-208` — v3 CMDs 18-26 stubbed; S3-side senders exist. Fix: gate UI entries OR implement. Effort: S/L.
- **sys-027** [LOW] `version.h:19-20` — `inline` without `static` in header. Fix: prepend `static`. Effort: trivial.
- **sys-028** [HIGH] Adopt PORKCHOP `heap_policy.h` + heap-pressure state machine; gate Argus/ambient. Effort: L.
- **sys-029** [MED] Adopt PORKCHOP Reservation Fence (80 KB malloc-then-free before WiFi init). Effort: M.
- **sys-030** [MED] Adopt PORKCHOP heap watermark persistence to SD. Effort: M.
- **sys-031** [MED] Add diagnostics menu (heap/frag/WiFi/C5/hat) under System submenu. Effort: M.
- **sys-032** [MED] Add Evil-M5 boot-to-feature: NVS `boot_feature` + countdown. Useful for kiosk. Effort: M.
- **sys-033** [LOW] Ghost-style REST control surface (System → Remote). Future. Effort: L.
