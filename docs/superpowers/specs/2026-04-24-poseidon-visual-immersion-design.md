# POSEIDON visual + immersion overhaul

**Date:** 2026-04-24
**Target:** POSEIDON v0.6.0 on M5Stack Cardputer-Adv (240 × 135, ESP32-S3, 512 KB SRAM, no PSRAM)
**Scope:** purely visual / immersion. **No logic or function changes.**

---

## Goals

1. Make idle and static screens feel **alive** via theme-appropriate ambient motion.
2. Glow up Triton — new sprite (B+D fusion: deep-sea demigod × glitchy AI construct), corner mascot on opted-in screens, mood tints theme accent, narrative toasts on events.
3. Add **screensavers** — 8 procedural + 2 GIF-sourced hero loops, theme auto-pick, 2-minute idle trigger.

## Non-goals

- **Flicker elimination.** Out of scope. No `LGFX_Sprite` compositing rewrite. Existing direct-to-display drawing stays as-is.
- **Radio lifecycle / power management.** Out of scope. Current ad-hoc `WiFi.mode()` calls remain untouched.
- **Tactile SFX changes.** Out of scope. Existing `sfx_*` paths preserved exactly.
- **Feature behavior.** No feature's input handling, state machine, or output behavior changes. Only visual layer additions.

## Constraints

- **No new architectural primitives.** All work additive — new modules, opt-in helpers. Existing `ui.cpp`, `menu.cpp`, feature files unchanged unless explicitly opting in.
- **RAM:** ~200 KB free heap typical. Triton sprite canvas budget capped at ~10 KB total (~8 KB face region, ~1 KB corner mascot).
- **Flash:** Triton face sprites ~9 KB (9 frames × 48 × 40 × 4-bit), corner mascot sprites ~3 KB (9 frames × 24 × 24 × 4-bit), per-theme palette LUT ~0.2 KB (7 themes × 16 colors × 2 bytes); screensaver hero GIFs ~155 KB (Hokusai Wave + Jellyfish Drift, 120×67 × ~20 frames × 4-bit indexed). Total asset add ~167 KB.
- **Visual companion brainstorm session output** persists at `.superpowers/brainstorm/38921-1777046266/content/*.html` for reference during implementation.

---

## §1 — Ambient motion system

### Module
New `ui_ambient.cpp` / `ui_ambient.h`. One public function:

```c
void ui_ambient_tick(int x, int y, int w, int h);
```

- Paints one frame of theme-appropriate ambient motion into the given rect.
- Caller invokes at the *start* of its refresh loop, before drawing content on top.
- Timing keyed off `millis()` internally — caller holds no state.
- No-op if `theme_current_id()` resolves to a theme with `ambient_style == AMB_NONE` (EINK, HICONTRAST).

### Per-theme ambient style

| Theme | Style | Implementation sketch |
|---|---|---|
| POSEIDON | Deep-sea motes + slow wave-band ripple | ~6 motes drawn as `drawPixel` w/ shadow ring, animated upward with `(millis() + seed) % cycle`; wave-band is one `drawFastHLine` whose Y oscillates sin-style |
| PHANTOM | Violet sigil flashes (cyberpunk glyphs, NOT fantasy occult — see §3 Phantom Override aesthetic) | small unicode-style char draws fading in/out via per-glyph timer |
| MATRIX | Reuse existing `ui_matrix_rain` at reduced density/intensity | wrap existing helper, pass alpha-equivalent (lower brightness palette) |
| AMBER | CRT scanline drift + phosphor jitter | one moving `drawFastHLine` + every-frame XOR speckle on random pixels |
| TRON | Scrolling grid + cyan/magenta packet hop | grid drawn as `drawFastHLine`/`drawFastVLine` mod-30; packet is a single `fillCircle` advancing along precomputed grid path |
| EINK | None | no-op |
| HICONTRAST | None | no-op |

### Mood-modulates-amplitude

When Triton is the active feature OR `triton_mode_t s_mode` is non-neutral (background-active hooks already exist):

| Triton mode | Ambient speed multiplier | Brightness |
|---|---|---|
| HUNT | 2.0× | full |
| STEALTH | 0.5× | 0.6× |
| SURGICAL | 1.0× | full |
| STORM | 3.5× | full + occasional flash burst |
| (neutral / sleeping) | 1.0× | full |

Implementation: `ui_ambient_tick` reads `triton_current_mode()`, applies multiplier to its internal phase calc.

### Integration

- One call inserted into `menu_draw_body()` in `menu.cpp` — every menu screen gets ambient for free.
- Idle-waiting screens in features (where the user just sits between events) opt in by adding `ui_ambient_tick(0, BODY_Y, SCR_W, BODY_H)` at the top of their refresh tick.
- No automatic injection elsewhere; explicit opt-in keeps behavior surprising-free.

---

## §2 — Triton omnipresence

### A. New sprite asset (in progress externally)

**Direction:** B+D fusion — ancient merman demigod (bronze skin, weathered, glowing cyan eyes, conch-shell horn) merged with glitchy AI construct (wireframe edges, circuit-trace tattoos, polygon dissolution, scanline artifacts).

**Source generation:** user generating sprite sheet via Google AI Studio with `gemini-2.5-flash-image` (nanobanana). Three prompts authored (full-body demigod-lean, full-body AI-construct-lean, face-only fusion). Drop location:
```
C:/Users/D/poseidon/assets/raw/triton_concept/
```

**Processing pipeline (this spec defines, implementation pending sheet drop):**
1. Cut sprite sheet on grid (4 cols × 2 rows = 8 poses).
2. Per-frame downscale to 48 × 40 (face region) and 24 × 24 (corner mascot) using nearest-neighbor with optional dither.
3. Quantize each frame to a 16-color palette (theme-aware — palette varies per theme).
4. Emit `triton_sprites.h` with `PROGMEM` 4-bit indexed arrays + frame enum:
   ```c
   enum triton_frame_t {
       TRI_IDLE, TRI_HUNT_SCAN_0, TRI_HUNT_SCAN_1,
       TRI_ALERT, TRI_ATTACK, TRI_SLEEPY,
       TRI_STOKED, TRI_DESPAIR, TRI_STEALTH,
       TRI_FRAME__COUNT
   };
   extern const uint8_t PROGMEM triton_sprite_data[TRI_FRAME__COUNT][...];
   extern const uint16_t PROGMEM triton_palette[THEME__COUNT][16];
   ```
5. Rewire `draw_face()` in `triton.cpp` to blit sprites instead of vector-drawing. Preserve current `mood_t` → frame mapping; preserve all timing/tick logic.

### B. Corner mascot

- 24 × 24 px sprite, drawn at bottom-right of `BODY_H` (anchor: `SCR_W - 28, BODY_Y + BODY_H - 28`).
- New helper `ui_mascot_tick(int x, int y)` — features call in their refresh loop on opt-in screens.
- Frame selection driven by polled comparison against existing per-feature stat counters (no new event bus — out of scope):
  - default → idle/breathing
  - feature's existing hit counter (e.g., `wifi_scan` AP-found count, `wifi_pmkid` handshake count, `triton_stats.session_hs`) increments → `ui_mascot_tick` reads delta vs last-seen → `TRI_STOKED` for 2 s
  - 30 s of no counter delta → `TRI_SLEEPY` then back to idle
  - last input timestamp (already in `input.cpp`) updated within last 200 ms → 1-frame `TRI_ALERT` flash
- Off by default on chrome-heavy screens (spectrum analyzers, attack dashboards). Opt-in screens initially: WiFi Scan, BLE Scan, idle wait pages, Triton's own face screen (where it's already there).

### C. Mood tints theme accent

- New `theme_effective_accent()` / `theme_effective_accent2()` accessors:
  ```c
  uint16_t theme_effective_accent(void)  { return mood_modulate(theme().accent, triton_current_mode()); }
  uint16_t theme_effective_accent2(void) { return mood_modulate(theme().accent2, triton_current_mode()); }
  ```
- `mood_modulate()` shifts hue/saturation per mode:
  - HUNT → push toward `#ff2266` (warm aggressive)
  - STEALTH → push toward `#3060a0` (cool muted)
  - SURGICAL → push toward `#ffffff` (sharp white)
  - STORM → push toward `#ffaa00` + occasional pulse to `#ff60d0`
  - neutral → unchanged (returns base `theme().accent`)
- Replace `T_ACCENT` macro definition site to call `theme_effective_accent()` — every existing `T_ACCENT` reference in features auto-inherits mood tint with zero per-feature changes.
- Status bar shows `TRITON · <MODE>` when non-neutral; reverts to plain when neutral.

### D. Narrative toasts

- New helper `ui_triton_toast(const char *msg)`:
  - Slides down from top edge over ~150 ms
  - Holds 3 s
  - Slides back up over ~150 ms
  - Border: `T_ACCENT2` (effective). Background: 90 % opacity dark. Source label: `"TRITON:"` in `T_ACCENT2`, message in `T_ACCENT`.
- Non-blocking, non-queued: latest replaces prior.
- Triggered from existing event hooks:
  - 3rd, 5th, 10th handshake of session → "that's three tonight. stacking up."
  - Boot, after `triton_learn_load()`, if `s_stats.session_hs > 0` → "caught N hs overnight. brain +M. good morning."
  - Mode change → "moving to STEALTH — keeping quiet."
- New setting `triton.chatty` (default: true). Off → toasts suppressed.

---

## §3 — Screensavers

### Module

New `screensaver.cpp` / `screensaver.h`.

```c
void screensaver_init(void);                  // load saved settings from NVS
void screensaver_tick(uint32_t now_ms);       // call from main loop; checks idle, triggers as needed
void screensaver_run(saver_id_t id);          // owns screen until any keypress; returns void
saver_id_t screensaver_for_theme(theme_id_t); // resolves auto-pick
```

Main loop adds a single line in `loop()`:
```c
screensaver_tick(millis());
```

### Trigger

- Idle: 120 000 ms (2 min, configurable in settings: 15 s / 30 s / 1 min / 2 min / 5 min / disabled).
- Idle = no input event since `last_input_ms` (already tracked by `input.cpp`).
- Manual: new System menu entry "Screensaver preview" calls `screensaver_run(screensaver_for_theme(theme_current_id()))`.
- Exit: any keypress consumed (does NOT propagate to underlying feature).

### Procedural screensavers (8)

| ID | Name | Default theme | Description |
|---|---|---|---|
| SAV_DEEP | Deep Descent | POSEIDON | Bioluminescent motes rising bottom→top (cyan + magenta), occasional Triton silhouette drifts horizontally every ~18 s, intermittent magenta flash bursts. Triton silhouette uses `triton_sprite_data[TRI_IDLE]` scaled 2× (post §2A processing). |
| SAV_CRT | CRT Static | AMBER | Vintage TV static (random amber pixels), slow vertical roll-bar (`fillRect` band moving top→bottom), faint "POSEIDON" burn-in dimly visible center. |
| SAV_HYPER | Hyperspace Corridor | TRON | Forward-zoom tunnel: cyan + magenta rings flying outward from screen center via z-axis sim (drawn as expanding `drawRect` from center, alpha-faded), faint grid-texture overlay, glowing pulse core dead center. *Replaces "Circuit Grid" — TRON theme default.* |
| SAV_RAIN | Matrix Rain Full | MATRIX | Existing `ui_matrix_rain` extended to full-screen, denser column count, longer trails. |
| SAV_OVERRIDE | Phantom Override | PHANTOM | Stacked layers: dim cipher hex stream noise (background) + SIGINT-style intercept side panel (right) + rotating wireframe phantom skull bottom-left + magenta crosshair tracking targets across viewport + "OVERRIDE" / "INTERCEPT LOCKED" flash overlays every ~4 s + REC top bar + LIVE bottom bar. |
| SAV_ABYSS | Abyssal Grid | (wildcard) | TRON × POSEIDON fusion: wireframe ocean floor in perspective, neon vector jellyfish (cyan + magenta) drifting up, vector triangle-fish school crossing, sonar pulse rings emanating from center, depth-meter pulsing, wireframe Triton swimming through every ~20 s. |
| SAV_SONAR | Sonar Bridge | (wildcard) | Top-down sub sonar: 4 concentric rings, rotating sweep arm with cyan afterglow cone, blips fade in/out at varying ranges, target lock corners pulsing on tracked contact, full HUD chrome (DEPTH / HEADING / RANGE / SONAR mode / contact count / locked count / timestamp). |
| SAV_WAR | War Room | (wildcard) | Nine micro-panels of fake operational data: spectrum bars, sparklines, target list, mini sonar, terminal scrollback, central tactical map with blinking targets, RF intercept feed, channel map, packet-rate sparkline, uplink status. NORAD-density operational density. |

### Hero GIFs (2, sourced externally)

| ID | Name | Source plan | Approx flash |
|---|---|---|---|
| SAV_HOKUSAI | Hokusai Wave | Find / generate animated wave loop, downscale to 120 × 67, ~20 frames, palette-quantize to 4-bit indexed | ~80 KB |
| SAV_JELLY | Jellyfish Drift | Find / generate bioluminescent jellyfish loop on dark BG, ~120 × 67, ~18 frames, 4-bit indexed | ~72 KB |

Stored as `PROGMEM` arrays in `assets/screensaver_hokusai.h`, `assets/screensaver_jellyfish.h`. Renderer: `screensaver_play_gif(const gif_t *)` shared helper.

### Theme auto-pick map

| Theme | Default | User can override (settings) |
|---|---|---|
| POSEIDON | SAV_DEEP | SAV_HOKUSAI / SAV_JELLY / SAV_ABYSS / SAV_SONAR |
| AMBER | SAV_CRT | (any) |
| TRON | SAV_HYPER | SAV_ABYSS |
| MATRIX | SAV_RAIN | (any) |
| PHANTOM | SAV_OVERRIDE | SAV_WAR |
| EINK | none — backlight dim only | (none — paper aesthetic respected) |
| HICONTRAST | none — backlight dim only | (none — accessibility respected) |

`SAV_WAR` is wildcard — defaults to no auto-pair, user picks manually from any theme.

### Settings

New System menu entry "Screensaver" → submenu:
- Enable / disable (default: enable)
- Idle timeout (15 s / 30 s / 1 m / 2 m / 5 m / off) — default 2 m
- Selection mode: "Auto-pick from theme" (default) / "User pick" → manual list of all 10 savers
- Preview current

Persists to NVS under namespace `pscr` (keys: `enabled`, `timeout`, `mode`, `manual_id`).

---

## §4 — Migration / rollout

### Strategy: single PR, additive, default-on for the safe stuff

All work in this spec is additive — no existing files modified destructively. Single PR delivers:

1. New files:
   - `src/ui_ambient.cpp` / `src/ui_ambient.h`
   - `src/screensaver.cpp` / `src/screensaver.h`
   - `src/sprites/triton_sprites.h` (generated post sprite-sheet processing)
   - `src/assets/screensaver_hokusai.h`, `src/assets/screensaver_jellyfish.h` (post GIF sourcing)
   - `src/features/screensaver_settings.cpp` (settings submenu)

2. Modified files (minimal touchpoints):
   - `src/theme.h` / `src/theme.cpp` — add `theme_effective_accent()`, redefine `T_ACCENT` macro to call it. *Backward-compat: every existing `T_ACCENT` reference inherits mood-tint automatically with zero source change.*
   - `src/menu.cpp` — single `ui_ambient_tick(...)` call in `menu_draw_body()`.
   - `src/main.cpp` — single `screensaver_tick(millis())` call in main loop.
   - `src/features/triton.cpp` — `draw_face()` rewritten to blit sprites instead of vector calls. Mood→frame mapping and timing preserved.
   - `src/features/wifi_scan.cpp`, `src/features/ble_scan.cpp` — opt in to corner mascot via `ui_mascot_tick(...)` in refresh loop.
   - `src/menu.cpp` — add System submenu entry for "Screensaver".

3. Settings keys added to NVS:
   - `pscr/enabled`, `pscr/timeout`, `pscr/mode`, `pscr/manual_id`
   - `triton/chatty`

### Rollout order within the PR

Build one piece at a time, verify on hardware before stacking:

1. `ui_ambient_tick` + per-theme implementations (no integration yet) → unit test on AMBIENT preview screen accessible from System menu.
2. Wire ambient into `menu_draw_body()` → visual verification across all 7 themes.
3. `theme_effective_accent()` + macro redefine → verify every screen looks the same when Triton is neutral.
4. Triton sprite processing pipeline (after sheet drop) → emit `triton_sprites.h` → verify face renders correctly across all 7 theme palettes.
5. Rewire `draw_face()` → A/B compare against vector version on hardware.
6. Mood-tint integration → verify accent shifts on mode change.
7. `ui_mascot_tick` + 2 opt-in features (WiFi Scan, BLE Scan) → verify corner placement, event reactions.
8. `ui_triton_toast` + 3 trigger sites → verify slide animation, no flicker amplification.
9. `screensaver` module + 5 default-mapped procedural savers → verify idle trigger, theme auto-pick, exit on keypress.
10. 3 wildcard procedural savers (Abyss, Sonar, War) → verify settings selection.
11. Hero GIF sourcing + processing pipeline → emit asset headers → verify playback at expected framerate.
12. Settings submenu → verify NVS persistence across reboot.

### Risks & rollback

- **Mood-tint everywhere** — biggest blast radius. Gate behind `triton.tint_global` setting (default: true). If any theme looks wrong tinted, user can disable.
- **Sprite blit performance in Triton face** — if frame rate drops below the existing vector-drawn version, keep vector path as fallback flag `triton.use_sprites` (default: true).
- **Screensaver flash size** — if hero GIFs blow flash budget (likely under 200 KB total, but verify against current `pio run` output), ship procedural-only first, GIFs as a follow-up.
- **NVS keys** — using fresh namespace `pscr` to avoid collision with existing settings.

### What does NOT ship in this PR

- Per-theme animation polish beyond the 7 ambient styles defined.
- Additional Triton sprite poses beyond the 9 enumerated.
- Additional screensavers beyond the 10 defined.
- GIF authoring tools (compression/quantization done offline, headers committed as data).

---

## Implementation references

- Visual companion brainstorm session: `.superpowers/brainstorm/38921-1777046266/content/`
  - `section1-ambient.html` — POSEIDON / AMBER / TRON / EINK ambient mockups
  - `section2-triton-ambient.html` — corner mascot + mood tints + toasts composed with ambient
  - `section2-theme-mood-matrix.html` — AMBER+HUNT, TRON+STEALTH, MATRIX+SURGICAL, PHANTOM+STORM
  - `section3-cooler.html` — Phantom Override + Abyssal Grid
  - `section3-more-dope.html` — Sonar Bridge + Hyperspace Corridor + War Room
- nanobanana sprite drop: `assets/raw/triton_concept/` (pending user)
- Existing UI primitives in `src/ui.h` (matrix_rain, radar, waves, hexstream, glitch, eq_bars, freq_bars) reused where applicable.
