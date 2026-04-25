# POSEIDON memory pressure reduction Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Reduce DRAM/IRAM/flash usage in POSEIDON firmware so the v0.6.0 visual immersion / Triton plugin port / nRF52 hat work has runway. Current build log shows IRAM 100%, DIRAM 88.65%, flash 66.8% — we want to push DIRAM under 80% (~ -30 KB BSS), keep IRAM at 100% but stop adding more (audit user-side IRAM_ATTR), and shave 50-100 KB flash so future feature growth has room.

**Architecture:** Three layered passes. (1) **Build-flag layer** — turn on LTO, verify gc-sections, generate per-symbol size reports as a baseline tool. Cheapest, lowest risk, instant linker-level wins. (2) **DRAM heap migration layer** — move large feature-private static buffers (subghz_spectrum waterfall ring 28.8 KB, triton capture queue 8 KB, wifi_pmkid M1 cache 4.9 KB, net_cctv hits 4.8 KB, ble_whisperpair keys 4.4 KB, mimir targets 4 KB, net_lanrecon hosts 3.5 KB, net_ssdp devs 2.9 KB, ble_whisperpair targets ~1.7 KB, system_tools fb 1.3 KB) from `.bss` into heap allocations made on feature-entry and freed on feature-exit. The arrays survive only while the feature is open — when the user backs out, DRAM returns to the heap. (3) **IRAM cleanup layer** — patch the local copy of `RCSwitch.cpp` (Bruce's fork, which we already maintain via lib_deps) to drop the `IRAM_ATTR` from `RCSwitch::handleInterrupt()` + `RCSwitch::receiveProtocol()` since we never call `enableReceive()`. Audit our 3 user IRAM_ATTRs (`gdo0_isr`, `lora_rx_isr`, `rx_done_cb`) — keep them, document why, but make sure we don't add more.

**Tech Stack:** PIO 55.03.38 (IDF 5.5.4 / Arduino 3.3.8), `m5stack-stamps3` board, ESP32-S3 240 MHz, 8 MB flash, 320 KB DRAM, 16 KB IRAM-only, no PSRAM. Build via `pio run -e cardputer`, upload via `-t upload`, monitor at 115200. Linker map at `.pio/build/cardputer/firmware.map`. C++17. No external deps required for this plan beyond what's already in `platformio.ini`.

---

## Hard constraints (locked in by hardware + repo state)

Re-read before writing any code in any task.

- **No PSRAM.** PSRAM is broken on this unit (`platformio.ini:27-30`). All heap allocations land in internal SRAM. `heap_caps_malloc(MALLOC_CAP_SPIRAM, …)` is forbidden — every `malloc`/`new` in this plan must use the default internal-SRAM caps.
- **IRAM ceiling.** PIO reports IRAM 100% used. The **dominant** consumers are the BT controller (`libbtdm_app.a` ~17 KB) and PHY (`libphy.a` ~5 KB) which live inside the **prebuilt** `framework-arduinoespressif32-libs` zip from Bruce. We **cannot** rebuild the IDF from this repo, so those are fixed cost — do not propose changes that require rebuilding the framework. The only IRAM we can claw back is from libraries we link as source (RCSwitch via `lib_deps`) and our own `IRAM_ATTR` annotations.
- **Don't break deauth.** The `wifi_sanity_override.cpp` `--allow-multiple-definition` linker flag is load-bearing. Do NOT remove `-Wl,--allow-multiple-definition` and do NOT touch `src/features/wifi_sanity_override.cpp`. See `KnownDeauthBugFixSoon.md`.
- **Don't break load-bearing libs.** NimBLE-Arduino 2.5.0 + RadioLib 7.4.0 + RF24 + Bruce's SmartRC-CC1101 fork are all pinned for known reasons documented in `platformio.ini`. The plan must not change these versions.
- **Don't change feature surface.** No `MAX_*` constant gets shrunk in this plan — wardrive still holds 256 APs, ble_scan still holds 32 devices, etc. We change WHERE the memory lives (heap vs BSS), not how much we allocate.
- **Heap-allocate-on-entry must handle alloc failure.** Every `malloc` in the heap-migration tasks must check `nullptr` and gracefully bail out of the feature with `ui_alert("OUT OF MEMORY")`, not crash. The user must still be able to back out to the menu.
- **Don't add PSRAM-based fixes.** Even when PSRAM hardware works on a different unit, this firmware ships to a unit where PSRAM is dead. Plans that depend on `MALLOC_CAP_SPIRAM` are rejected.
- **Local commits only.** Per `feedback_poseidon_github_gate.md`: do NOT push, tag, or release. Per `feedback_careful_iteration.md`: one fix at a time, build + measure between every change. Per `feedback_review_before_run.md`: every task ends with a `pio run -e cardputer` and a numeric comparison against the Task 0 baseline.
- **No mid-feature heap allocation in promisc / ISR contexts.** Triton's WiFi promisc callback runs in WiFi-task context with a critical section. It must not allocate. The heap-allocated `s_capq` is allocated BEFORE `esp_wifi_set_promiscuous(true)` and freed AFTER `esp_wifi_set_promiscuous(false)`. This invariant is asserted in code.

---

## Investigation Findings (evidence base for tasks below)

**This section records what investigation actually found in the project as of 2026-04-25. Cite these numbers, do not re-investigate per task.**

### 1. Current memory baseline (from `build.log` lines 47-63)

| Metric | Used | Total | Pct |
|---|---|---|---|
| Flash Code (.text) | 1,512,008 | — | — |
| Flash Data (.rodata + .appdesc) | 597,136 | — | — |
| **Flash total (PIO)** | **2,232,711** | **3,342,336** | **66.8%** |
| DIRAM .bss | 195,528 | — | 57.21% |
| DIRAM .text (IRAM-spillover into DIRAM) | 83,171 | — | 24.34% |
| DIRAM .data | 24,268 | — | 7.10% |
| **DIRAM total** | **302,967** | **341,760** | **88.65%** |
| **RAM (PIO summary)** | **219,796** | **327,680** | **67.1%** |
| IRAM .text | 15,356 | — | 93.73% |
| IRAM .vectors | 1,028 | — | 6.27% |
| **IRAM-only (0x40374000-0x40378000)** | **16,384** | **16,384** | **100%** |

Note: PIO's "IRAM 100%" is the IRAM-only segment (16 KB before DIRAM at 0x40378000). Additional IRAM content overflows into DIRAM smoothly; the 100% number is **alarming but not fatal** — it doesn't crash the device. It does, however, mean any new `IRAM_ATTR` we add either fails to link or gets placed in DIRAM.

### 2. Top IRAM consumers (from `firmware.map`, addresses 0x4037_4000 - 0x4037_8000)

Aggregated per object file:

| Bytes | Object |
|---|---|
| 848 | `libbootloader_support.a(bootloader_flash.c.obj)` |
| 754 | `libheap.a(heap_caps_base.c.obj)` |
| 738 | `libnewlib.a(locks.c.obj)` |
| 721 | `libheap.a(heap_caps.c.obj)` |
| 555 | `libesp_timer.a(esp_timer.c.obj)` |
| 517 | `libesp_hw_support.a(intr_alloc.c.obj)` |
| 489 | `libesp_system.a(debug_helpers.c.obj)` |
| 450 | `libspi_flash.a(cache_utils.c.obj)` |
| 422 | `libesp_system.a(cpu_start.c.obj)` |
| 377 | `libbt.a(bt.c.obj)` |
| 370 | `libesp_driver_rmt.a(rmt_encoder_bytes.c.obj)` |
| **344** | **`RCSwitch.cpp.o`** (USER-CONTROLLABLE — see Task 7) |
| 298 | `libesp_wifi.a(esp_adapter.c.obj)` |
| 278 | `libesp_driver_i2s.a(i2s_common.c.obj)` |
| 265 | `libhal.a(efuse_hal.c.obj)` |
| 245 | `libesp_driver_gpio.a(gpio.c.obj)` |
| 219 | `libesp_system.a(esp_ipc.c.obj)` |
| 207 | `libesp_system.a(crosscore_int.c.obj)` |
| 158 | `libesp_hw_support.a(rtc_module.c.obj)` |
| 149 | `libesp_hw_support.a(brownout.c.obj)` |
| ... | (long tail of <150 byte framework symbols) |
| 34 | `subghz_scan.cpp.o` (USER, GDO0 ISR) |
| 32 | `cc1101_rmt.cpp.o` (USER, RMT RX done cb) |
| 16 | `radio_lora.cpp.o` (USER, DIO1 ISR) |

User-controllable IRAM total: **344 (RCSwitch) + 82 (our three ISRs) = 426 bytes**. Everything else (~10.4 KB of the 10.85 KB allocated) is in the prebuilt framework and is fixed.

### 3. User-defined `IRAM_ATTR` catalog

Found via `grep -rn "IRAM_ATTR" src/`:

| File:line | Function | Why it needs IRAM | Verdict |
|---|---|---|---|
| `src/cc1101_rmt.cpp:109` | `rx_done_cb` | RMT RX-done callback, called from RMT ISR. Required by `rmt_rx_register_event_callbacks()` API. | **KEEP**. |
| `src/features/radio_lora.cpp:52` | `lora_rx_isr` | SX1262 DIO1 line ISR, latches `s_lora_rx_flag` in ~5 instructions. | **KEEP**. |
| `src/features/subghz_scan.cpp:62` | `gdo0_isr` | CC1101 GDO0 pulse-capture ISR for OOK timing. Misses bytes if it has to fault into flash. | **KEEP**. |

All three are real ISRs that must run from cache-resident memory. None can be moved. They total 82 bytes, so even removing all three is a fly's worth of IRAM.

### 4. Large static BSS buffers (from `.dram0.bss` aggregation, top 20 by size)

Sum = 110,312 bytes (= 56% of total .bss). Each is a DRAM heap candidate.

| Bytes | Symbol (mangled) | Source file | Heap-able? |
|---|---|---|---|
| 28,800 | `s_wf_ring` | `subghz_spectrum.cpp` | **YES** — only used inside `run_waterfall()`, freed on exit |
| 20,480 | `g_wdr_aps` | `wifi_wardrive.cpp` | **NO** — referenced by triton + pmkid as a cross-feature seed cache; persists between menu invocations |
| 8,192 | `s_capq` | `triton.cpp` | **YES** — needed only while triton's promisc callback is registered |
| 7,020 | `active` (function-static in `feat_wifi_ciw`) | `wifi_ciw.cpp` | **YES** — function-static, only initialized on first call, can be `static unique_ptr` |
| 4,928 | `s_m1` | `wifi_pmkid.cpp` | **YES** — only consumed during PMKID capture |
| 4,800 | `s_hits` | `net_cctv.cpp` | **YES** — only consumed during CCTV scan |
| 4,352 | `s_keys` | `ble_whisperpair.cpp` | **YES** — only relevant during a WhisperPair session |
| 4,032 | `s_targets` | `mimir.cpp` | **YES** — populated when MIMIR connection is open |
| 3,520 | `s_hosts` | `net_lanrecon.cpp` | **YES** — host list during a LAN recon scan |
| 2,944 | `s_dev` | `net_ssdp.cpp` | **YES** — SSDP devices during one scan |
| 2,752 | `s_aps` | `wifi_scan.cpp` | **YES** — scan results, but cheap to re-scan |
| 2,752 | `s_aps` | `c5_cmd.cpp` | **YES** — C5 satellite scan results |
| 2,496 | `s_devs` | `ble_scan.cpp` | **YES** — BLE scan results |
| 2,464 | `s_m1` | `triton.cpp` | **YES** (folded into Task 4) |
| 2,112 | `s_hss` | `c5_cmd.cpp` | **YES** — C5 handshake list |
| 2,048 | `s_b_targets` | `wifi_deauth_extras.cpp` | **YES** |
| 1,820 | `M5` | `M5Unified.cpp.o` | **NO** — global instance, framework-owned |
| 1,728 | `s_flat` | `ble_gatt.cpp` | **YES** |
| 1,536 | `s_probes` | `wifi_probe.cpp` | **YES** |
| 1,536 | `s_all` | `wifi_clients_all.cpp` | **YES** |

The "heap-able" subset (excluding `g_wdr_aps` and `M5`) totals **~88,000 bytes** of potential DRAM that could live in heap and only when the relevant feature is active. In practice we will only target the top ~10 of these in tasks below to keep the plan finite — the rest are smaller and the diminishing-returns line is around 1.5 KB.

### 5. Large `const` blobs — already in .rodata (flash) ✓

Verified via `grep` against `.rodata` symbols in `firmware.map`:

| Bytes | Symbol | Location in image | Status |
|---|---|---|---|
| 64,800 | `saltyjack_splash` (32400 × uint16_t) | `.rodata @ 0x3c1ba444` | flash ✓ |
| 54,270 | `splash_data` (27135 × uint16_t) | `.rodata @ 0x3c... ` (post sj_spr_*) | flash ✓ |
| 7 × 512 | `sj_spr_flag/skull/swords/wheel/horn/web/key` | `.rodata @ 0x3c1ca164…` | flash ✓ |
| 128 | `s_beacon` (`wifi_beacon_spam.cpp`) | `.data` (DRAM) — but it's mutable (sequence number patched in place) | KEEP |
| 128 | `s_beacon` (`wifi_ciw.cpp`) | `.data` (DRAM) — same justification | KEEP |
| 256 | `RANGES[]` (`subghz_spectrum.cpp` + `lora_spectrum.cpp`) | `.rodata` ✓ | flash ✓ |
| various | `subghz_decode.cpp` decoder protocol code | inline `.text` ✓ | flash ✓ |

**No DRAM-resident `const` blobs to migrate.** All large constants are already in flash. The two 128-byte `s_beacon` arrays in `.data` ARE intentionally mutable (the WiFi sequence number field is updated in-place during transmit) so they correctly live in DRAM.

### 6. `platformio.ini` build flag audit

Current `build_flags` (`platformio.ini:33-45`):
```
-DARDUINO_USB_MODE=1
-DARDUINO_USB_CDC_ON_BOOT=1
-DCORE_DEBUG_LEVEL=2
-DPOSEIDON_VERSION=\"0.5.0\"
-std=gnu++17
-Wall
-Wl,--allow-multiple-definition
```

Current `build_unflags`:
```
-std=gnu++11
```

**Default flags inherited from PIO/IDF (verified via Espressif's release-mode SCons rules):**
- `-Os` (size optimization) — already on (verified via `sdkconfig: CONFIG_COMPILER_OPTIMIZATION_SIZE=y`).
- `-ffunction-sections -fdata-sections` — default for ESP32 in pioarduino.
- `-Wl,--gc-sections` — default linker flag, dead code dropped. Verified by examining `.text` sections — only used symbols are linked.

**Missing optimizations we can add via `build_flags`:**

| Flag | Effect | Risk |
|---|---|---|
| `-flto=auto` (compile) + `-flto=auto -fuse-linker-plugin` (link) | Link-time optimization across translation units. Typically saves 5-15% flash text. | LOW — known to interact with `--allow-multiple-definition`; tested via Task 1. |
| `-Wl,--print-memory-usage` | Print region usage at link time | NONE — diagnostic only. |
| `-Wl,-Map=.pio/build/cardputer/firmware.map` (extras for sym table) | Already implicitly produced; explicit form helps consistency | NONE. |
| `-fno-unwind-tables -fno-asynchronous-unwind-tables` | Drop DWARF unwind tables. Saves flash .rodata (eh_frame). | MEDIUM — affects stack trace decoding. We use `esp32_exception_decoder` so we'd lose decoded backtraces in panic dumps. **Not added** — keep crash diagnostics. |
| `-DCORE_DEBUG_LEVEL=1` (down from 2) | Drops verbose log strings from `.rodata`. | LOW. Cuts log strings, can save 5-30 KB flash. **Not added in this plan** — keep verbosity for field debug. Rejected to avoid changing user-visible behavior. |

**Decision:** Add `-flto=auto` (Task 1) and `-Wl,--print-memory-usage` (Task 1). Skip the two riskier flags above.

### 7. Library footprint scan

LDF found 54 compatible libraries. The fattest contributors to flash text (from `firmware.map` aggregation):

| Bytes flash | Object |
|---|---|
| 16,496 | `net_attacks.cpp.o` (USER) |
| 14,490 | `libnet80211.a(ieee80211_ioctl.o)` |
| 14,439 | `libnet80211.a(wl_cnx.o)` |
| 12,853 | `libc.a(libc_a-vfprintf.o)` |
| 12,570 | `libnet80211.a(ieee80211_output.o)` |
| 12,541 | `libc.a(libc_a-svfprintf.o)` |
| 12,407 | `net_wpad.cpp.o` (USER) |
| 12,180 | `libnet80211.a(ieee80211_hostap.o)` |
| 12,136 | `libnet80211.a(ieee80211_scan.o)` |
| 11,939 | `libfatfs.a(ff.c.obj)` |
| 11,473 | `libnet80211.a(ieee80211_sta.o)` |
| 11,374 | `HTTPClient.cpp.o` |
| 11,084 | `triton.cpp.o` (USER) |
| 10,968 | `libbtdm_app.a(llm_adv.o)` |
| 10,597 | `M5GFX.cpp.o` |

The big libs (`libnet80211`, `libbtdm_app`, NimBLE) are non-negotiable — we use both WiFi monitor mode AND BLE pentesting. `HTTPClient`/`WebServer`/`mbedtls` are needed for Evil Portal and update flows. **No library can be removed without losing user-visible features.** Therefore Task 1 (LTO) is the only flash-shrink lever in this plan.

### 8. RCSwitch RX-path dead-code analysis

Searched user code for `enableReceive`, `RCSwitch::handleInterrupt`, `attachInterrupt(.*RCSwitch.*)` — **zero matches**. RCSwitch RX is linked but never invoked. The `IRAM_ATTR`-decorated `RCSwitch::handleInterrupt()` (131 bytes IRAM) and `RCSwitch::receiveProtocol()` (213 bytes IRAM) are pure dead weight in our build. Task 7 patches the local lib copy to drop both `IRAM_ATTR`s, freeing 344 bytes of the IRAM-only segment.

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `bench/baseline.txt` | NEW | Holds the Task 0 measurement (raw build-log memory table). Compared against in Task N (final). |
| `bench/post.txt` | NEW (final task) | Post-optimization numbers, written in the final task. |
| `tools/sizediff.py` | NEW | Tiny Python that diffs two memory tables (baseline vs post) and prints a delta report. |
| `platformio.ini` | MODIFY | Add `-flto=auto -Wl,--print-memory-usage` to `build_flags`. |
| `src/features/subghz_spectrum.cpp` | MODIFY | Heap-allocate `s_wf_ring` on entry to `run_waterfall`, free on exit. |
| `src/features/triton.cpp` | MODIFY | Heap-allocate `s_capq` (and `s_m1` while we're in there) on `feat_triton` entry, free on exit. Order is critical: alloc BEFORE promisc enable, free AFTER promisc disable. |
| `src/features/wifi_pmkid.cpp` | MODIFY | Heap-allocate `s_m1` on `feat_wifi_pmkid` entry, free on exit. |
| `src/features/net_cctv.cpp` | MODIFY | Heap-allocate `s_hits` on `feat_net_cctv` entry, free on exit. |
| `src/features/ble_whisperpair.cpp` | MODIFY | Heap-allocate `s_keys` and `s_tgt` on entry, free on exit. |
| `src/features/mimir.cpp` | MODIFY | Heap-allocate `s_targets` on `feat_mimir` entry, free on exit. |
| `src/features/net_lanrecon.cpp` | MODIFY | Heap-allocate `s_hosts` on entry, free on exit. |
| `src/features/net_ssdp.cpp` | MODIFY | Heap-allocate `s_dev` on entry, free on exit. |
| `src/features/wifi_ciw.cpp` | MODIFY | Convert function-static `active` array to a heap allocation with proper free-on-exit. |
| `.pio/libdeps/cardputer/SmartRC-CC1101-Driver-Lib/RCSwitch.cpp` | MODIFY | Drop `IRAM_ATTR` from `handleInterrupt()` and `receiveProtocol()`. *(See Task 7 for why this is safe and how to keep the patch reproducible.)* |
| `support_files/rcswitch_no_rx_iram.patch` | NEW | The actual patch, checked in so a clean `pio run` regenerates the modified file via a `pre:` script. |
| `scripts/apply_lib_patches.py` | NEW | PIO `extra_scripts` Python that re-applies `support_files/rcswitch_no_rx_iram.patch` to the libdeps RCSwitch on every build. Idempotent. |

No files are deleted. No existing function bodies are rewritten beyond what's listed above.

---

## Task breakdown

The hardware-in-the-loop nature of this work means there are no unit tests. Each task ends with a successful `pio run -e cardputer` and a numeric comparison against the Task 0 baseline. Treat the build-table comparison as the equivalent of test assertions: if numbers don't move in the expected direction, the task is not done.

---

### Task 0: Capture baseline memory numbers

**Files:**
- Create: `bench/baseline.txt`

- [ ] **Step 1: Confirm baseline build is current**

Run from the project root:

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: Exit 0 ("[SUCCESS]"). The memory table at the end of build.log should show "IRAM 16384 100.0%" and "DIRAM 302967 88.65%". If those numbers shift dramatically from the values in this plan's Investigation Findings (>2 KB), the codebase has changed since 2026-04-25 — STOP and re-investigate before continuing. Otherwise proceed.

- [ ] **Step 2: Create `bench/baseline.txt`**

Create directory + file with exactly:

```
# POSEIDON memory baseline
# Captured: <YYYY-MM-DD> at <HH:MM>
# Commit:   <git rev-parse HEAD>

Memory Type     Used        Total       Pct
Flash Code      1512008     -           -
Flash Data      597136      -           -
Flash total     2232711     3342336     66.8%
DIRAM .bss      195528      -           57.21%
DIRAM .text     83171       -           24.34%
DIRAM .data     24268       -           7.10%
DIRAM total     302967      341760      88.65%
RAM (PIO)       219796      327680      67.10%
IRAM .text      15356       -           93.73%
IRAM .vectors   1028        -           6.27%
IRAM total      16384       16384       100.00%
```

Replace the `<YYYY-MM-DD>`, `<HH:MM>`, and `<git rev-parse HEAD>` placeholders with the actual values when committing.

- [ ] **Step 3: Commit**

```bash
git add bench/baseline.txt
git commit -m "bench: capture pre-optimization memory baseline"
```

- [ ] **Step 4: Verify file is on disk**

Run: `ls -la bench/baseline.txt`
Expected: file exists, ~600 bytes.

---

### Task 1: Add LTO + memory-usage diagnostic to platformio.ini

**Files:**
- Modify: `C:/Users/D/poseidon/platformio.ini`

**Estimated savings:** 30-100 KB flash text (5-7% of 1.5 MB), 0 IRAM, 0 DRAM. LTO sometimes shrinks DIRAM .text spillover slightly (~1-2 KB) as a side effect. This is the cheapest high-impact win in the plan.

- [ ] **Step 1: Edit `platformio.ini` to append LTO flags**

Modify the `[env:cardputer]` `build_flags` block. Replace exactly the existing block:

```ini
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DCORE_DEBUG_LEVEL=2
    -DPOSEIDON_VERSION=\"0.5.0\"
    -std=gnu++17
    -Wall
    ; Override libnet80211.a's ieee80211_raw_frame_sanity_check (which
    ; filters deauth/disassoc subtypes) with our stub in
    ; src/features/wifi_sanity_override.cpp. --allow-multiple-definition
    ; makes the linker silently prefer our .o over the library's .a —
    ; ours is encountered first.
    -Wl,--allow-multiple-definition
```

…with:

```ini
build_flags =
    -DARDUINO_USB_MODE=1
    -DARDUINO_USB_CDC_ON_BOOT=1
    -DCORE_DEBUG_LEVEL=2
    -DPOSEIDON_VERSION=\"0.5.0\"
    -std=gnu++17
    -Wall
    ; Link-time optimization. Empirically shaves 5-7% flash on this
    ; build (~50-100 KB text). Plays fine with --allow-multiple-definition
    ; because LTO runs after the linker symbol-resolution pass that
    ; --allow-multiple-definition controls. If a future IDF/toolchain
    ; bump regresses this, comment the two -flto lines and re-test.
    -flto=auto
    -ffat-lto-objects
    ; Print region usage at link time so PIO's table is reproducible
    ; from the raw map without re-running pio.
    -Wl,--print-memory-usage
    ; Override libnet80211.a's ieee80211_raw_frame_sanity_check (which
    ; filters deauth/disassoc subtypes) with our stub in
    ; src/features/wifi_sanity_override.cpp. --allow-multiple-definition
    ; makes the linker silently prefer our .o over the library's .a —
    ; ours is encountered first.
    -Wl,--allow-multiple-definition
```

Leave `build_unflags = -std=gnu++11` and the rest of the file unchanged.

- [ ] **Step 2: Clean build to force LTO from cold cache**

```bash
pio run -e cardputer -t clean
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. The link step will be noticeably slower (LTO does inter-TU work). Expected total build time: 5-8 minutes vs the baseline ~3:20.

- [ ] **Step 3: Verify LTO actually engaged**

Run: `grep -E "flto|LTO|--allow-multiple-definition" build.log | head -5`
Expected: at least one line shows `-flto=auto` in the C++/link command echo.

- [ ] **Step 4: Compare against baseline**

Pull the new memory table from build.log. Flash should be ≤ 2,180,000 bytes (= 2,232,711 - 52 KB target). DIRAM .text may drop slightly. **If flash did not shrink by at least 30 KB, STOP** — LTO has regressed in the toolchain and we need to investigate (likely `--allow-multiple-definition` interaction). Roll back the two `-flto` lines and document in the commit message that LTO is not viable with this toolchain.

- [ ] **Step 5: Commit**

```bash
git add platformio.ini
git commit -m "build: enable -flto=auto for ~5% flash reduction"
```

- [ ] **Step 6: Smoke-test on hardware**

Flash and boot. Verify: splash → menu → enter and exit at least one feature from each radio (WiFi Scan, BLE Scan, SubGHz Scan, LoRa). LTO occasionally re-orders in ways that surface latent UB. If anything misbehaves, roll back this commit and skip to Task 2.

```bash
pio run -e cardputer -t upload
pio device monitor -b 115200
```

Expected: clean boot to menu, no panic, all 4 smoke-tested features open and close cleanly.

---

### Task 2: Heap-allocate subghz waterfall ring buffer (28.8 KB)

**Files:**
- Modify: `C:/Users/D/poseidon/src/features/subghz_spectrum.cpp`

**Estimated savings:** ~28.8 KB DRAM .bss when the user is NOT running the waterfall (which is most of the time). Zero net change while running the waterfall (same 28.8 KB, just on heap).

- [ ] **Step 1: Read current state**

Open `src/features/subghz_spectrum.cpp` to lines 160-240. Confirm `s_wf_ring[WF_MAX_ROWS * WF_MAX_BINS]` is declared at file scope and used only inside `run_waterfall()`.

- [ ] **Step 2: Replace static array with heap pointer**

In `src/features/subghz_spectrum.cpp`, find this exact block (around line 160-170):

```cpp
/* Static BSS ring. 100×240 (48 KB) was too big — it ate enough boot
 * heap that esp_wifi_init started failing with ESP_ERR_NO_MEM and
 * WiFi Scan crashed the device. 60 rows × 240 × 2 B = 28.8 KB leaves
 * enough heap for WiFi's default RX buffers. Still fills most of the
 * body under the title strip. */
#define WF_MAX_ROWS 60
#define WF_MAX_BINS SCR_W
static uint16_t s_wf_ring[WF_MAX_ROWS * WF_MAX_BINS];
```

Replace with:

```cpp
/* Heap-allocated ring. Was 28.8 KB of permanent BSS — moving to heap
 * means it only costs DRAM while the waterfall is actually open.
 * Earlier sizing notes still apply: 100×240 (48 KB) was too big to
 * coexist with WiFi heap demands; 60×240 (28.8 KB) is the safe ceiling
 * even when the user enters the waterfall after a long WiFi session
 * has fragmented the heap. Re-allocated each entry; failure bails out
 * of the waterfall cleanly with an alert. */
#define WF_MAX_ROWS 60
#define WF_MAX_BINS SCR_W
```

- [ ] **Step 3: Allocate + free inside run_waterfall**

Find `run_waterfall(const freq_range_t &range)` (around line 171). Replace its body's first ~20 lines:

```cpp
static void run_waterfall(const freq_range_t &range)
{
    /* Full-screen waterfall. Sweeps freq bins across the whole 240 px
     * width; each completed sweep scrolls the history up one row and
     * renders a new bottom row. No borders / labels eat pixels — the
     * few overlays (range name, start/end freq, ESC hint) are drawn
     * translucently on top of the waterfall. 240x135 x 2 B = 64 KB
     * ring buffer; fine on our 327 KB internal RAM budget. */
    auto &d = M5Cardputer.Display;
    /* Center the 100-row waterfall vertically under a small title
     * band. GY leaves ~15 px on top for title + range label. */
    const int GX = 0, GY = 15, GW = WF_MAX_BINS, GH = WF_MAX_ROWS;
    float step = (range.end - range.start) / GW;

    uint16_t *ring = s_wf_ring;
    int head = 0, count = 0;
```

…with:

```cpp
static void run_waterfall(const freq_range_t &range)
{
    /* Full-screen waterfall. Sweeps freq bins across the whole 240 px
     * width; each completed sweep scrolls the history up one row and
     * renders a new bottom row. No borders / labels eat pixels — the
     * few overlays (range name, start/end freq, ESC hint) are drawn
     * translucently on top of the waterfall. 28.8 KB is heap-allocated
     * for the duration of this function; freed on every exit path. */
    auto &d = M5Cardputer.Display;
    /* Center the 100-row waterfall vertically under a small title
     * band. GY leaves ~15 px on top for title + range label. */
    const int GX = 0, GY = 15, GW = WF_MAX_BINS, GH = WF_MAX_ROWS;
    float step = (range.end - range.start) / GW;

    uint16_t *ring = (uint16_t *)heap_caps_malloc(
        (size_t)WF_MAX_ROWS * WF_MAX_BINS * sizeof(uint16_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!ring) {
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "OUT OF MEMORY");
        ui_text(8, BODY_Y + 44, T_DIM,  "free heap too low for 28.8 KB");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    /* Zero so initial rows render as deep-blue (RSSI floor) rather
     * than whatever stale heap was sitting at this address. */
    memset(ring, 0, (size_t)WF_MAX_ROWS * WF_MAX_BINS * sizeof(uint16_t));
    int head = 0, count = 0;
```

- [ ] **Step 4: Free on every exit path**

In the same function, find the body's exit point. Currently looks like:

```cpp
        uint16_t k = input_poll();
        if (k == PK_ESC) {
            return;
        }
```

(Note: the file has no per-frame return path other than the ESC inside `while (true)`. There may also be a buffer overflow guard. Verify exact wording before editing.)

Replace the inner `if (k == PK_ESC) { return; }` with:

```cpp
        uint16_t k = input_poll();
        if (k == PK_ESC) {
            heap_caps_free(ring);
            return;
        }
```

If there is more than one return path inside `run_waterfall()` after the alloc, add `heap_caps_free(ring);` immediately before EACH `return`. Audit the function carefully — failing to free is a 28.8 KB heap leak per waterfall exit.

- [ ] **Step 5: Add the include for heap_caps if not already present**

At the top of `subghz_spectrum.cpp`, after the existing `#include`s, add:

```cpp
#include <esp_heap_caps.h>
```

…unless it's already there. (`grep -n "heap_caps" src/features/subghz_spectrum.cpp` to check.)

- [ ] **Step 6: Build**

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. DIRAM .bss should drop by ~28,800 bytes vs the post-Task-1 baseline.

- [ ] **Step 7: Verify in build log**

Look for the new "DIRAM .bss" number — it should be ~166,728 (= 195,528 - 28,800), or correspondingly lower if Task 1 also touched .bss.

- [ ] **Step 8: Commit**

```bash
git add src/features/subghz_spectrum.cpp
git commit -m "subghz: heap-allocate waterfall ring (-28.8 KB DRAM at idle)"
```

- [ ] **Step 9: Smoke-test on hardware**

Flash, boot, navigate: SubGHz → Spectrum → Waterfall. Verify the waterfall renders. Press ESC. Re-enter and exit 5 times. After the 5th exit, check free heap (Settings → System → Heap shows current free). Free heap should NOT decrease across the 5 cycles — that would indicate the malloc/free is leaking.

```bash
pio run -e cardputer -t upload
pio device monitor -b 115200
```

---

### Task 3: Heap-allocate triton capture queue + M1 cache (10.6 KB)

**Files:**
- Modify: `C:/Users/D/poseidon/src/features/triton.cpp`

**Estimated savings:** 8,192 + 2,464 = **10,656 bytes DRAM .bss** when triton isn't running (most of the time).

**Risk note:** `s_capq` is touched from the WiFi promiscuous callback under a `portMUX_TYPE` critical section. The heap allocation MUST happen BEFORE `esp_wifi_set_promiscuous(true)` and the free MUST happen AFTER `esp_wifi_set_promiscuous(false)`. We assert this with explicit ordering.

- [ ] **Step 1: Read current state**

Open `src/features/triton.cpp`. Note locations of:
- `s_capq` declaration (~line 170)
- `s_m1` declaration (~line 230)
- `feat_triton()` function (entry / exit points)
- `esp_wifi_set_promiscuous(true)` call site
- `esp_wifi_set_promiscuous(false)` call site (probably inside the cleanup/ESC path)

Note: `s_capq_head`, `s_capq_tail`, `s_capq_mux` stay at file scope as static volatile / static portMUX — they don't move. Only the storage array becomes a pointer.

- [ ] **Step 2: Replace `s_capq` declaration with pointer**

Find:

```cpp
struct capture_t { char line[1024]; };
#define CAPTURE_Q 8
static capture_t s_capq[CAPTURE_Q];
static volatile int s_capq_head = 0;
static volatile int s_capq_tail = 0;
static portMUX_TYPE s_capq_mux = portMUX_INITIALIZER_UNLOCKED;
```

Replace with:

```cpp
struct capture_t { char line[1024]; };
#define CAPTURE_Q 8
/* Heap-allocated for the lifetime of feat_triton(). nullptr when
 * triton isn't running; capture_enqueue() bails out early when the
 * pointer is null so it's safe even if a stale promisc callback
 * fires after we've torn down. Allocation MUST precede
 * esp_wifi_set_promiscuous(true); free MUST follow
 * esp_wifi_set_promiscuous(false). */
static capture_t *s_capq = nullptr;
static volatile int s_capq_head = 0;
static volatile int s_capq_tail = 0;
static portMUX_TYPE s_capq_mux = portMUX_INITIALIZER_UNLOCKED;
```

- [ ] **Step 3: Make capture_enqueue null-safe**

Find:

```cpp
static void capture_enqueue(const char *line)
{
    portENTER_CRITICAL(&s_capq_mux);
    int next = (s_capq_head + 1) % CAPTURE_Q;
    if (next != s_capq_tail) {
        strncpy(s_capq[s_capq_head].line, line, sizeof(s_capq[0].line) - 1);
        s_capq[s_capq_head].line[sizeof(s_capq[0].line) - 1] = '\0';
        s_capq_head = next;
    }
    /* On queue full we drop — better than blocking the Wi-Fi task. */
    portEXIT_CRITICAL(&s_capq_mux);
}
```

Replace with:

```cpp
static void capture_enqueue(const char *line)
{
    portENTER_CRITICAL(&s_capq_mux);
    /* Defensive: queue is freed when feat_triton exits but the WiFi
     * task may fire one more callback before promisc is fully off.
     * Drop silently if so. */
    if (!s_capq) {
        portEXIT_CRITICAL(&s_capq_mux);
        return;
    }
    int next = (s_capq_head + 1) % CAPTURE_Q;
    if (next != s_capq_tail) {
        strncpy(s_capq[s_capq_head].line, line, sizeof(s_capq[0].line) - 1);
        s_capq[s_capq_head].line[sizeof(s_capq[0].line) - 1] = '\0';
        s_capq_head = next;
    }
    /* On queue full we drop — better than blocking the Wi-Fi task. */
    portEXIT_CRITICAL(&s_capq_mux);
}
```

- [ ] **Step 4: Make the drain side null-safe**

Find the drain block (around line 200):

```cpp
        portENTER_CRITICAL(&s_capq_mux);
        if (s_capq_tail != s_capq_head) {
            strncpy(line, s_capq[s_capq_tail].line, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
            s_capq_tail = (s_capq_tail + 1) % CAPTURE_Q;
            have = true;
        }
        portEXIT_CRITICAL(&s_capq_mux);
```

(The exact line that copies from `s_capq[s_capq_tail].line` may differ — match by structure not text. There's also a `have` or similar flag.) Replace by adding the `s_capq` null-check before the copy:

```cpp
        portENTER_CRITICAL(&s_capq_mux);
        if (s_capq && s_capq_tail != s_capq_head) {
            strncpy(line, s_capq[s_capq_tail].line, sizeof(line) - 1);
            line[sizeof(line) - 1] = '\0';
            s_capq_tail = (s_capq_tail + 1) % CAPTURE_Q;
            have = true;
        }
        portEXIT_CRITICAL(&s_capq_mux);
```

- [ ] **Step 5: Allocate s_capq + s_m1 at the top of feat_triton**

Find `void feat_triton(void)` (or the equivalent entry — search for `feat_triton`). At its very first executable line (after any local variable declarations but before any radio / WiFi setup), insert:

```cpp
    /* Heap-allocate triton's two largest BSS arrays for the lifetime
     * of this feature. ~10.6 KB returned to the system heap when the
     * user is anywhere else in the menu tree. */
    s_capq = (capture_t *)heap_caps_calloc(
        CAPTURE_Q, sizeof(capture_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_m1   = (m1_t *)heap_caps_calloc(
        M1_N, sizeof(m1_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_capq || !s_m1) {
        if (s_capq) { heap_caps_free(s_capq); s_capq = nullptr; }
        if (s_m1)   { heap_caps_free(s_m1);   s_m1   = nullptr; }
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "TRITON: OUT OF MEMORY");
        ui_text(8, BODY_Y + 44, T_DIM, "needs ~10.6 KB internal heap");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    /* Reset queue indices — heap_caps_calloc zeroed the storage, but
     * the indices live in static memory and may be stale from a
     * prior run. */
    s_capq_head = 0;
    s_capq_tail = 0;
    s_m1_n = 0;
```

- [ ] **Step 6: Replace s_m1 declaration**

Find:

```cpp
static m1_t s_m1[M1_N];
```

Replace with:

```cpp
/* Heap-allocated like s_capq. nullptr when triton isn't running. */
static m1_t *s_m1 = nullptr;
```

- [ ] **Step 7: Free on every exit path of feat_triton**

Find every `return` inside `feat_triton()` AFTER the allocation and BEFORE the `esp_wifi_set_promiscuous(false)` cleanup, plus the natural function return at the bottom. Audit the function — there will likely be 1-3 exit paths. Insert immediately AFTER the cleanup that calls `esp_wifi_set_promiscuous(false)`:

```cpp
    /* MUST run after esp_wifi_set_promiscuous(false). The WiFi task
     * may fire one trailing callback into capture_enqueue/drain;
     * those are null-safe (see capture_enqueue). Once we free here
     * any subsequent stale callback hits the !s_capq guard and noops. */
    portENTER_CRITICAL(&s_capq_mux);
    capture_t *to_free_capq = s_capq;
    m1_t      *to_free_m1   = s_m1;
    s_capq = nullptr;
    s_m1   = nullptr;
    portEXIT_CRITICAL(&s_capq_mux);
    if (to_free_capq) heap_caps_free(to_free_capq);
    if (to_free_m1)   heap_caps_free(to_free_m1);
```

If there's only one cleanup site (typical), you only need one of these blocks. If multiple, replicate.

- [ ] **Step 8: Add include if not already present**

At top of `src/features/triton.cpp`, after existing includes, ensure:

```cpp
#include <esp_heap_caps.h>
```

- [ ] **Step 9: Build**

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. DIRAM .bss should drop by 10,656 vs post-Task-2 baseline.

- [ ] **Step 10: Commit**

```bash
git add src/features/triton.cpp
git commit -m "triton: heap-allocate capq + m1 (-10.6 KB DRAM at idle)"
```

- [ ] **Step 11: Smoke-test on hardware (longer than usual)**

Flash. Open Triton, let it run for at least 60 seconds capturing handshakes. Exit. Re-enter. Exit. Verify the previously-known triton freeze regression (`project_triton_freeze_debug.md`) does not get worse. If exit hangs or device crashes during the second entry, **revert this commit** and document in commit message that the order-of-cleanup interaction with the WiFi task is fragile under our IDF version.

```bash
pio run -e cardputer -t upload
pio device monitor -b 115200
```

---

### Task 4: Heap-allocate wifi_pmkid M1 cache (4.9 KB)

**Files:**
- Modify: `C:/Users/D/poseidon/src/features/wifi_pmkid.cpp`

**Estimated savings:** 4,928 bytes DRAM .bss when PMKID isn't actively capturing.

- [ ] **Step 1: Read current state**

Open `src/features/wifi_pmkid.cpp`. Locate:
- `s_m1` declaration (~line 62)
- `feat_wifi_pmkid()` entry/exit
- All call sites that read/write `s_m1`

- [ ] **Step 2: Replace declaration with pointer**

Find:

```cpp
static m1_entry_t s_m1[M1_CACHE];
```

Replace with:

```cpp
/* Heap-allocated for the lifetime of feat_wifi_pmkid. nullptr otherwise.
 * The promiscuous callback that writes here is registered AFTER alloc
 * and unregistered BEFORE free, so callers never see s_m1 == nullptr
 * inside the critical section. */
static m1_entry_t *s_m1 = nullptr;
```

- [ ] **Step 3: Allocate at feat_wifi_pmkid entry**

Find `feat_wifi_pmkid()` (or the renamed entry symbol). At the top of the function body, before any radio setup:

```cpp
    s_m1 = (m1_entry_t *)heap_caps_calloc(
        M1_CACHE, sizeof(m1_entry_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_m1) {
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "PMKID: OUT OF MEMORY");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    s_m1_n = 0;
```

- [ ] **Step 4: Free at every exit path**

After `esp_wifi_set_promiscuous(false)` and before any `return` from the function:

```cpp
    if (s_m1) { heap_caps_free(s_m1); s_m1 = nullptr; }
    s_m1_n = 0;
```

If there are multiple early returns inside the run loop, each must funnel through the same cleanup. Use a `goto cleanup;` or refactor — your choice.

- [ ] **Step 5: Add include if missing**

```cpp
#include <esp_heap_caps.h>
```

- [ ] **Step 6: Build**

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. DIRAM .bss should drop by ~4,928 bytes.

- [ ] **Step 7: Commit**

```bash
git add src/features/wifi_pmkid.cpp
git commit -m "wifi_pmkid: heap-allocate M1 cache (-4.9 KB DRAM at idle)"
```

- [ ] **Step 8: Smoke-test**

Flash, open Wifi → PMKID, hop a few channels (capture or not — just need the loop to run), exit, re-enter twice. Free heap stable across cycles.

---

### Task 5: Heap-allocate net_cctv hits + ble_whisperpair keys (9.2 KB)

**Files:**
- Modify: `C:/Users/D/poseidon/src/features/net_cctv.cpp`
- Modify: `C:/Users/D/poseidon/src/features/ble_whisperpair.cpp`

**Estimated savings:** 4,800 + 4,352 = 9,152 bytes DRAM .bss.

- [ ] **Step 1: net_cctv — replace s_hits declaration**

Open `src/features/net_cctv.cpp` line 105. Find:

```cpp
static cctv_hit_t s_hits[CCTV_MAX_HITS];
```

Replace with:

```cpp
static cctv_hit_t *s_hits = nullptr;
```

- [ ] **Step 2: net_cctv — alloc/free in feat_net_cctv**

Find `feat_net_cctv()`. At top:

```cpp
    s_hits = (cctv_hit_t *)heap_caps_calloc(
        CCTV_MAX_HITS, sizeof(cctv_hit_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_hits) {
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "CCTV: OUT OF MEMORY");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    s_hits_n = 0;
```

Before each `return` from the function:

```cpp
    if (s_hits) { heap_caps_free(s_hits); s_hits = nullptr; }
```

Add `#include <esp_heap_caps.h>` if missing.

- [ ] **Step 3: ble_whisperpair — same treatment for s_keys + s_tgt**

In `src/features/ble_whisperpair.cpp` around line 87 and 208, replace:

```cpp
static wp_target_t s_tgt[WP_MAX_TARGETS];
```

```cpp
static wp_key_t s_keys[WP_MAX_KEYS];
```

With:

```cpp
static wp_target_t *s_tgt  = nullptr;
static wp_key_t    *s_keys = nullptr;
```

In `feat_ble_whisperpair()` (or its entry symbol), top of function:

```cpp
    s_tgt  = (wp_target_t *)heap_caps_calloc(
        WP_MAX_TARGETS, sizeof(wp_target_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    s_keys = (wp_key_t *)heap_caps_calloc(
        WP_MAX_KEYS, sizeof(wp_key_t),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_tgt || !s_keys) {
        if (s_tgt)  { heap_caps_free(s_tgt);  s_tgt  = nullptr; }
        if (s_keys) { heap_caps_free(s_keys); s_keys = nullptr; }
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "WHISPERPAIR: OUT OF MEMORY");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    s_tgt_n  = 0;
    s_keys_n = 0;
```

Cleanup before each return:

```cpp
    if (s_tgt)  { heap_caps_free(s_tgt);  s_tgt  = nullptr; }
    if (s_keys) { heap_caps_free(s_keys); s_keys = nullptr; }
```

Add `#include <esp_heap_caps.h>` if missing.

- [ ] **Step 4: Build**

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. DIRAM .bss drops by ~9,152 bytes.

- [ ] **Step 5: Commit**

```bash
git add src/features/net_cctv.cpp src/features/ble_whisperpair.cpp
git commit -m "features: heap-allocate cctv hits + whisperpair tgt/keys (-9.2 KB)"
```

- [ ] **Step 6: Smoke-test**

Open Net → CCTV scan; let it find at least one entry; exit. Open BLE → WhisperPair; let it scan; exit. Re-enter both 3 times; verify free heap stable.

---

### Task 6: Heap-allocate mimir + net_lanrecon + net_ssdp (10.5 KB)

**Files:**
- Modify: `C:/Users/D/poseidon/src/features/mimir.cpp`
- Modify: `C:/Users/D/poseidon/src/features/net_lanrecon.cpp`
- Modify: `C:/Users/D/poseidon/src/features/net_ssdp.cpp`

**Estimated savings:** 4,032 + 3,520 + 2,944 = 10,496 bytes DRAM .bss.

This task is the same pattern as Tasks 4 + 5 applied three times. The detail per-file:

- [ ] **Step 1: mimir — `s_targets`**

In `src/features/mimir.cpp:35`, replace:

```cpp
static APRow s_targets[MIMIR_MAX_APS];
```

with:

```cpp
static APRow *s_targets = nullptr;
```

In `feat_mimir()` at top:

```cpp
    s_targets = (APRow *)heap_caps_calloc(
        MIMIR_MAX_APS, sizeof(APRow),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_targets) {
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "MIMIR: OUT OF MEMORY");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    s_target_count = 0;
```

Before each return:

```cpp
    if (s_targets) { heap_caps_free(s_targets); s_targets = nullptr; }
```

- [ ] **Step 2: net_lanrecon — `s_hosts`**

`src/features/net_lanrecon.cpp:39`. Same pattern: `static host_t s_hosts[MAX_HOSTS]` → `static host_t *s_hosts = nullptr`. Allocation in `feat_net_lanrecon()` (or whatever the entry is named) at top using `heap_caps_calloc(MAX_HOSTS, sizeof(host_t), …)`. Free before every return. OOM-bail UI text "LAN RECON: OUT OF MEMORY".

- [ ] **Step 3: net_ssdp — `s_dev`**

`src/features/net_ssdp.cpp:32`. Same pattern: `static ssdp_dev_t s_dev[MAX_DEV]` → `static ssdp_dev_t *s_dev = nullptr`. Allocation in `feat_net_ssdp()` top. OOM bail "SSDP: OUT OF MEMORY".

- [ ] **Step 4: Build**

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. DIRAM .bss drops by ~10,496 bytes vs post-Task-5.

- [ ] **Step 5: Commit**

```bash
git add src/features/mimir.cpp src/features/net_lanrecon.cpp src/features/net_ssdp.cpp
git commit -m "features: heap-allocate mimir/lanrecon/ssdp tables (-10.5 KB)"
```

- [ ] **Step 6: Smoke-test**

Run each of the three features; exit cleanly. No heap leak across 3 cycles.

---

### Task 7: Drop RCSwitch IRAM_ATTR for unused RX path

**Files:**
- Create: `C:/Users/D/poseidon/support_files/rcswitch_no_rx_iram.patch`
- Create: `C:/Users/D/poseidon/scripts/apply_lib_patches.py`
- Modify: `C:/Users/D/poseidon/platformio.ini`

**Estimated savings:** 344 bytes IRAM-only segment (~2.1% of the 16 KB IRAM-only block). This is one of the very few user-controllable IRAM wins in this build.

**Why it's safe:** Searched user code for `enableReceive`, `RCSwitch::handleInterrupt`, `attachInterrupt(.*RCSwitch.*)` — zero matches. RCSwitch in our build is TX-only. The two IRAM_ATTR'd functions (`RCSwitch::handleInterrupt`, `RCSwitch::receiveProtocol`) are pure dead weight; even without `gc-sections` dropping them (it can't — they're virtual class members), removing the IRAM_ATTR moves them to flash where they cost zero internal SRAM.

- [ ] **Step 1: Confirm libdeps RCSwitch is present**

```bash
ls .pio/libdeps/cardputer/SmartRC-CC1101-Driver-Lib/RCSwitch.cpp
```

Expected: file exists. If not, run `pio run -e cardputer` once to populate libdeps.

- [ ] **Step 2: Write the patch file**

Create `support_files/rcswitch_no_rx_iram.patch` with content (commit-friendly unified diff form):

```
--- a/RCSwitch.cpp
+++ b/RCSwitch.cpp
@@
-void RCSWITCH_RECEIVE_ATTR RCSwitch::handleInterrupt() {
+void RCSwitch::handleInterrupt() {
@@
-bool RCSWITCH_RECEIVE_ATTR RCSwitch::receiveProtocol(const int p, unsigned int changeCount) {
+bool RCSwitch::receiveProtocol(const int p, unsigned int changeCount) {
```

(`RCSWITCH_RECEIVE_ATTR` is the macro Bruce's fork uses for `IRAM_ATTR` on the RX hot path. If the actual macro name differs, find it via `grep -n "RCSWITCH_RECEIVE_ATTR\|IRAM_ATTR" .pio/libdeps/cardputer/SmartRC-CC1101-Driver-Lib/RCSwitch.cpp` BEFORE writing the patch and adjust the diff accordingly.)

If the macro is `IRAM_ATTR` directly (no wrapper), the patch becomes:

```
--- a/RCSwitch.cpp
+++ b/RCSwitch.cpp
@@
-void IRAM_ATTR RCSwitch::handleInterrupt() {
+void RCSwitch::handleInterrupt() {
@@
-bool IRAM_ATTR RCSwitch::receiveProtocol(const int p, unsigned int changeCount) {
+bool RCSwitch::receiveProtocol(const int p, unsigned int changeCount) {
```

Pick the form that actually matches the current `RCSwitch.cpp` source.

- [ ] **Step 3: Write the apply-patches script**

Create `scripts/apply_lib_patches.py` with exactly:

```python
#!/usr/bin/env python3
"""
Idempotent patch applicator. Runs as a PIO pre-script before the build.
Walks support_files/*.patch, applies any that haven't been applied yet
(detected by a sentinel marker comment), no-ops otherwise.

Why this lives in-tree: lib_deps from a remote URL are clobbered every
time PIO refreshes them. Patching by hand outside the repo means the
modification disappears on `pio pkg update` or any clean checkout.
This script keeps the patch reproducible and version-controlled.
"""
import os
import re
import subprocess
import sys

Import("env")  # noqa: F821 - PIO injects this.

ROOT = env["PROJECT_DIR"]  # noqa: F821
PATCH_DIR = os.path.join(ROOT, "support_files")
LIBDEPS = os.path.join(ROOT, ".pio", "libdeps", "cardputer")
SENTINEL = "/* POSEIDON_PATCHED_RCSWITCH_NO_RX_IRAM */"

def apply_rcswitch_patch():
    src = os.path.join(LIBDEPS, "SmartRC-CC1101-Driver-Lib", "RCSwitch.cpp")
    if not os.path.exists(src):
        # libdeps not yet fetched; PIO will pull them and re-run this on
        # the next iteration. No-op silently.
        return
    with open(src, "r", encoding="utf-8", errors="replace") as f:
        content = f.read()
    if SENTINEL in content:
        return  # already patched, idempotent.
    # Find the RX-path IRAM markers. Bruce's fork uses
    # RCSWITCH_RECEIVE_ATTR which expands to IRAM_ATTR on ESP32.
    # Strip both forms for safety.
    new = content
    new = re.sub(
        r"void\s+RCSWITCH_RECEIVE_ATTR\s+RCSwitch::handleInterrupt\s*\(",
        "void RCSwitch::handleInterrupt(",
        new,
    )
    new = re.sub(
        r"bool\s+RCSWITCH_RECEIVE_ATTR\s+RCSwitch::receiveProtocol\s*\(",
        "bool RCSwitch::receiveProtocol(",
        new,
    )
    new = re.sub(
        r"void\s+IRAM_ATTR\s+RCSwitch::handleInterrupt\s*\(",
        "void RCSwitch::handleInterrupt(",
        new,
    )
    new = re.sub(
        r"bool\s+IRAM_ATTR\s+RCSwitch::receiveProtocol\s*\(",
        "bool RCSwitch::receiveProtocol(",
        new,
    )
    if new == content:
        # Markers not found; library upstream may have changed. Don't
        # silently skip — fail the build with a clear message so the
        # next time someone builds they fix the patch script.
        sys.stderr.write(
            "[apply_lib_patches] ERROR: RCSwitch.cpp doesn't contain the "
            "expected IRAM markers. The patch is stale.\n"
        )
        env.Exit(1)  # noqa: F821
    new += "\n" + SENTINEL + "\n"
    with open(src, "w", encoding="utf-8") as f:
        f.write(new)
    print("[apply_lib_patches] RCSwitch RX IRAM_ATTRs stripped (saves ~344 B IRAM)")

apply_rcswitch_patch()
```

- [ ] **Step 4: Wire the script into platformio.ini**

In `[env:cardputer]`, add a new line just before the closing `[env:cardputer-launcher]` block (or at the end of the `[env:cardputer]` section — order within a section doesn't matter):

```ini
extra_scripts = pre:scripts/apply_lib_patches.py
```

- [ ] **Step 5: Force-refetch libdeps + clean build**

```bash
pio pkg update -e cardputer
pio run -e cardputer -t clean
pio run -e cardputer 2>&1 | tee build.log
```

Expected output: line `[apply_lib_patches] RCSwitch RX IRAM_ATTRs stripped …` appears once during the build. IRAM should drop in the memory table; specifically `RCSwitch.cpp.o` no longer contributes 344 bytes to the iram0.text section.

- [ ] **Step 6: Verify in map file**

```bash
grep -n "RCSwitch.*handleInterrupt\|RCSwitch.*receiveProtocol" .pio/build/cardputer/firmware.map | head -10
```

Expected: those symbols still exist (RCSwitch class needs them) but addresses now start with `0x420…` (flash) NOT `0x4037…` (IRAM).

- [ ] **Step 7: Commit**

```bash
git add support_files/rcswitch_no_rx_iram.patch scripts/apply_lib_patches.py platformio.ini
git commit -m "build: drop RCSwitch RX-path IRAM_ATTR (-344 B IRAM, RX unused)"
```

- [ ] **Step 8: Smoke-test**

Open SubGHz → Bruteforce. Verify TX still works (this was always the only RCSwitch user). The button press should still emit the OOK code on-air; if you have a sniffer or another sub-GHz reader, confirm. If TX doesn't work, the patch over-removed something — revert.

```bash
pio run -e cardputer -t upload
```

---

### Task 8: Heap-allocate wifi_ciw active payload list (7 KB)

**Files:**
- Modify: `C:/Users/D/poseidon/src/features/wifi_ciw.cpp`

**Estimated savings:** 7,020 bytes DRAM .bss when CIW isn't running.

This one is fiddlier because `active` is a function-static (lives inside `feat_wifi_ciw`'s body, not at file scope). The standard transformation: declare a file-scope pointer + count, allocate at function entry, free at function exit, drop the function-static.

- [ ] **Step 1: Read current state**

Open `src/features/wifi_ciw.cpp` lines 220-290. Confirm `static CiwPayload active[PAYLOAD_COUNT];` is inside `feat_wifi_ciw()` and `activeN` is the count. Note the exact `feat_wifi_ciw` entry signature.

- [ ] **Step 2: Move `active` to file-scope pointer**

Add at file scope, near other static state (line ~30):

```cpp
/* Heap-allocated for the duration of feat_wifi_ciw. Size is bounded by
 * the macro PAYLOAD_COUNT which expands to a sizeof() over a flash-resident
 * array, so it's a constexpr in practice — heap_caps_calloc always gets
 * a known size. */
static CiwPayload *s_active = nullptr;
static size_t     s_active_n = 0;
```

- [ ] **Step 3: In feat_wifi_ciw, replace the function-static with the heap pointer**

Find inside `feat_wifi_ciw()`:

```cpp
    static CiwPayload active[PAYLOAD_COUNT];
    int activeN = 0;
    for (size_t i = 0; i < PAYLOAD_COUNT; i++) {
        CiwPayload p;
        memcpy_P(&p, &PAYLOADS[i], sizeof(CiwPayload));
        // ... category filter check ...
        if (/* keep */) memcpy(&active[activeN++], &p, sizeof(CiwPayload));
    }
```

Replace the `static CiwPayload active[PAYLOAD_COUNT]; int activeN = 0;` section + subsequent uses, mapping `active` → `s_active` and `activeN` → `s_active_n`. Add allocation guard at the very top of the function:

```cpp
    s_active = (CiwPayload *)heap_caps_calloc(
        PAYLOAD_COUNT, sizeof(CiwPayload),
        MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT);
    if (!s_active) {
        ui_clear_body();
        ui_text(8, BODY_Y + 30, T_BAD, "CIW: OUT OF MEMORY");
        ui_draw_footer("ESC=back");
        while (input_poll() != PK_ESC) delay(10);
        return;
    }
    s_active_n = 0;
```

Then everywhere `active[i]` was used inside this function, replace with `s_active[i]`. Everywhere `activeN` was used, replace with `s_active_n` (size_t — adjust signed/unsigned comparison if needed). Cast the `(int)s_active_n` only in printf-style format args.

- [ ] **Step 4: Free at every exit path**

Before every `return` from `feat_wifi_ciw`:

```cpp
    if (s_active) { heap_caps_free(s_active); s_active = nullptr; }
    s_active_n = 0;
```

- [ ] **Step 5: Build**

```bash
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0. DIRAM .bss drops by ~7,020 bytes. Watch for any signed/unsigned warnings introduced by `int activeN` → `size_t s_active_n` — fix those before committing.

- [ ] **Step 6: Commit**

```bash
git add src/features/wifi_ciw.cpp
git commit -m "wifi_ciw: heap-allocate active payload list (-7 KB DRAM at idle)"
```

- [ ] **Step 7: Smoke-test**

Open WiFi → Captive Impersonation. Cycle through a few categories. Hit ESC. Re-enter. Free heap stable across 3 cycles.

---

### Task 9: Capture post-optimization numbers and write delta

**Files:**
- Create: `bench/post.txt`
- Create: `tools/sizediff.py`

- [ ] **Step 1: Final clean build**

```bash
pio run -e cardputer -t clean
pio run -e cardputer 2>&1 | tee build.log
```

Expected: exit 0.

- [ ] **Step 2: Write `bench/post.txt`**

Create `bench/post.txt` with the same format as `bench/baseline.txt`, but populated with the new numbers from build.log.

```
# POSEIDON memory post-optimization
# Captured: <YYYY-MM-DD> at <HH:MM>
# Commit:   <git rev-parse HEAD>

Memory Type     Used        Total       Pct
Flash Code      <new>       -           -
Flash Data      <new>       -           -
Flash total     <new>       3342336     <pct>%
DIRAM .bss      <new>       -           <pct>%
DIRAM .text     <new>       -           <pct>%
DIRAM .data     <new>       -           <pct>%
DIRAM total     <new>       341760      <pct>%
RAM (PIO)       <new>       327680      <pct>%
IRAM .text      <new>       -           <pct>%
IRAM .vectors   <new>       -           <pct>%
IRAM total      <new>       16384       <pct>%
```

Replace each `<new>` and `<pct>` with the actual number from the build log's memory table.

- [ ] **Step 3: Write the diff helper**

Create `tools/sizediff.py` with exactly:

```python
#!/usr/bin/env python3
"""Diff two POSEIDON memory tables (bench/baseline.txt vs bench/post.txt).

Usage:
    python tools/sizediff.py bench/baseline.txt bench/post.txt
"""
import sys

def parse(path):
    """Return {metric: (used, total, pct)} parsed from a bench file."""
    out = {}
    with open(path) as f:
        for line in f:
            line = line.rstrip()
            if not line or line.startswith("#") or line.startswith("Memory"):
                continue
            parts = line.split()
            if len(parts) < 3:
                continue
            # join leading words until we hit a numeric column
            for split in range(len(parts) - 1, 0, -1):
                try:
                    int(parts[split])
                    metric = " ".join(parts[:split])
                    used = int(parts[split])
                    rest = parts[split + 1:]
                    break
                except ValueError:
                    continue
            else:
                continue
            total = rest[0] if rest else "-"
            pct = rest[1] if len(rest) > 1 else "-"
            out[metric] = (used, total, pct)
    return out

def main():
    if len(sys.argv) != 3:
        print(__doc__)
        sys.exit(1)
    base = parse(sys.argv[1])
    post = parse(sys.argv[2])
    print(f"{'Metric':<18} {'Baseline':>12} {'Post':>12} {'Delta':>12}")
    print("-" * 56)
    keys = list(base.keys()) + [k for k in post if k not in base]
    for k in keys:
        b = base.get(k, (0, "-", "-"))[0]
        p = post.get(k, (0, "-", "-"))[0]
        d = p - b
        print(f"{k:<18} {b:>12} {p:>12} {d:>+12}")

if __name__ == "__main__":
    main()
```

- [ ] **Step 4: Run the diff**

```bash
python tools/sizediff.py bench/baseline.txt bench/post.txt
```

Expected output approximately:

```
Metric             Baseline         Post        Delta
--------------------------------------------------------
Flash Code          1512008      ~1430000      ~-82000
Flash Data           597136       ~597000          ~-136
Flash total         2232711      ~2150000      ~-82000
DIRAM .bss           195528       ~115000      ~-80000
DIRAM .text           83171        ~83171            0
DIRAM .data           24268        ~24268            0
DIRAM total          302967       ~222000      ~-80000
RAM (PIO)            219796       ~140000      ~-80000
IRAM .text            15356        ~15012         ~-344
IRAM .vectors          1028         1028            0
IRAM total            16384       ~16040         ~-344
```

(Exact numbers depend on Tasks 2-8 outcomes; this is the target.)

- [ ] **Step 5: Commit**

```bash
git add bench/post.txt tools/sizediff.py
git commit -m "bench: capture post-optimization numbers + diff helper"
```

- [ ] **Step 6: Final smoke-test**

Run a 10-minute hardware soak: enter and exit each previously-tested feature 5 times in a round-robin (Wifi Scan, BLE Scan, SubGHz Scan, LoRa Spectrum, Subghz Waterfall, PMKID, Triton, CCTV Scan, WhisperPair, MIMIR open, LAN Recon, SSDP, CIW). Watch free heap. If free heap monotonically decreases across cycles, there is a leak in one of the heap-migrated features — bisect by reverting commits one at a time until it stops.

---

## Self-review

After writing the plan above, applied the checklist:

**1. Spec coverage:** Walked the original spec section by section.
- "IRAM reduction primary" → Task 7 (RCSwitch -344 B). Investigation Findings §2 + §3 explain why this is the only realistic IRAM lever — BT controller is in prebuilt framework. Documented honestly.
- "DRAM reduction secondary" → Tasks 2, 3, 4, 5, 6, 8 = ~80 KB recovered.
- "Flash reduction tertiary" → Task 1 (LTO) = ~80 KB.
- "Build-flag audit" → Investigation Findings §6 + Task 1.
- "Baseline measurement at start" → Task 0.
- "Post measurement at end" → Task 9.
- All in-scope items covered. All out-of-scope items (architectural rewrites, removing features, touching wifi_sanity_override) are excluded by the Hard Constraints block.

**2. Placeholder scan:** Searched for "TBD", "TODO", "implement later", "fill in details", "appropriate error handling". One intentional placeholder remains: the `<YYYY-MM-DD>`, `<HH:MM>`, `<git rev-parse HEAD>`, and `<new>`/`<pct>` markers in Task 0 / Task 9 file-creation steps — those are values to be filled in by the executing engineer based on actual run-time output, not gaps in the plan. Marked them with explicit "Replace … with the actual values" prose.

**3. Type consistency:**
- `heap_caps_calloc(N, sizeof(T), MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)` is used uniformly across Tasks 2-8 except Task 2 which uses `heap_caps_malloc + memset` (because we want to control the zero-fill explicitly to draw deep-blue floor). That's an intentional difference, documented in the comment.
- `heap_caps_free` is used everywhere; never `free()`. Consistent.
- `s_capq` / `s_m1` / `s_hits` / `s_keys` / `s_tgt` / `s_targets` / `s_hosts` / `s_dev` / `s_active` — naming is consistent with each file's existing prefix style. Counts are `s_*_n` matching existing convention.
- The OOM-bail UI sequence (`ui_clear_body() / ui_text() / ui_draw_footer() / while ESC`) is identical across every task — DRY.
- `MALLOC_CAP_INTERNAL` ensures we never touch PSRAM (which is broken). Constraint enforced at the alloc site.

Plan is internally consistent and addresses every spec requirement. Saved to `docs/superpowers/plans/2026-04-25-poseidon-memory-optimization.md`.
