# POSEIDON ambient motion + theme mood-tinting Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Phase A of the v0.6.0 visual immersion overhaul — add a procedural per-theme ambient motion module that paints behind every menu, plus a small Triton-mood-aware modulation of the theme accent so HUNT / STEALTH / SURGICAL / STORM visibly tint the UI without per-feature code changes. Sprites, screensavers, and narrative toasts are explicitly OUT of scope and deferred to later plans.

**Architecture:** Three additive surfaces. (1) `src/ui_ambient.{h,cpp}` exposes one public `ui_ambient_tick(x, y, w, h)` that dispatches to a per-theme procedural painter; tinted themes (POSEIDON, PHANTOM, MATRIX, AMBER, TRON) paint, paper themes (E-INK, HI-CONTRAST) no-op. (2) `theme.{h,cpp}` gains `theme_effective_accent()` / `theme_effective_accent2()` that blend the base accent toward a per-mood tint; the existing `T_ACCENT` / `T_ACCENT2` macros are redefined to call them so every existing call-site auto-inherits mood tint with zero per-feature work. The mood is read from a new `triton_current_mode_int()` accessor exported from `triton.cpp`, which returns 0 (neutral) when the Triton feature isn't active. (3) `menu.cpp` gets a single `ui_ambient_tick()` call inside `draw_menu()`, gated by an NVS-backed `pamb/enabled` flag so the user can disable it from a new System submenu entry "Ambient Preview" (which doubles as a debug viewer).

**Tech Stack:** C++17, Arduino-ESP32 / pioarduino 55.03.38, M5Cardputer 1.1.1 (LovyanGFX-backed `M5Cardputer.Display`), Preferences (NVS) for the on/off flag, ESP-IDF `esp_random()`. Target ESP32-S3 @ 240 MHz, 240x135 ST7789v2 LCD. `pio run -e cardputer` builds; `pio run -e cardputer -t upload` flashes; `pio device monitor -b 115200` reads serial.

---

## Hard constraints (locked in by hardware + repo state)

These constrain every task in this plan. Re-read before writing any code.

- **No PSRAM.** PSRAM is broken on this unit (see `platformio.ini:27-30`). All ambient state must be SRAM-resident. Do not allocate via `ps_malloc`. Do not pre-allocate large buffers.
- **IRAM is at 100%.** Do NOT mark any new function `IRAM_ATTR`. Ambient code lives in flash, called from regular task context. The dispatcher itself is small enough that the hit is invisible, but adding `IRAM_ATTR` will fail to link.
- **No malloc in render loop.** Per-frame allocations would fragment heap. Every piece of mutable ambient state is either function-local on the stack, file-scope `static` arrays sized at compile time, or `const`/`PROGMEM`. No `new` calls anywhere in this plan.
- **Frame budget: ~30 FPS = 33 ms; ambient pass <= 2 ms.** Pixel cost ceilings per effect are listed in the per-task code comments — total pixels touched per frame stays well under the 240*101 = 24240-pixel body region. POSEIDON 8 motes * 3 px + 1 hline = 264 px/frame. AMBER 2 hlines + 4 jitter = 484 px/frame. TRON ~13 grid lines + 9 px packet = ~510 px/frame. PHANTOM 4 glyphs * (12*16) = 768 px/frame worst case. MATRIX defers to existing `ui_matrix_rain` which is already validated. None come close to 2 ms on a 240 MHz S3 (~10M pixels/sec writeRect throughput).
- **Theme leak guard.** All colors come from `theme()` macros / accessors. The only literal RGB565 values allowed in this plan are the four `MOOD_TINT_*` constants in `theme.cpp` — those are the per-mood blend targets and are intentionally not theme-derived (they ARE the mood). No `0xF81F` / `0x07FF` etc. anywhere else.
- **Don't break existing menus.** The hook in `draw_menu()` checks `ambient_enabled()` (NVS-backed, default true) before calling `ui_ambient_tick()`. If the user reports visual issues, they ESC to System -> Ambient Preview -> off and the call becomes a no-op.
- **Local commits only.** Per `feedback_poseidon_github_gate.md`: do not push, do not tag, do not create a release. Commits stay on the local branch until the user explicitly says "ship it."

---

## File Structure

| File | Status | Responsibility |
|---|---|---|
| `src/ui_ambient.h` | NEW | Public API: declares `ui_ambient_tick()` and `ui_ambient_enabled() / _set()` toggle accessors. |
| `src/ui_ambient.cpp` | NEW | Per-theme procedural painters (POSEIDON / PHANTOM / MATRIX / AMBER / TRON), dispatcher, mood-speed-scale helper, NVS-backed enable flag. |
| `src/theme.h` | MODIFY | Add `theme_effective_accent()` / `theme_effective_accent2()` declarations. Redefine `T_ACCENT` / `T_ACCENT2` macros to call them. |
| `src/theme.cpp` | MODIFY | Implement the two effective-accent functions plus a static `blend_565()` helper and `MOOD_TINT_*` constants. |
| `src/features/triton.cpp` | MODIFY | Add `s_triton_active` flag plus `extern "C" int triton_current_mode_int(void)` accessor that returns 0 when Triton isn't active and 1..4 otherwise. |
| `src/menu.cpp` | MODIFY | One include for `ui_ambient.h`, one guarded call to `ui_ambient_tick(...)` inside `draw_menu()` BEFORE the row iteration. Add a new System menu entry "Ambient Preview". |
| `src/features/system_tools.cpp` | MODIFY | Append `feat_ambient_preview()` — opens a clean body, calls `ui_ambient_tick()` in a 33 ms loop, exits on any key. Also opens an inner toggle for the on/off NVS flag. |

No files are deleted. No existing function bodies are rewritten beyond the ones marked above.

---

## Task breakdown

The hardware-in-the-loop nature of this work means there are no unit tests. Each task ends with a verifiable build step (`pio run -e cardputer` must exit 0) and, where the task introduces visible behavior, a flash-and-look step with the exact expected outcome. Treat the visual checks as the equivalent of test assertions: if the screen does not match, the task is not done.

### Task 1: Create `src/ui_ambient.h` with public API

**Files:**
- Create: `src/ui_ambient.h`

- [ ] **Step 1: Write the header**

Create file `src/ui_ambient.h` with exactly:

```cpp
/*
 * ui_ambient — theme-aware procedural ambient motion behind static screens.
 *
 * Call ui_ambient_tick() at the start of any refresh loop (or once per
 * menu redraw) BEFORE drawing content on top. Picks an animation per the
 * active theme: POSEIDON deep-sea motes + wave band, AMBER CRT scanline,
 * TRON grid + packet hop, MATRIX dimmed rain, PHANTOM cyberpunk glyphs.
 * No-op for E-INK and HI-CONTRAST (paper aesthetic + accessibility).
 *
 * If Triton is the active feature and in a non-neutral mood, ambient
 * speed is modulated: HUNT 2.0x, STEALTH 0.5x, SURGICAL 1.0x, STORM 3.5x.
 *
 * Persisted enable flag in NVS namespace "pamb", key "enabled" (default
 * true). ui_ambient_tick() respects the flag — when disabled it returns
 * immediately so the menu hook is a true no-op and any visual regression
 * can be turned off without a re-flash.
 */
#pragma once

#include <Arduino.h>

void ui_ambient_tick(int x, int y, int w, int h);

bool ui_ambient_enabled(void);
void ui_ambient_enabled_set(bool on);
```

- [ ] **Step 2: Build to verify the header parses**

Run: `pio run -e cardputer 2>&1 | tail -8`
Expected: build exits 0. No source includes the header yet, so this only verifies the header itself parses cleanly.

- [ ] **Step 3: Commit**

```bash
git add src/ui_ambient.h
git commit -m "feat(ui_ambient): public API header"
```

---

### Task 2: Add `triton_current_mode_int()` accessor

**Files:**
- Modify: `src/features/triton.cpp` (add flag + accessor)

The ambient module needs to read the current Triton mood. Today, `s_mode` is `static volatile triton_mode_t` (file-scope, line 60), and there is no "Triton feature is currently running" flag. We add both: a flag flipped on entry/exit of `feat_triton()`, and a C-callable accessor that combines them.

- [ ] **Step 1: Add the active flag and the accessor near the existing `s_mode` declaration**

In `src/features/triton.cpp`, immediately after the `static volatile triton_mode_t s_mode = TM_HUNT;` line (currently line 60), insert:

```cpp
/* Set true while feat_triton() is on-screen, false otherwise. Read by
 * ui_ambient and theme.cpp's effective-accent path so the mood tint
 * only applies WHILE the user is in Triton (HUNT/STEALTH/SURGICAL/STORM
 * are Triton-modes, not global modes). */
static volatile bool s_triton_active = false;

/* C-callable mode accessor for ui_ambient and theme.cpp. Returns 0 when
 * Triton feature isn't active OR mode is unrecognised. 1=HUNT, 2=STEALTH,
 * 3=SURGICAL, 4=STORM. */
extern "C" int triton_current_mode_int(void)
{
    if (!s_triton_active) return 0;
    switch (s_mode) {
    case TM_HUNT:     return 1;
    case TM_STEALTH:  return 2;
    case TM_SURGICAL: return 3;
    case TM_STORM:    return 4;
    }
    return 0;
}
```

- [ ] **Step 2: Set the flag at `feat_triton()` entry**

Find `void feat_triton(void)` (line 873). Immediately inside the function body, BEFORE `radio_switch(RADIO_WIFI);`, insert:

```cpp
    s_triton_active = true;
```

- [ ] **Step 3: Clear the flag on every return from `feat_triton()`**

`feat_triton()` has multiple early-return paths (`if (!pick_mode()) return;`, `if (s_mode == TM_SURGICAL && !pick_surgical_target()) return;`, `if (!sd_mount()) { ... return; }`, etc.). Wrap the body so the flag is always cleared.

The cleanest approach: turn the existing body of `feat_triton()` into a static helper `triton_run_inner()` and have `feat_triton()` set the flag, call the helper, clear the flag.

```cpp
static void triton_run_inner(void);

void feat_triton(void)
{
    s_triton_active = true;
    triton_run_inner();
    s_triton_active = false;
}

static void triton_run_inner(void)
{
    /* paste the prior body of feat_triton() here, unchanged */
    radio_switch(RADIO_WIFI);
    WiFi.mode(WIFI_STA);
    /* ... (everything that was originally in feat_triton) ... */
}
```

This guarantees every `return` inside the inner function still clears the flag.

- [ ] **Step 4: Build**

Run: `pio run -e cardputer 2>&1 | tail -10`
Expected: build exits 0. Any error will be a missed `static` keyword on the renamed inner function, or a missed forward decl.

- [ ] **Step 5: Flash and verify nothing regressed**

Run: `pio run -e cardputer -t upload`
Then on-device: navigate to Triton, pick HUNT mode, let it run for ~5 seconds, ESC out. Expected: identical behavior to before (no visible change yet — the accessor is exposed but no caller uses it).

- [ ] **Step 6: Commit**

```bash
git add src/features/triton.cpp
git commit -m "feat(triton): expose triton_current_mode_int + s_triton_active flag"
```

---

### Task 3: Skeleton `src/ui_ambient.cpp` with NVS toggle and dispatcher

**Files:**
- Create: `src/ui_ambient.cpp`

This task lays down the dispatcher and per-theme stubs. No theme paints anything yet — that's tasks 4 through 8. After this task, calling `ui_ambient_tick()` is a no-op for every theme but the function is wired through and the NVS flag works.

- [ ] **Step 1: Write the file**

Create `src/ui_ambient.cpp`:

```cpp
/*
 * ui_ambient.cpp — see ui_ambient.h.
 *
 * One static painter per theme. Dispatched by theme_current_id().
 * Mood-aware speed scale read from triton_current_mode_int().
 *
 * State invariants:
 *   - All per-frame state derives from millis() and esp_random(); no
 *     module-scope mutable state except the cached NVS flag.
 *   - Functions write directly to M5Cardputer.Display (no sprite buffer,
 *     no PSRAM — this unit's PSRAM is broken).
 *   - Functions are NOT marked IRAM_ATTR — IRAM is full (see platformio.ini).
 */
#include "ui_ambient.h"
#include "theme.h"
#include "app.h"
#include "ui.h"
#include <M5Cardputer.h>
#include <esp_random.h>
#include <Preferences.h>

extern "C" int triton_current_mode_int(void);
extern void ui_matrix_rain(int x, int y, int w, int h, uint16_t color);

/* ---- NVS-backed enable flag ---- */
static bool s_amb_loaded  = false;
static bool s_amb_enabled = true;

bool ui_ambient_enabled(void)
{
    if (!s_amb_loaded) {
        Preferences p;
        if (p.begin("pamb", true)) {
            s_amb_enabled = p.getBool("enabled", true);
            p.end();
        }
        s_amb_loaded = true;
    }
    return s_amb_enabled;
}

void ui_ambient_enabled_set(bool on)
{
    s_amb_enabled = on;
    s_amb_loaded  = true;
    Preferences p;
    if (p.begin("pamb", false)) {
        p.putBool("enabled", on);
        p.end();
    }
}

/* ---- mood-modulated speed scale ----
 * neutral = 1.0, HUNT = 2.0, STEALTH = 0.5, SURGICAL = 1.0, STORM = 3.5.
 * Per the spec at docs/superpowers/specs/2026-04-24-poseidon-visual-immersion-design.md
 */
static float ambient_speed_scale(void)
{
    switch (triton_current_mode_int()) {
    case 1: return 2.0f;
    case 2: return 0.5f;
    case 3: return 1.0f;
    case 4: return 3.5f;
    default: return 1.0f;
    }
}

/* Per-theme painter forward decls — bodies below in the order they are
 * implemented in the plan: POSEIDON (Task 4), AMBER (Task 5), TRON
 * (Task 6), MATRIX (Task 7), PHANTOM (Task 8). */
static void amb_poseidon(int x, int y, int w, int h);
static void amb_phantom (int x, int y, int w, int h);
static void amb_matrix  (int x, int y, int w, int h);
static void amb_amber   (int x, int y, int w, int h);
static void amb_tron    (int x, int y, int w, int h);

void ui_ambient_tick(int x, int y, int w, int h)
{
    if (!ui_ambient_enabled()) return;
    if (w <= 0 || h <= 0)      return;
    switch (theme_current_id()) {
    case THEME_POSEIDON:   amb_poseidon(x, y, w, h); break;
    case THEME_PHANTOM:    amb_phantom (x, y, w, h); break;
    case THEME_MATRIX:     amb_matrix  (x, y, w, h); break;
    case THEME_AMBER:      amb_amber   (x, y, w, h); break;
    case THEME_TRON:       amb_tron    (x, y, w, h); break;
    case THEME_EINK:       /* paper aesthetic — no ambient */          break;
    case THEME_HICONTRAST: /* accessibility — no ambient */             break;
    default:               break;
    }
}

/* ---- POSEIDON: deep-sea motes + slow wave-band ripple (Task 4) ---- */
static void amb_poseidon(int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
}

/* ---- AMBER: CRT scanline drift + phosphor jitter (Task 5) ---- */
static void amb_amber(int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
}

/* ---- TRON: scrolling grid + cyan/magenta packet hop (Task 6) ---- */
static void amb_tron(int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
}

/* ---- MATRIX: dimmed reuse of existing ui_matrix_rain (Task 7) ---- */
static void amb_matrix(int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
}

/* ---- PHANTOM: cyberpunk glyph flashes (Task 8) ---- */
static void amb_phantom(int x, int y, int w, int h)
{
    (void)x; (void)y; (void)w; (void)h;
}
```

- [ ] **Step 2: Build**

Run: `pio run -e cardputer 2>&1 | tail -10`
Expected: build exits 0. The file compiles, all stubs are reachable. PlatformIO will pick up the new `src/ui_ambient.cpp` automatically (it globs `src/**/*.cpp`).

- [ ] **Step 3: Flash and verify nothing changed**

Run: `pio run -e cardputer -t upload`
On-device: navigate menus normally. Expected: no visible change (no caller invokes `ui_ambient_tick` yet).

- [ ] **Step 4: Commit**

```bash
git add src/ui_ambient.cpp
git commit -m "feat(ui_ambient): dispatcher + NVS flag + per-theme stubs"
```

---

### Task 4: Implement POSEIDON painter (motes + wave-band)

**Files:**
- Modify: `src/ui_ambient.cpp` (replace `amb_poseidon` body)

Pixel budget: 8 motes * 3 px (centre + halo above/below) + 1 horizontal line of width `w` = 24 + 240 = 264 px/frame. Order of magnitude under the 2 ms target.

- [ ] **Step 1: Replace the stub**

In `src/ui_ambient.cpp`, replace the entire `amb_poseidon` function (currently a stub) with:

```cpp
/* ---- POSEIDON: deep-sea motes + slow wave-band ripple ----
 * 8 cyan/magenta motes drift bottom -> top, each on its own period and
 * x-position. One horizontal "wave band" line oscillates vertically via a
 * triangle wave. Phase is keyed off millis() and a per-mote seed so motes
 * don't sync. */
static void amb_poseidon(int x, int y, int w, int h)
{
    auto &d = M5Cardputer.Display;
    uint32_t now    = millis();
    float    scale  = ambient_speed_scale();

    static const struct mote_t {
        uint8_t  col_pct;    /* x position as percent of w */
        uint16_t period_ms;  /* full cycle duration at scale 1.0 */
        uint32_t seed;       /* phase offset */
        bool     alt;        /* true = T_ACCENT2, false = T_ACCENT */
    } MOTES[8] = {
        {  8, 5000, 0x1111u, false },
        { 22, 7000, 0x2222u, true  },
        { 36, 6000, 0x3333u, false },
        { 50, 8000, 0x4444u, true  },
        { 62, 5500, 0x5555u, false },
        { 78, 7000, 0x6666u, false },
        { 88, 6500, 0x7777u, true  },
        { 14, 9000, 0x8888u, false },
    };

    for (int i = 0; i < 8; ++i) {
        uint32_t period = (uint32_t)((float)MOTES[i].period_ms / scale);
        if (period < 500) period = 500;
        uint32_t phase  = (now + MOTES[i].seed) % period;
        /* y goes from h-2 (bottom) down to -2 (off top) over the period */
        int my = (h - 2) - (int)((int64_t)phase * (h + 2) / (int64_t)period);
        int mx = (MOTES[i].col_pct * w) / 100;
        if (my < 0 || my >= h) continue;
        uint16_t color = MOTES[i].alt ? T_ACCENT2 : T_ACCENT;
        d.drawPixel(x + mx, y + my, color);
        if (my > 0)     d.drawPixel(x + mx, y + my - 1, color);
        if (my < h - 1) d.drawPixel(x + mx, y + my + 1, color);
    }

    /* Wave-band: triangle wave between h/10 and 8*h/10 with period 8000 ms. */
    uint32_t wp = (uint32_t)(8000.0f / scale);
    if (wp < 1000) wp = 1000;
    uint32_t wphase = now % wp;
    int range = (h * 7) / 10;
    int saw   = (int)((int64_t)wphase * 2 * range / (int64_t)wp);
    int wy    = (h / 10) + abs(saw - range);
    if (wy >= 0 && wy < h) {
        d.drawFastHLine(x, y + wy, w, T_ACCENT);
    }
}
```

- [ ] **Step 2: Build**

Run: `pio run -e cardputer 2>&1 | tail -8`
Expected: build exits 0.

- [ ] **Step 3: Defer visual verification to Task 9**

Visual verification needs `feat_ambient_preview` (Task 9) to be reachable, OR the menu hook (Task 10). For now, just confirm the build is green. The painter is wired and ready.

- [ ] **Step 4: Commit**

```bash
git add src/ui_ambient.cpp
git commit -m "feat(ui_ambient): POSEIDON deep-sea motes + wave-band painter"
```

---

### Task 5: Implement AMBER painter (CRT scanline drift + phosphor jitter)

**Files:**
- Modify: `src/ui_ambient.cpp` (replace `amb_amber` body)

Pixel budget: 2 horizontal lines (`2 * w = 480`) + 4 random pixels = 484 px/frame.

- [ ] **Step 1: Replace the stub**

In `src/ui_ambient.cpp`:

```cpp
/* ---- AMBER: CRT scanline drift + phosphor jitter ----
 * One bright scanline drifts top -> bottom every ~3 s. A second dim line
 * trails one row below. Four pixels per frame are speckled at theme.dim
 * for the phosphor-noise look. */
static void amb_amber(int x, int y, int w, int h)
{
    auto &d = M5Cardputer.Display;
    uint32_t now   = millis();
    float    scale = ambient_speed_scale();

    uint32_t period = (uint32_t)(3000.0f / scale);
    if (period < 600) period = 600;
    uint32_t phase = now % period;
    /* sy walks from -4 (off-top) through h+4 (off-bottom) so the line
     * enters and exits cleanly each pass. */
    int sy = (int)((int64_t)phase * (h + 8) / (int64_t)period) - 4;
    if (sy >= 0 && sy < h) {
        d.drawFastHLine(x, y + sy, w, T_ACCENT);
    }
    if (sy + 1 >= 0 && sy + 1 < h) {
        d.drawFastHLine(x, y + sy + 1, w, T_DIM);
    }

    for (int i = 0; i < 4; ++i) {
        int px = x + (int)(esp_random() % (uint32_t)w);
        int py = y + (int)(esp_random() % (uint32_t)h);
        d.drawPixel(px, py, T_DIM);
    }
}
```

- [ ] **Step 2: Build**

Run: `pio run -e cardputer 2>&1 | tail -8`
Expected: exits 0.

- [ ] **Step 3: Commit**

```bash
git add src/ui_ambient.cpp
git commit -m "feat(ui_ambient): AMBER CRT scanline + phosphor jitter painter"
```

---

### Task 6: Implement TRON painter (grid + packet hop)

**Files:**
- Modify: `src/ui_ambient.cpp` (replace `amb_tron` body)

Pixel budget: ~8 vertical grid lines of height `h` + ~4 horizontal grid lines of width `w` + a 3x3 packet (9 px) + 4 halo pixels = ~8*101 + 4*240 + 13 ~= 1781 px/frame. Lines are `drawFastVLine` / `drawFastHLine` so the bottleneck is the SPI bus, well under 2 ms.

- [ ] **Step 1: Replace the stub**

In `src/ui_ambient.cpp`:

```cpp
/* ---- TRON: scrolling grid + cyan/magenta packet hop ----
 * 30 px grid scrolling diagonally at ~6 s/cell. A magenta 3x3 packet
 * traces an L-path through the body region every 4 s. */
static void amb_tron(int x, int y, int w, int h)
{
    auto &d = M5Cardputer.Display;
    uint32_t now   = millis();
    float    scale = ambient_speed_scale();

    uint32_t scroll_period = (uint32_t)(6000.0f / scale);
    if (scroll_period < 1500) scroll_period = 1500;
    int scroll = (int)((now % scroll_period) * 30 / scroll_period);

    for (int gx = -scroll; gx < w; gx += 30) {
        if (gx >= 0 && gx < w) d.drawFastVLine(x + gx, y, h, T_DIM);
    }
    for (int gy = -scroll; gy < h; gy += 30) {
        if (gy >= 0 && gy < h) d.drawFastHLine(x, y + gy, w, T_DIM);
    }

    uint32_t pkt_period = (uint32_t)(4000.0f / scale);
    if (pkt_period < 800) pkt_period = 800;
    uint32_t pphase = now % pkt_period;
    /* L-path: 4 segments, each 25% of period.
     *   seg 0: across (10%->30% of w) at y=20% of h
     *   seg 1: down   (20%->75% of h) at x=30% of w
     *   seg 2: across (30%->75% of w) at y=75% of h
     *   seg 3: down   (75%->95% of h) at x=75% of w
     */
    uint32_t quarter   = pkt_period / 4;
    int      seg       = (int)(pphase / quarter);
    if (seg > 3) seg = 3;
    int      seg_phase = (int)(pphase - (uint32_t)seg * quarter);
    int      seg_pct   = (int)((int64_t)seg_phase * 100 / (int64_t)quarter);

    int px, py;
    switch (seg) {
    case 0:  px = (w * (10 + (30 - 10) * seg_pct / 100)) / 100;
             py = (h * 20) / 100; break;
    case 1:  px = (w * 30) / 100;
             py = (h * (20 + (75 - 20) * seg_pct / 100)) / 100; break;
    case 2:  px = (w * (30 + (75 - 30) * seg_pct / 100)) / 100;
             py = (h * 75) / 100; break;
    default: px = (w * 75) / 100;
             py = (h * (75 + (95 - 75) * seg_pct / 100)) / 100; break;
    }

    d.fillRect(x + px - 1, y + py - 1, 3, 3, T_ACCENT2);
    /* 4-pixel cross halo. */
    d.drawPixel(x + px - 2, y + py,     T_ACCENT2);
    d.drawPixel(x + px + 2, y + py,     T_ACCENT2);
    d.drawPixel(x + px,     y + py - 2, T_ACCENT2);
    d.drawPixel(x + px,     y + py + 2, T_ACCENT2);
}
```

- [ ] **Step 2: Build**

Run: `pio run -e cardputer 2>&1 | tail -8`
Expected: exits 0.

- [ ] **Step 3: Commit**

```bash
git add src/ui_ambient.cpp
git commit -m "feat(ui_ambient): TRON grid scroll + packet hop painter"
```

---

### Task 7: Implement MATRIX painter (dimmed rain wrapper)

**Files:**
- Modify: `src/ui_ambient.cpp` (replace `amb_matrix` body)

This is the lightest task — re-uses the existing `ui_matrix_rain` from `src/ui.cpp` at `T_DIM` instead of `T_ACCENT` for ambient subtlety. Pixel budget bounded by the existing `MATRIX_COLS = 20` columns * 5 trail chars per col * (6*8) = 4800 px max, gated by an 80 ms internal advance-tick — actual per-frame writeRect cost is far smaller because most cells are static.

- [ ] **Step 1: Replace the stub**

In `src/ui_ambient.cpp`:

```cpp
/* ---- MATRIX: dimmed reuse of existing rain ----
 * The screensaver phase will run rain at full brightness over the whole
 * screen; for ambient we want it visibly subtler, so we pass T_DIM
 * instead of T_ACCENT. ui_matrix_rain handles its own per-column state
 * and 80 ms advance throttling internally. */
static void amb_matrix(int x, int y, int w, int h)
{
    ui_matrix_rain(x, y, w, h, T_DIM);
}
```

- [ ] **Step 2: Build**

Run: `pio run -e cardputer 2>&1 | tail -8`
Expected: exits 0.

- [ ] **Step 3: Commit**

```bash
git add src/ui_ambient.cpp
git commit -m "feat(ui_ambient): MATRIX dimmed-rain wrapper"
```

---

### Task 8: Implement PHANTOM painter (cyberpunk glyph flashes)

**Files:**
- Modify: `src/ui_ambient.cpp` (replace `amb_phantom` body)

Spec is explicit: cyberpunk glyphs, NOT fantasy occult. Use `@ # $ % ^ & * < > = ? / \` from a small printable-ASCII pool. Pixel budget worst case 4 visible glyphs * (12*16) = 768 px/frame; in practice usually 2 or fewer above the alpha threshold.

- [ ] **Step 1: Replace the stub**

In `src/ui_ambient.cpp`:

```cpp
/* ---- PHANTOM: cyberpunk glyph flashes ----
 * 4 glyph slots positioned in body region. Each fades in/out on its own
 * period via a triangle envelope. Below an alpha threshold they're not
 * drawn at all. Glyphs picked deterministically from seed + cycle index.
 * Cyberpunk ASCII pool, no occult symbology. */
static void amb_phantom(int x, int y, int w, int h)
{
    auto &d = M5Cardputer.Display;
    uint32_t now   = millis();
    float    scale = ambient_speed_scale();

    static const char     GLYPHS[]    = "@#$%^&*<>=?/\\";
    static const uint32_t GLYPH_COUNT = sizeof(GLYPHS) - 1;

    static const struct slot_t {
        uint8_t  cx_pct;
        uint8_t  cy_pct;
        uint16_t period_ms;
        uint32_t seed;
        bool     alt;
    } SLOTS[4] = {
        { 18, 25, 5000, 0xAAAAAAAAu, false },
        { 60, 55, 6000, 0xBBBBBBBBu, true  },
        { 35, 70, 5500, 0xCCCCCCCCu, false },
        { 75, 20, 6500, 0xDDDDDDDDu, true  },
    };

    for (int i = 0; i < 4; ++i) {
        uint32_t period = (uint32_t)((float)SLOTS[i].period_ms / scale);
        if (period < 1500) period = 1500;
        uint32_t phase  = (now + SLOTS[i].seed) % period;
        /* Triangle envelope 0..200..0 over the period. */
        int alpha = (int)((int64_t)phase * 200 / (int64_t)period);
        if (alpha > 100) alpha = 200 - alpha;
        if (alpha < 30) continue;

        uint32_t cycle = (now + SLOTS[i].seed) / period;
        char     glyph = GLYPHS[(SLOTS[i].seed ^ cycle) % GLYPH_COUNT];
        uint16_t color = SLOTS[i].alt ? T_ACCENT2 : T_ACCENT;
        int gx = x + (SLOTS[i].cx_pct * w) / 100;
        int gy = y + (SLOTS[i].cy_pct * h) / 100;

        d.setTextSize(2);
        d.setTextColor(color, T_BG);
        d.setCursor(gx, gy);
        d.print(glyph);
        d.setTextSize(1);  /* restore */
    }
}
```

- [ ] **Step 2: Build**

Run: `pio run -e cardputer 2>&1 | tail -8`
Expected: exits 0.

- [ ] **Step 3: Commit**

```bash
git add src/ui_ambient.cpp
git commit -m "feat(ui_ambient): PHANTOM cyberpunk glyph flashes painter"
```

---

### Task 9: Add System -> "Ambient Preview" debug feature

**Files:**
- Modify: `src/features/system_tools.cpp` (append the feature)
- Modify: `src/menu.cpp` (add forward decl + System submenu entry)

This gives a clean iteration loop for visually verifying every theme's painter without having to flood through Triton or wait for menus.

- [ ] **Step 1: Append the feature to `src/features/system_tools.cpp`**

Open `src/features/system_tools.cpp` and append at the very bottom of the file:

```cpp
/* ===== ambient preview =====
 * Live preview of the current theme's ambient motion. Hit ESC to exit.
 * 'a' toggles the NVS-backed enable flag so the user can disable ambient
 * globally if they don't like it. */
extern void ui_ambient_tick(int x, int y, int w, int h);
extern bool ui_ambient_enabled(void);
extern void ui_ambient_enabled_set(bool on);

void feat_ambient_preview(void)
{
    auto &d = M5Cardputer.Display;
    d.fillScreen(T_BG);
    ui_status_invalidate();
    ui_draw_status("AMBIENT", "preview");
    ui_draw_footer("[A] toggle on/off  [ESC] exit");

    while (true) {
        uint16_t k = input_poll();
        if (k == PK_ESC) break;
        if (k == 'a' || k == 'A') {
            ui_ambient_enabled_set(!ui_ambient_enabled());
            ui_toast(ui_ambient_enabled() ? "ambient ON" : "ambient OFF",
                     ui_ambient_enabled() ? T_GOOD : T_WARN, 700);
        }
        /* clear body, paint ambient, redraw status caption,
         * loop ~30 fps */
        d.fillRect(0, BODY_Y, SCR_W, BODY_H, T_BG);
        ui_ambient_tick(0, BODY_Y, SCR_W, BODY_H);
        d.setTextColor(T_ACCENT, T_BG);
        d.setCursor(4, BODY_Y + 2);
        d.print(theme().name);
        d.setTextColor(T_DIM, T_BG);
        d.setCursor(4, BODY_Y + 12);
        d.print(ui_ambient_enabled() ? "ambient: on" : "ambient: off");
        delay(33);
    }
    ui_force_clear_body();
}
```

- [ ] **Step 2: Add the menu entry and forward decl in `src/menu.cpp`**

Open `src/menu.cpp`. Near the other `extern void feat_*(void);` lines (around lines 16-128), add:

```cpp
extern void feat_ambient_preview(void);
```

Then locate `MENU_SYS[]` (line 577). Insert a new entry — a clean spot is right after the `'t', "Theme"` entry (line 586). Insert this new entry between the Theme entry and the `'u', "UI / Accessibility"` entry:

```cpp
    { 'b', "Ambient",   "Live ambient motion preview", nullptr, feat_ambient_preview,
      "Live preview of the active theme's ambient motion (motes, scanlines, "
      "TRON grid, Matrix rain, AMBER scan, PHANTOM glyphs). [A] toggles the "
      "NVS-backed enable flag — turn ambient off globally if you don't want "
      "it behind menus." },
```

The hotkey `b` (mnemonic: "background") is currently unused under MENU_SYS so there is no conflict.

- [ ] **Step 3: Build**

Run: `pio run -e cardputer 2>&1 | tail -10`
Expected: build exits 0. If it fails on `theme()` not declared, add `#include "../theme.h"` is already present in system_tools.cpp.

- [ ] **Step 4: Flash and walk every theme**

Run: `pio run -e cardputer -t upload`
Then on-device, for each of the 7 themes:

| Theme | Switch via | Open via | Expected visual |
|---|---|---|---|
| POSEIDON | already default; or System -> Theme -> POSEIDON | System -> Ambient | 8 cyan + magenta motes drift bottom -> top, 1 cyan horizontal wave-band oscillates vertically |
| AMBER | System -> Theme -> AMBER | System -> Ambient | bright amber scanline drifts top -> bottom, dim trailing line below it, 4 amber jitter pixels per frame |
| TRON | System -> Theme -> TRON | System -> Ambient | dim cyan grid scrolls diagonally, magenta 3x3 packet traces an L-path every ~4 s |
| MATRIX | System -> Theme -> MATRIX | System -> Ambient | green Matrix rain at dim brightness (visibly less than `ui_matrix_rain` in Triton) |
| PHANTOM | System -> Theme -> PHANTOM | System -> Ambient | violet/magenta cyberpunk ASCII glyphs (`@`, `#`, `$`, `%`, `^`, `&`, `*`, `<`, `>`, `=`, `?`, `/`, `\\`) fade in/out at 4 positions |
| E-INK | System -> Theme -> E-INK | System -> Ambient | NO ambient motion (paper aesthetic preserved) — just blank body |
| HI-CONTRAST | System -> Theme -> HI-CONTRAST | System -> Ambient | NO ambient motion (accessibility preserved) — just blank body |

Press `A` while in the preview — confirm the toast says "ambient OFF" and motion stops, press again, confirm "ambient ON" and motion resumes. Reboot the device and re-open Ambient Preview — confirm the saved state persisted (NVS write worked).

If a theme paints nothing or paints wrong colors, fix the corresponding `amb_*` function from Tasks 4-8 before continuing.

- [ ] **Step 5: Commit**

```bash
git add src/features/system_tools.cpp src/menu.cpp
git commit -m "feat(ui_ambient): System->Ambient Preview live debug feature"
```

---

### Task 10: Wire ambient into the menu body draw

**Files:**
- Modify: `src/menu.cpp` (add include + one call inside `draw_menu`)

The menu body is rendered by `static void draw_menu(const menu_node_t *parent, int cursor)` in `src/menu.cpp` (line 686). It calls `ui_force_clear_body()` first, then paints the title bar / rows / hint strip on top of the cleared body. We insert the ambient call between the clear and the title.

- [ ] **Step 1: Add the include**

Near the top of `src/menu.cpp`, after `#include "c5_cmd.h"` (line 13), add:

```cpp
#include "ui_ambient.h"
```

- [ ] **Step 2: Insert the ambient call inside `draw_menu`**

Find:
```cpp
static void draw_menu(const menu_node_t *parent, int cursor)
{
    ui_force_clear_body();
    auto &d = M5Cardputer.Display;
```

Change to:
```cpp
static void draw_menu(const menu_node_t *parent, int cursor)
{
    ui_force_clear_body();
    /* Paint theme-aware ambient motion BEFORE menu chrome — rows draw
     * over the top with an opaque background so they remain readable.
     * No-op when the user has disabled ambient via System -> Ambient. */
    ui_ambient_tick(0, BODY_Y, SCR_W, BODY_H);
    auto &d = M5Cardputer.Display;
```

That's it. One single new line of business code, plus a 4-line comment.

- [ ] **Step 3: Build**

Run: `pio run -e cardputer 2>&1 | tail -10`
Expected: exits 0.

- [ ] **Step 4: Flash and verify on every theme**

Run: `pio run -e cardputer -t upload`

Walk the main menu in each theme. Expected:

- POSEIDON / AMBER / TRON / MATRIX / PHANTOM: ambient motion is visible behind menu rows. Menu rows themselves remain readable because each row's cell paints with `T_SEL_BG` / `T_BG` background fill (see `draw_menu` lines 730-750 — the row background fill already overpaints whatever ambient drew underneath in that strip).
- E-INK / HI-CONTRAST: behaves identically to before (no ambient).
- After hitting cursor up/down, the body is cleared and ambient repaints fresh — confirms ambient redraws on every menu redraw, not just on entry.

If menu rows are unreadable: the row's `fillRoundRect` for selected items + the per-row text print should still be opaque against the ambient. Check that `T_BG` on unselected rows is the correct background — for MATRIX / TRON the rows draw text at `T_BG` foreground for contrast, which is fine.

If you see flicker WORSE than usual on an unselected row: that's a sign that `draw_menu` doesn't fill the entire row strip when re-rendering, leaving ambient pixels visible inside row text gaps. Out of scope for this plan (flicker work is explicitly excluded per the spec). Note it and move on.

- [ ] **Step 5: Commit**

```bash
git add src/menu.cpp
git commit -m "feat(menu): paint ui_ambient_tick behind every menu draw"
```

---

### Task 11: Implement `theme_effective_accent()` + `theme_effective_accent2()` and redefine `T_ACCENT` macros

**Files:**
- Modify: `src/theme.h` (add accessors + redefine macros)
- Modify: `src/theme.cpp` (add blend helper + implementations)

This is the highest-blast-radius change in this plan. `T_ACCENT` is referenced widely. After this task, every existing call site automatically inherits mood-tinting when Triton is active and in HUNT / STEALTH / SURGICAL / STORM, with zero per-feature edits.

- [ ] **Step 1: Add accessor declarations to `src/theme.h`**

In `src/theme.h`, immediately after `const poseidon_theme_t &theme(void);` (currently line 44), insert:

```cpp
/* Triton-mood-modulated accent colors. When the Triton feature is active
 * AND in a non-neutral mode (HUNT / STEALTH / SURGICAL / STORM), these
 * return the base theme accent blended toward a per-mood tint. When
 * Triton is not active, returns the unchanged base accent.
 *
 * The T_ACCENT / T_ACCENT2 macros below route through these helpers, so
 * every existing call site picks up mood-tinting automatically. */
uint16_t theme_effective_accent(void);
uint16_t theme_effective_accent2(void);
```

Then change the macros at the bottom of `src/theme.h`. Currently:
```cpp
#define T_ACCENT   (theme().accent)
#define T_ACCENT2  (theme().accent2)
```

Replace both lines with:
```cpp
/* Routed through the mood-modulated accessor. When Triton is neutral
 * (or not active), this is identical to theme().accent[2]. */
#define T_ACCENT   (theme_effective_accent())
#define T_ACCENT2  (theme_effective_accent2())
```

- [ ] **Step 2: Add `blend_565` helper + mood tints + implementations to `src/theme.cpp`**

Open `src/theme.cpp`. After the `theme()` definition at the bottom of the file (currently line 185 returns `THEMES[s_current]`), append:

```cpp
/* ---- mood-modulated effective accent ---- */

/* RGB565 alpha blend: result = a*(255-t)/255 + b*t/255. */
static uint16_t blend_565(uint16_t a, uint16_t b, uint8_t t)
{
    uint8_t ar = (a >> 11) & 0x1F, ag = (a >> 5) & 0x3F, ab = a & 0x1F;
    uint8_t br = (b >> 11) & 0x1F, bg = (b >> 5) & 0x3F, bb = b & 0x1F;
    uint8_t r  = (uint8_t)((ar * (255 - t) + br * t) / 255);
    uint8_t g  = (uint8_t)((ag * (255 - t) + bg * t) / 255);
    uint8_t bl = (uint8_t)((ab * (255 - t) + bb * t) / 255);
    return (uint16_t)((r << 11) | (g << 5) | bl);
}

/* Per-mood blend targets — RGB565 conversions of the spec's hex values:
 *   HUNT     #ff2266 -> warm aggressive red
 *   STEALTH  #3060a0 -> cool muted blue
 *   SURGICAL #ffffff -> sharp white
 *   STORM    #ffaa00 -> amber
 *
 * These are the only literal RGB565 values in this plan — they are the
 * tint targets themselves, not theme colors, so they intentionally bypass
 * the theme() pipeline. */
#define MOOD_TINT_HUNT     0xF14CU  /* 0xff/0x22/0x66 -> 11111 001000 01100 */
#define MOOD_TINT_STEALTH  0x3304U  /* 0x30/0x60/0xa0 -> 00110 011000 10100 */
#define MOOD_TINT_SURGICAL 0xFFFFU  /* white */
#define MOOD_TINT_STORM    0xFD40U  /* 0xff/0xaa/0x00 -> 11111 101010 00000 */

/* Blend amount: ~55% toward tint for HUNT / STEALTH / STORM gives a
 * visible shift without obliterating the theme. SURGICAL uses a lighter
 * blend so the accent reads as "sharper" rather than "white-washed". */
#define MOOD_BLEND_DEFAULT 140
#define MOOD_BLEND_SURGICAL 100

extern "C" int triton_current_mode_int(void);

uint16_t theme_effective_accent(void)
{
    uint16_t base = theme().accent;
    switch (triton_current_mode_int()) {
    case 1: return blend_565(base, MOOD_TINT_HUNT,     MOOD_BLEND_DEFAULT);
    case 2: return blend_565(base, MOOD_TINT_STEALTH,  MOOD_BLEND_DEFAULT);
    case 3: return blend_565(base, MOOD_TINT_SURGICAL, MOOD_BLEND_SURGICAL);
    case 4: return blend_565(base, MOOD_TINT_STORM,    MOOD_BLEND_DEFAULT);
    }
    return base;
}

uint16_t theme_effective_accent2(void)
{
    uint16_t base = theme().accent2;
    switch (triton_current_mode_int()) {
    case 1: return blend_565(base, MOOD_TINT_HUNT,     MOOD_BLEND_DEFAULT);
    case 2: return blend_565(base, MOOD_TINT_STEALTH,  MOOD_BLEND_DEFAULT);
    case 3: return MOOD_TINT_SURGICAL;  /* SURGICAL: hard white for accent2 */
    case 4: return blend_565(base, MOOD_TINT_STORM,    MOOD_BLEND_DEFAULT);
    }
    return base;
}
```

- [ ] **Step 3: Build (this is the hot moment — `T_ACCENT` is referenced everywhere)**

Run: `pio run -e cardputer 2>&1 | tail -30`
Expected: exits 0. Most call sites use `T_ACCENT` as an `uint16_t` value at runtime, which still works when the macro is now a function call.

If the build fails: the most likely culprit is a static initializer of an `uint16_t` constant or a `constexpr` table that uses `T_ACCENT`. RGB565 values from a function call are not constexpr. Search with:
```bash
grep -rn "T_ACCENT" src/ | grep -E "(static const|constexpr|= T_ACCENT)"
```
If any hit is in a static initializer (e.g. `static const uint16_t COLS[] = { T_ACCENT, ... };`), change that array to be filled at runtime in an init function. None are expected (existing usage is all expression-context per skim of `src/ui.cpp` / `src/menu.cpp`), but call this out so the engineer doesn't get stuck.

- [ ] **Step 4: Flash and verify with Triton NOT active**

Run: `pio run -e cardputer -t upload`

Walk every menu and theme. Expected: visually identical to the pre-Task-11 state. `triton_current_mode_int()` returns 0 -> `theme_effective_accent()` returns `theme().accent` unchanged.

If anything looks different from before: the blend math has a bug, or a theme is somehow returning a non-zero mode. Check `s_triton_active` is initialized to `false`.

- [ ] **Step 5: Flash and verify mood tinting WHILE in Triton**

Enter Triton (root menu -> 't' -> pick HUNT -> let it run). The status bar accent, the row borders within Triton's own UI, and any selected indicator should now read warmer (shifted toward the magenta-red `MOOD_TINT_HUNT`).

Press ESC out of Triton -> back to main menu. The accent should snap back to the base theme accent because `s_triton_active` flips false on exit.

Repeat for STEALTH (cool blue shift), SURGICAL (sharper white), STORM (amber shift).

If the tint doesn't apply when expected: check `s_triton_active` is being set true at `feat_triton()` entry (Task 2, Step 2). If it does apply but doesn't go away: check the inner-helper refactor (Task 2, Step 3) put `s_triton_active = false` AFTER `triton_run_inner()`.

- [ ] **Step 6: Commit**

```bash
git add src/theme.h src/theme.cpp
git commit -m "feat(theme): T_ACCENT routes through mood-modulated effective accent"
```

---

### Task 12: Final cross-theme cross-mood smoke test

**Files:** none (verification only)

This is a deliberate verification pause before declaring the plan done. No code changes — just a hardware run-through to confirm the whole stack works end-to-end.

- [ ] **Step 1: Boot from cold**

Power-cycle the Cardputer. Expected: boots normally, no crash.

- [ ] **Step 2: Walk all 7 themes through the main menu**

For each theme (POSEIDON, AMBER, TRON, MATRIX, PHANTOM, E-INK, HI-CONTRAST):

- Switch via System -> Theme.
- Confirm main menu paints with ambient motion in the colored 5 themes, no ambient in E-INK / HI-CONTRAST.
- Confirm menu row text remains readable on top of the ambient.
- Confirm no flicker WORSE than baseline.

- [ ] **Step 3: Walk all 4 Triton modes**

For each mode (HUNT, STEALTH, SURGICAL, STORM):
- Enter Triton, pick the mode, let it run for ~10 s on the POSEIDON theme.
- Observe: the status bar accent + Triton's own UI accents should be visibly shifted toward the mood tint (HUNT warm-magenta, STEALTH cool-blue, SURGICAL whitened, STORM amber).
- Note that ambient ALSO speeds up / slows down per the mood scale — POSEIDON motes climb visibly faster on STORM, slower on STEALTH.

- [ ] **Step 4: Toggle ambient off via the new System submenu, confirm it sticks across reboot**

System -> Ambient Preview -> press `A`. Toast says "ambient OFF". ESC. Walk the main menu — no ambient. Power-cycle. Walk the main menu — still no ambient (NVS persistence). System -> Ambient Preview -> press `A` -> "ambient ON" toast -> ESC -> ambient is back.

- [ ] **Step 5: Final commit (chore-level summary if desired)**

If everything verified, no source change required. If the verification surfaces a bug, fix it as a Task-12-fix commit, then re-run Step 2 and Step 3.

```bash
git status
# expected: nothing to commit, working tree clean
```

---

## Self-review checklist

Run through this list before declaring the plan ready for execution.

### Spec coverage

| Spec requirement | Implementing task |
|---|---|
| `ui_ambient_tick(int,int,int,int)` public function | Task 1 (header), Task 3 (dispatcher) |
| Per-theme ambient: POSEIDON motes + wave-band | Task 4 |
| Per-theme ambient: PHANTOM glyph flashes (cyberpunk, NOT occult) | Task 8 |
| Per-theme ambient: MATRIX dimmed rain | Task 7 |
| Per-theme ambient: AMBER scanline + jitter | Task 5 |
| Per-theme ambient: TRON grid + packet hop | Task 6 |
| Per-theme ambient: E-INK no-op | Task 3 (switch case) |
| Per-theme ambient: HI-CONTRAST no-op | Task 3 (switch case) |
| Mood -> ambient speed scale (HUNT 2.0x / STEALTH 0.5x / SURGICAL 1.0x / STORM 3.5x) | Task 3 (`ambient_speed_scale`) |
| `triton_current_mode_int()` accessor | Task 2 |
| `s_triton_active` lifecycle | Task 2 |
| `theme_effective_accent()` + `theme_effective_accent2()` | Task 11 |
| `T_ACCENT` / `T_ACCENT2` macro redefinition | Task 11 |
| Mood tint targets: HUNT warm, STEALTH cool, SURGICAL white, STORM amber | Task 11 (MOOD_TINT_*) |
| One-line wiring into `menu.cpp::draw_menu` | Task 10 |
| System -> Ambient Preview debug feature | Task 9 |
| NVS persistence of the on/off flag | Task 3 (`ui_ambient_enabled` / `_set`), Task 9 (toggle UI) |
| No PSRAM / no IRAM / no malloc-in-render | Task 3 comments + per-task pixel budgets |
| Local commits only (no push/tag/release) | All tasks: `git add` + `git commit` only |

Spec items intentionally NOT covered by this plan: Triton sprite sheet (blocked on `assets/raw/triton_concept/` empty), corner mascot, narrative toasts, screensavers, any GIF assets. Those will be in subsequent plans.

### Placeholder scan

- [x] No "TBD" / "TODO" / "implement later" / "fill in" anywhere.
- [x] Every code block is complete and copy-pasteable — no `...` ellipses inside function bodies.
- [x] Pixel budgets explicitly stated per painter task.
- [x] Every `pio run` step has its expected exit (0).
- [x] Every flash step has explicit visual expectations.
- [x] Every commit has an explicit `git add` for the right files.

### Type consistency

- [x] `triton_current_mode_int()` always returns `int`, never `triton_mode_t`. Defined `extern "C"` in Task 2; declared the same way in Task 3 and Task 11.
- [x] `ui_ambient_tick` always takes `(int,int,int,int)`. Same signature in Task 1 header, Task 3 implementation, Task 9 forward decl, Task 10 caller.
- [x] `ui_ambient_enabled` returns `bool`, `ui_ambient_enabled_set` takes `bool`. Same in Task 1 header and Task 3 implementation. Task 9 toggle uses the same signatures.
- [x] `T_ACCENT` / `T_ACCENT2` consistently macros that expand to function calls. The functions themselves are `uint16_t` returns. No callsite expects `constexpr`.
- [x] `s_triton_active` is `volatile bool` so the `extern "C"` accessor in a different translation unit reads a live value. Set in `feat_triton()` (regular task context), read in `triton_current_mode_int()` (also regular task context — neither is in an ISR), so `volatile` is paranoid-safe but not strictly required.
- [x] NVS namespace name `pamb` is 4 chars (under the 15-char NVS key limit) and unique (does not collide with `pui`, `poseidon`, `pscr` per existing usage in the repo).

### Risk callouts (honest)

The single biggest risk in this plan is **Task 11's macro redefinition of `T_ACCENT` causing a static-initializer build break.** Every other change is additive and trivially reversible; Task 11 forces a runtime function-call into a macro that may have been used in a `constexpr` or `static const uint16_t array[] = { ... }` context somewhere we haven't audited. Mitigation: Task 11 Step 3 explicitly tells the engineer to grep for that pattern and convert to runtime init if found. Fallback if the pattern is too widespread: gate the redefinition behind a `#define POSEIDON_ENABLE_MOOD_TINT 1` preprocessor flag in `theme.h`, and only enable it after fixing each offending site one at a time.

Secondary risk: ambient drawing UNDER menu rows could expose flicker that was previously masked by the body's solid `T_BG` fill. The plan explicitly says flicker is out of scope; if the user finds it intolerable they can disable ambient via the new toggle. Long-term fix is a sprite-buffer compositing pass which is its own future plan.

### Spec ambiguity resolved

The spec named the menu hook function as `menu_draw_body()`, but the actual function in `src/menu.cpp` is `static void draw_menu(const menu_node_t *parent, int cursor)` (line 686). Resolved by inspecting the source — Task 10 hooks `draw_menu` directly, immediately after `ui_force_clear_body()` and before the title paint.

The spec also assumed `s_triton_active` already existed; it does not in `src/features/triton.cpp` as of 2026-04-25. Resolved by Task 2 adding both the flag and the entry/exit lifecycle (refactoring `feat_triton()` into a thin wrapper over an inner helper so every early-return cleans up correctly).
