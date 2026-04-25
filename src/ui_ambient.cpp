/*
 * ui_ambient.cpp — see ui_ambient.h.
 *
 * Single combined cyberpunk ambient: three layers painted every frame.
 *   1. TRON grid scrolling diagonally at low brightness — gives the
 *      world structure.
 *   2. POSEIDON motes drifting bottom -> top in cyan/magenta — gives
 *      life.
 *   3. Magenta packet hopping an L-path on a slow loop — gives accent.
 *
 * The earlier per-theme dispatch (5 painters) was scrapped when we
 * collapsed to a single POSEIDON theme. All three layers stay subtle
 * enough that menu rows remain readable on top.
 *
 * State invariants:
 *   - All per-frame state derives from millis() and esp_random(); no
 *     module-scope mutable state except the cached NVS flag.
 *   - Functions write directly to M5Cardputer.Display (no sprite buffer,
 *     no PSRAM — this unit's PSRAM is broken).
 *   - Functions are NOT marked IRAM_ATTR — IRAM is full.
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

/* Intentional non-theme literals. AMB_GRID is the dim purple for the
 * TRON grid layer — has to sit BELOW T_DIM in luminance so menu hint
 * text on top stays the dominant visual element. Not theme-derived
 * because the grid color IS the cyberpunk-grid identity. */
#define AMB_GRID  0x1004u  /* very dark cyan-purple */

void ui_ambient_tick(int x, int y, int w, int h)
{
    if (!ui_ambient_enabled()) return;
    if (w <= 0 || h <= 0)      return;

    auto &d = M5Cardputer.Display;
    uint32_t now = millis();

    /* ---- Layer 1: TRON grid (scrolling diagonally) ----
     * 30 px spacing both axes. Grid offset advances 1 cell per 6 s for
     * a subtle parallax drift. Lines drawn in AMB_GRID — well below
     * T_DIM so menu hints sit comfortably on top. */
    int scroll = (int)((now % 6000u) * 30u / 6000u);
    for (int gx = -scroll; gx < w; gx += 30) {
        if (gx >= 0 && gx < w) d.drawFastVLine(x + gx, y, h, AMB_GRID);
    }
    for (int gy = -scroll; gy < h; gy += 30) {
        if (gy >= 0 && gy < h) d.drawFastHLine(x, y + gy, w, AMB_GRID);
    }

    /* ---- Layer 2: POSEIDON motes drifting bottom -> top ----
     * 8 motes, each on its own period + x-position + cyan/magenta tint.
     * 3 px tall (centre + halo above/below). Phase keyed off millis +
     * per-mote seed so they don't sync. */
    static const struct mote_t {
        uint8_t  col_pct;
        uint16_t period_ms;
        uint32_t seed;
        bool     alt;          /* true = T_ACCENT2 (magenta) */
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
        uint32_t period = MOTES[i].period_ms;
        uint32_t phase  = (now + MOTES[i].seed) % period;
        int my = (h - 2) - (int)((int64_t)phase * (h + 2) / (int64_t)period);
        int mx = (MOTES[i].col_pct * w) / 100;
        if (my < 0 || my >= h) continue;
        uint16_t color = MOTES[i].alt ? T_ACCENT2 : T_ACCENT;
        d.drawPixel(x + mx, y + my, color);
        if (my > 0)     d.drawPixel(x + mx, y + my - 1, color);
        if (my < h - 1) d.drawPixel(x + mx, y + my + 1, color);
    }

    /* ---- Layer 3: magenta packet hopping the L-path ----
     * 8000 ms cycle, four 25% segments tracing an L through the body
     * region. 3x3 magenta block + 4-pixel cross halo. Slower than the
     * old per-theme TRON packet so it reads as "occasional accent"
     * rather than "constant motion". */
    uint32_t pkt_period = 8000u;
    uint32_t pphase     = now % pkt_period;
    uint32_t quarter    = pkt_period / 4u;
    int      seg        = (int)(pphase / quarter);
    if (seg > 3) seg = 3;
    int      seg_phase  = (int)(pphase - (uint32_t)seg * quarter);
    int      seg_pct    = (int)((int64_t)seg_phase * 100 / (int64_t)quarter);

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
