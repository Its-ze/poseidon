/*
 * theme.cpp — single POSEIDON cyberpunk palette.
 *
 * Cyan / magenta / purple on pure black. T_DIM is medium ice-cyan
 * (not grey) so every "hint" / "muted" string in the UI stays legible
 * while still reading as secondary.
 *
 * Persists current id to NVS namespace "pui" key "theme" — kept for
 * forward-compat in case we add more palettes later. Right now the
 * persistence is effectively a no-op since there's only one valid id.
 */
#include "theme.h"
#include <M5Cardputer.h>
#include <Preferences.h>

static const poseidon_theme_t THEMES[] = {
    /* POSEIDON CYBERPUNK — cyan / magenta / purple on black.
     *
     * Color choices:
     *   bg          0x0000  pure black, max contrast for text
     *   fg          0xBFFF  ice cyan-white — slightly cool, still pops
     *   accent      0x07FF  pure cyan — titles, hotkey letters, primary
     *   accent2     0xF81F  pure magenta — selection borders, alarms
     *   warn        0xFC60  warm orange — distinct from cyan accents
     *   bad         0xF82A  saturated red-pink — disasters
     *   good        0x07F8  mint cyan — fits palette without breaking it
     *   dim         0x6BFF  medium ice cyan — REPLACES grey, this is
     *                       the readability fix that made hints legible
     *   sel_bg      0x2807  deep cyan-purple — selected row fill
     *   sel_border  0xF81F  magenta — selected row outline pops
     *   status_bg   0x2007  dark cyan-purple — status bar top
     *   status_bg2  0x1004  near-black purple — status bar bottom (gradient)
     *   footer_bg   0x1004  matches status_bg2
     *   rule        0xC81F  bright magenta-purple divider lines
     */
    {
        "POSEIDON",
        0x0000,             /* bg */
        0xBFFF,             /* fg */
        0x07FF,             /* accent */
        0xF81F,             /* accent2 */
        0xFC60,             /* warn */
        0xF82A,             /* bad */
        0x07F8,             /* good */
        0x6BFF,             /* dim */
        0x2807,             /* sel_bg */
        0xF81F,             /* sel_border */
        0x2007,             /* status_bg */
        0x1004,             /* status_bg2 */
        0x1004,             /* footer_bg */
        0xC81F,             /* rule */
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
    Preferences p;
    if (p.begin("pui", false)) {
        p.putUChar("theme", (uint8_t)id);
        p.end();
    }
}

theme_id_t theme_current_id(void) { return s_current; }
const poseidon_theme_t &theme(void) { return THEMES[s_current]; }
