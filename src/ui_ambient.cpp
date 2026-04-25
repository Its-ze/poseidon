/*
 * ui_ambient.cpp — see ui_ambient.h.
 *
 * One static painter per theme. Dispatched by theme_current_id().
 *
 * Mood-aware speed modulation is wired into ambient_speed_scale() but
 * currently always returns 1.0 — the Triton-side accessor
 * (triton_current_mode_int) hasn't been added yet. Adding it requires
 * touching triton.cpp which is the active freeze-regression area; we
 * ship ambient at neutral speed first, mood tint comes once the freeze
 * is settled. See docs/superpowers/plans/2026-04-25-poseidon-ambient-motion.md
 * Task 2 for the deferred change.
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

/* ---- mood-modulated speed scale (currently always neutral) ----
 * neutral = 1.0; once Task 2 lands, HUNT 2.0, STEALTH 0.5, SURGICAL 1.0,
 * STORM 3.5 per the spec. */
static float ambient_speed_scale(void)
{
    return 1.0f;
}

/* Per-theme painter forward decls. */
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
        uint8_t  col_pct;
        uint16_t period_ms;
        uint32_t seed;
        bool     alt;
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
        int my = (h - 2) - (int)((int64_t)phase * (h + 2) / (int64_t)period);
        int mx = (MOTES[i].col_pct * w) / 100;
        if (my < 0 || my >= h) continue;
        uint16_t color = MOTES[i].alt ? T_ACCENT2 : T_ACCENT;
        d.drawPixel(x + mx, y + my, color);
        if (my > 0)     d.drawPixel(x + mx, y + my - 1, color);
        if (my < h - 1) d.drawPixel(x + mx, y + my + 1, color);
    }

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
    d.drawPixel(x + px - 2, y + py,     T_ACCENT2);
    d.drawPixel(x + px + 2, y + py,     T_ACCENT2);
    d.drawPixel(x + px,     y + py - 2, T_ACCENT2);
    d.drawPixel(x + px,     y + py + 2, T_ACCENT2);
}

/* ---- MATRIX: dimmed reuse of existing rain ----
 * The screensaver phase will run rain at full brightness over the whole
 * screen; for ambient we want it visibly subtler, so we pass T_DIM
 * instead of T_ACCENT. ui_matrix_rain handles its own per-column state
 * and 80 ms advance throttling internally. */
static void amb_matrix(int x, int y, int w, int h)
{
    ui_matrix_rain(x, y, w, h, T_DIM);
}

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
        d.setTextSize(1);
    }
}
