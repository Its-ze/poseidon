/*
 * theme.cpp — three curated palettes.
 *
 * Persists current id to NVS namespace "pui" key "theme". theme_preview
 * stays an in-RAM-only hot-swap so the picker can browse without
 * thrashing flash with ~300 writes/sec while the user holds an arrow.
 */
#include "theme.h"
#include <M5Cardputer.h>
#include <Preferences.h>

static const poseidon_theme_t THEMES[] = {
    /* ---- POSEIDON CYBERPUNK ----
     * Cyan / magenta / purple on black. T_DIM is medium ice-cyan
     * (0x6BFF) — NOT grey — so hint/footer text stays legible on black
     * while still reading as secondary. Selection: deep cyan-purple
     * fill behind a bright magenta border. */
    {
        "POSEIDON",
        0x0000,             /* bg: pure black */
        0xBFFF,             /* fg: ice cyan-white */
        0x07FF,             /* accent: pure cyan */
        0xF81F,             /* accent2: pure magenta */
        0xFC60,             /* warn: warm orange */
        0xF82A,             /* bad: red-pink */
        0x07F8,             /* good: mint cyan */
        0x6BFF,             /* dim: medium ice cyan (THE readability fix) */
        0x2807,             /* sel_bg: deep cyan-purple */
        0xF81F,             /* sel_border: magenta */
        0x2007,             /* status_bg: dark cyan-purple */
        0x1004,             /* status_bg2: near-black purple */
        0x1004,             /* footer_bg */
        0xC81F,             /* rule: bright magenta-purple */
    },
    /* ---- MATRIX HACKER ----
     * Souped-up phosphor green. Bright leading-character matrix rain
     * runs in the ambient layer at full brightness. Even the "dim"
     * channel here is medium green (0x05A0) instead of grey, so hint
     * text reads in palette. */
    {
        "MATRIX",
        0x0000,             /* bg: pure black */
        0x07E0,             /* fg: bright phosphor green */
        0x07E0,             /* accent: bright green */
        0x9FE0,             /* accent2: lime/yellow-green for splash */
        0xFFE0,             /* warn: yellow (still reads "alert") */
        0xF800,             /* bad: red (high-stakes only) */
        0x07E0,             /* good: bright green */
        0x05A0,             /* dim: medium green (NOT grey) */
        0x0240,             /* sel_bg: very dark green */
        0x07E0,             /* sel_border: bright green */
        0x0220,             /* status_bg: dark green */
        0x0080,             /* status_bg2: near-black green */
        0x0080,             /* footer_bg */
        0x0440,             /* rule */
    },
    /* ---- E-INK PAPER ----
     * Black on white. Designed for direct sunlight and minimal-mode
     * use. Ambient layer is intentionally no-op so the page stays
     * uncluttered. Selection is a light grey highlight with a black
     * border — looks like a paper highlight strip. */
    {
        "E-INK",
        0xFFFF,             /* bg: paper white */
        0x0000,             /* fg: pure black */
        0x0000,             /* accent: black */
        0x4208,             /* accent2: dark grey */
        0x0000,             /* warn: black */
        0x0000,             /* bad: black */
        0x4208,             /* good: dark grey */
        0x8410,             /* dim: medium grey (legible on white due to luminance contrast) */
        0xC618,             /* sel_bg: light grey highlight */
        0x0000,             /* sel_border: black */
        0xDEFB,             /* status_bg: off-white */
        0xC618,             /* status_bg2: light grey */
        0xDEFB,             /* footer_bg */
        0x8410,             /* rule */
    },
};

static theme_id_t s_current = THEME_POSEIDON;
static bool       s_inited  = false;

void theme_init(void)
{
    if (s_inited) return;
    s_inited = true;
    Preferences p;
    if (p.begin("pui", true)) {   /* read-only */
        uint8_t v = p.getUChar("theme", (uint8_t)THEME_POSEIDON);
        p.end();
        if (v >= THEME__COUNT) v = THEME_POSEIDON;
        s_current = (theme_id_t)v;
    }
}

void theme_set(theme_id_t id)
{
    if (id >= THEME__COUNT) id = THEME_POSEIDON;
    s_current = id;
    /* Always write through on commit — s_current may have been shifted
     * around by theme_preview() between the last NVS read and this call,
     * so we can't elide based on the RAM copy. */
    Preferences p;
    if (p.begin("pui", false)) {
        p.putUChar("theme", (uint8_t)id);
        p.end();
    }
}

/* Live-preview helper — changes the in-RAM theme without touching NVS.
 * Used by the theme picker for per-frame swatch rendering and arrow-key
 * browsing, which would otherwise thrash flash at ~300 writes/sec. */
void theme_preview(theme_id_t id)
{
    if (id >= THEME__COUNT) id = THEME_POSEIDON;
    s_current = id;
}

theme_id_t theme_current_id(void) { return s_current; }
const poseidon_theme_t &theme(void) { return THEMES[s_current]; }
