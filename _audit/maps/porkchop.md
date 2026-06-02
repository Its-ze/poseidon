# M5PORKCHOP Reference Map

Commit: 800f06b
Cloned: 2026-06-01
Repo: https://github.com/0ct0sec/M5PORKCHOP
Target: M5Stack Cardputer / Cardputer-Adv (same family as POSEIDON; ESP32-S3 StampS3, 8MB flash, NO PSRAM — `-DBOARD_HAS_PSRAM=0`)

## Feature index

| Feature | File:line | Notes |
|---------|-----------|-------|
| WiFi scan | `src/core/network_recon.cpp:759` (start) | Shared promiscuous service used by OINK/DNH/SPECTRUM; channel hop in same file ~L930 |
| WiFi deauth | `src/modes/oink.cpp` (sendDeauthFrame / sendDeauthBurst, declared `oink.h:219`) | Uses `esp_wifi_80211_tx(WIFI_IF_STA,…)` via `core/wsl_bypasser.cpp:85` |
| Beacon spam | `src/modes/bacon.cpp:496` (`esp_wifi_80211_tx`) | "BACON" beacon broadcaster — NOT random spam: emits 1–3 AP fingerprint Vendor IE (OUI 0x50:52:4B = "PRK") for hide-and-seek game with Sirloin; CH:6 only |
| Probe / hidden-SSID reveal | `src/modes/spectrum.cpp` (broadcast deauth reveal), `oink.h:222` (`sendAssociationRequest`) | SPECTRUM has explicit "reveal mode" sending broadcast deauths to flush PROBE-RSP carrying hidden SSID |
| Evil twin / portal | — | NOT PRESENT |
| Wardrive | `src/modes/warhog.h`, `warhog.cpp:459` (xTaskCreatePinnedToCore for scan) | GPS-gated, WiGLE CSV output; bounty system for Sirloin satellite |
| PMKID | `src/modes/oink.h:88` (struct), processed in `oink.cpp` processEAPOL; saved via `saveAllPMKIDs` | DNH mode also captures PMKIDs (`donoham.h:96`) |
| Handshake (M1-M4) | `src/modes/oink.h:55` (struct), `processEAPOL` in `oink.cpp` | Hashcat 22000 + PCAP export; tracks M1+M2 and M2+M3 pairs |
| Raw frame TX | `src/core/wsl_bypasser.cpp` | Deauth + Disassoc, `-Wl,-zmuldefs` linker flag in `platformio.ini:34` to override `ieee80211_raw_frame_sanity_check` |
| Passive recon (DO NO HAM) | `src/modes/donoham.h`, `donoham.cpp` | Adaptive-state-machine channel hop (HOPPING/DWELLING/HUNTING/IDLE_SWEEP) |
| BLE scan | `src/modes/piggyblues.h:94` (NimBLEScanCallbacks) | Continuous async scan, vendor classification |
| BLE spam (Apple/Android/Samsung/Windows) | `src/modes/piggyblues.cpp` (sendAppleJuice, sendAndroidFastPair, sendSamsungSpam, sendWindowsSwiftPair) | Non-connectable advertising, 20–40ms interval |
| BLE finder/findmy/karma/whisperpair/clone/GATT/HID | — | NOT PRESENT |
| Sub-GHz (CC1101) | — | NOT PRESENT — no CC1101 driver in tree |
| nRF24 (mousejack/sniff) | — | NOT PRESENT |
| IR (tvbgone/clone/remote) | — | NOT PRESENT |
| Mesh / LoRa | — | SX1262 only **quiesced** for SD bus coexistence (`config.cpp:382` `prepareCapLoraGpio`); no LoRa stack |
| Net attacks (DHCP/Responder/etc.) | — | NOT PRESENT |
| WiFi File Server | `src/web/fileserver.h`, `fileserver.cpp` | STA mode, downloads from SD over HTTP |
| WiGLE upload | `src/web/wigle.cpp`, `src/ui/wigle_menu.cpp` | HTTPS POST of wardrive CSVs |
| wpasec upload | `src/web/wpasec.cpp` | HTTPS POST of handshakes/PMKID to wpa-sec.stanev.org |
| GPS | `src/gps/gps.cpp` | TinyGPSPlus; supports onboard + Cap LoRa GPS source |
| Menu / UI / theme | `src/ui/menu.cpp`, `display.cpp` | Three-canvas system: topBar / mainCanvas / bottomBar (8-bit color) |
| Input | `M5Cardputer.update()` in `main.cpp:194` | Keyboard via M5Cardputer lib |
| Crash viewer | `src/ui/crash_viewer.h/cpp` | Reads SD crash files; cap at 32 (`crash_viewer.cpp:49`) |
| Diagnostics | `src/ui/diagnostics_menu.cpp` | Heap, PSRAM, WiFi state, Knuth ratio |
| Achievements / XP / Unlockables | `src/core/xp.cpp/h`, `ui/achievements_menu.cpp`, `ui/unlockables_menu.cpp` | Gamification layer (POSEIDON does not have this) |
| Avatar / Mood / Weather (Piglet) | `src/piglet/` | Tamagotchi-style personality |
| ESP-NOW sync (PigSync ↔ Sirloin) | `src/modes/pigsync_client.cpp`, `pigsync_protocol.h` | PMK + LMK encrypted, channel-1 discovery + data-channel switch |
| Stress test | `src/core/stress_test.cpp` | Heap-budgeted network injection (no RF) |
| SD format | `src/core/sd_format.cpp`, `ui/sd_format_menu.cpp` | Uses `esp_task_wdt_reset()` during long erase |
| BadUSB | — | NOT PRESENT |
| Special: BACON game | `src/modes/bacon.h:19` | Vendor IE protocol for two-device hide-and-seek |
| Special: BOAR BROS exclusion list | `src/modes/oink.h:172` | Fixed 50-entry array (zero heap), persists to SD |
| Special: PigSync bounty system | `src/modes/warhog.h:21,49` | POPS sends Sirloin a list of 15 bountied BSSIDs to focus on |

## Freeze-resilience patterns

- **Watchdog handling**: `esp_task_wdt_reset()` called only in long-running SD ops — `core/sd_format.cpp:306,357,366` and `ui/sd_format_menu.cpp:239,246`. Mode loops rely on `loop()`-driven cooperative ticks, not WDT feeds.
- **Long-running op pattern**: NO `xTaskCreate` for attack tasks. Only `WarhogMode::scanTask` uses `xTaskCreatePinnedToCore` (`warhog.cpp:459`) — a finite-duration scan, deleted on completion (`vTaskDelete`, L284/421). Everything else is cooperative `update()` ticks called from `main.cpp:254 porkchop.update()`. Explicit `yield()` after `NimBLEDevice::deinit(true)` (`network_recon.cpp:811`) "to let deferred FreeRTOS cleanup tasks coalesce freed BLE memory."
- **Heap monitoring**: First-class subsystem. `core/heap_health.h` (HeapPressureLevel Normal/Caution/Warning/Critical), `core/heap_gates.h` (`checkTlsGates`, `canTls`, `shouldProactivelyCondition`, `canGrow` with frag-ratio threshold), `core/heap_policy.h` (47 named thresholds incl. `kMinHeapForTls=35000`, `kPressureLevel3Free=30000`, `kKnuthRatioWarning=0.70`). EMA-smoothed display percent with asymmetric alpha (`kDisplayEmaAlphaDown=0.10`, `kDisplayEmaAlphaUp=0.20`) absorbs transient spikes — analogous to but more elaborate than anything POSEIDON has.
- **Crash handler / panic save**: Crash *viewer* exists (`ui/crash_viewer.cpp`) reading files from SD, but I found no `esp_register_shutdown_handler` writer — coredump partition is in `partitions.csv:8` but `sdkconfig.defaults:2` sets `CONFIG_ESP_COREDUMP_ENABLE=n`. Crash logs are presumably written by mode-side error paths to SD, not by the panic handler.
- **Watermark persistence**: `HeapHealth::persistWatermarks()` called every loop, rate-limited 60s, persists min-free / min-largest across reboots (`heap_health.h:50,51`).
- **Background heap stabilization tracking**: `NetworkRecon::isHeapStable()` (`network_recon.cpp:946`) marks `heapStabilized=true` once largest block > `kHeapStableThreshold=50000`.

## Hardware bring-up patterns

- **CC1101 / nRF24 / IR**: not present in firmware.
- **SD SPI**: dedicated `SPIClass sdSPI(FSPI)` (`config.cpp:28`); explicit pins CS=12 MOSI=14 CLK=40 MISO=39; `sdSPI.begin(SCK,MISO,MOSI,CS)` at `config.cpp:271`; SX1262 CS pre-deasserted on G5 at `main.cpp:99-100` **before** `M5Cardputer.begin()` because that begin reconfigures G5 as keyboard matrix pull-up.
- **CapLoRa GPS (SX1262 quiesce)**: `Config::prepareCapLoraGpio()` (`config.cpp:382`) — `gpio_reset_pin(G13)` clears default FSPIQ IOMUX, then SX1262 reset pulse + CS HIGH so it tri-states MISO. Followed by `Config::reinitSD()` post-GPS-UART init (`main.cpp:161`). Direct precedent for POSEIDON's shared-HSPI gotcha.
- **WiFi+BLE coexistence**: Strict pattern. Before each WiFi reactivation: stop scan/advertising, `NimBLEDevice::deinit(true)`, `delay(100)`, `yield()`, `delay(50)`, then `WiFi.mode(WIFI_STA)` — see `network_recon.cpp:794-822` and `wifi_utils.cpp:303,444`. Inverse (WiFi off before BLE): `piggyblues.cpp:594` `WiFi.mode(WIFI_OFF); delay(BLE_OP_DELAY_MS); NimBLEDevice::init("")`.
- **WiFi pre-init "Reservation Fence"** (`main.cpp:59-87`): 80KB heap_caps_malloc fence allocated **before** `WiFi.mode(WIFI_STA)` to force WiFi driver's ~35KB permanent buffers to land **above** the fence. Fence then freed, leaving large contiguous free space below. Replaces an earlier 5-phase alloc/free conditioning dance. Direct heuristic POSEIDON could adopt.
- **Never `WiFi.mode(WIFI_OFF)` during teardown**: explicitly warned at `wifi_utils.cpp:227-229` — triggers `esp_wifi_deinit/init 257` errors on fragmented heap. Many sites comment "Keep driver alive to avoid esp_wifi_init 257" (`captures_menu.cpp:1029,1039`, `wigle_menu.cpp:640,650`, `diagnostics_menu.cpp:174`).
- **Display + TX**: 8-bit color sprites, three-canvas split (topBar / mainCanvas / bottomBar) all created once at `display.cpp:219-226`. No explicit "cache to SRAM before TX" pattern found — but they avoid the POSEIDON pattern by NOT calling `pushSprite` during raw `esp_wifi_80211_tx` bursts; deauth bursts happen inside `OinkMode::update()` and the next frame's `Display::update()` is the only `pushSprite` site (`display.cpp:476-478`). Single-threaded interleave sidesteps the cache-stall scramble.

## Memory budget patterns

- **Static BSS allocations**: `OinkMode::boarBros[50]` (`oink.h:241` — fixed array, comment "zero heap allocation"), `OinkMode::filteredCache[64]`, `DoNoHamMode::channelStats[13]` (`donoham.h:100`), `SpectrumMode::renderNets[MAX_SPECTRUM_NETWORKS=64]` (`spectrum.h:113`), `Porkchop::MAX_EVENT_QUEUE_SIZE=32` cap (`porkchop.h:90`). Render snapshots (`SpectrumRenderNet`, `SpectrumRenderSelected`, `SpectrumRenderMonitor`) are heap-safe copies so the WiFi callback never holds a vector pointer.
- **PSRAM**: **DISABLED** at compile time (`platformio.ini:22 -DBOARD_HAS_PSRAM=0`). NimBLE allocator explicitly set internal-only (`platformio.ini:27-28`). Same hardware constraint POSEIDON's broken-PSRAM unit faces — PORKCHOP confirms a viable feature set under that constraint. Diagnostics still query `ESP.getPsramSize()` for display only (`diagnostics_menu.cpp:149`).
- **Heap watermarks logged**: Every 5s in `main.cpp:199-205` (free / largest / minFree). HeapHealth persists watermarks to SD (`heap_health.h:50`).
- **Knuth's 50% rule**: live free-blocks / allocated-blocks ratio computed (`heap_health.h:38`), threshold `kKnuthRatioWarning=0.70` (`heap_policy.h:82`) — flags pathological fragmentation. Only enabled when diagnostics menu is active to avoid the cost of heap enumeration.
- **Vector growth gating**: `HeapGates::canGrow(minFreeHeap, minFragRatio=0.40)` (`heap_gates.h:51`) — fragmentation-aware push_back guard. `NetworkRecon` explicitly does NOT `shrink_to_fit` mid-run (`network_recon.cpp:940-943`) to avoid TLSF re-fragmenting.
- **TLS reserve**: `tlsReserve = heap_caps_malloc(...)` (`wifi_utils.cpp:84,103`) — pre-allocates contiguous block kept around so TLS handshake doesn't fail on fragmented heap. Currently disabled in `main.cpp:125` ("browser handles TLS"), but the mechanism is in tree.

## Build / lib pinning

- **platform**: `espressif32@6.12.0`, framework arduino, board `m5stack-stamps3`, `f_cpu=240MHz`, flash QIO 8MB.
- **Custom version tag**: `custom_version = 0.1.8b-PSTH`.
- **Lib pins**: `m5stack/M5Unified@^0.2.11`, `m5stack/M5Cardputer@^1.1.1`, `bblanchon/ArduinoJson@^7.4.2`, `mikalhart/TinyGPSPlus@^1.0.3`, `h2zero/NimBLE-Arduino@^2.3.7`.
- **Key build flags**: `-Os`, `-Wl,-zmuldefs` (override raw-frame sanity check), `-DBOARD_HAS_PSRAM=0`, NimBLE internal mem only, `-DPORKCHOP_LOG_ENABLED=0` for release (separate `[env:m5cardputer-debug]` for verbose).
- **sdkconfig.defaults**: coredump OFF, mbedTLS dynamic buffers with 2KB content len, WiFi RX `STATIC=4 DYNAMIC=8`, TX `DYNAMIC=16 type=1`.
- **Partition layout** (`partitions.csv`): nvs 20KB, otadata 8KB, app0/app1 3MB each (dual OTA), spiffs 1.5MB, **coredump 64KB** (even though disabled in sdkconfig — slot reserved).

## Anything PORKCHOP does that POSEIDON does NOT appear to do (high-level)

- Reservation-fence heap layout pattern (80KB malloc-then-free to push WiFi driver allocations high).
- Tiered heap-pressure state machine (Normal/Caution/Warning/Critical) with feature-shedding gates and EMA-smoothed display percent.
- Knuth 50% rule fragmentation telemetry (free/alloc block ratio).
- Heap-watermark persistence across reboots (SD-backed min-free / min-largest history).
- `canGrow(minFree, minFrag)` fragmentation-aware vector growth gate before every push_back.
- Explicit ban on `WiFi.mode(WIFI_OFF)` mid-run; documented pattern of keeping WiFi driver hot to avoid `esp_wifi_init 257`.
- BACON Vendor-IE hide-and-seek game with paired companion device (Sirloin).
- PigSync ESP-NOW encrypted bidirectional capture sync with bounty list + beacon grunts + RTC/battery telemetry.
- Adaptive 4-state DNH channel hopper (HOPPING/DWELLING/HUNTING/IDLE_SWEEP) with per-channel dead-streak and hunt cooldowns.
- XP / achievements / unlockables gamification layer threaded through every mode.
- Per-mode `injectTestNetwork` plus `core/stress_test.cpp` for no-RF heap stress.
- BOAR BROS network exclusion list (fixed 50-entry, persisted) and explicit `filteredCount`.
- Single cooperative `update()` loop architecture for all attack modes — no per-mode xTaskCreate, sidestepping POSEIDON's rc=-1 silent-fail and most of the TX-during-display contention surface.
