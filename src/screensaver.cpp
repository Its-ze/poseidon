/*
 * screensaver.cpp — three theme-appropriate full-screen idle takeovers.
 *
 * The painter for each theme owns the entire screen for the duration
 * of its run. We don't try to preserve the prior status / footer / menu
 * state — when the screensaver returns, the caller (menu loop) is
 * expected to repaint its UI from scratch.
 *
 * State invariants:
 *   - Per-painter state is function-static so each theme's run picks up
 *     where it left off if the user briefly hit a key and then went idle
 *     again. (For wardrive cinema this means the AP roll-buffer survives
 *     short gaps.)
 *   - No PSRAM, no malloc in the render loop, no IRAM_ATTR.
 *   - Each painter polls input_poll() every frame and returns the moment
 *     a real key arrives.
 */
#include "screensaver.h"
#include "theme.h"
#include "app.h"
#include "ui.h"
#include "input.h"
#include <M5Cardputer.h>
#include <Preferences.h>
#include <esp_random.h>
#include <math.h>
#include <string.h>
#include <stdio.h>

extern void ui_matrix_rain(int x, int y, int w, int h, uint16_t color);

/* ---- NVS-backed config ---- */
static bool     s_enabled        = true;
static uint32_t s_timeout_ms     = 120000;   /* 2 minutes default */
static bool     s_loaded         = false;

static void load_settings(void)
{
    if (s_loaded) return;
    s_loaded = true;
    Preferences p;
    if (p.begin("pscr", true)) {
        s_enabled    = p.getBool ("enabled", true);
        s_timeout_ms = p.getUInt ("timeoutms", 120000);
        p.end();
    }
}

bool screensaver_enabled(void) { load_settings(); return s_enabled; }

void screensaver_enabled_set(bool on)
{
    load_settings();
    s_enabled = on;
    Preferences p;
    if (p.begin("pscr", false)) {
        p.putBool("enabled", on);
        p.end();
    }
}

uint32_t screensaver_timeout_ms(void) { load_settings(); return s_timeout_ms; }

void screensaver_timeout_set(uint32_t ms)
{
    load_settings();
    if (ms < 10000)    ms = 10000;     /* 10 s floor */
    if (ms > 3600000)  ms = 3600000;   /* 1 hr ceiling */
    s_timeout_ms = ms;
    Preferences p;
    if (p.begin("pscr", false)) {
        p.putUInt("timeoutms", ms);
        p.end();
    }
}

/* ============ POSEIDON: WARDRIVE.cinema ============ */

#define WD_AP_SLOTS      9
#define WD_SSID_MAX      19    /* fits 2-px char width budget on screen */
#define WD_SSID_POOL     36

static const char *const WD_SSID_POOL_TBL[WD_SSID_POOL] = {
    "Linksys_2G_Net",       "ATT_Home_5G",          "Verizon_FiOS",
    "TPLink_Guest",         "Xfinity_WiFi",         "Spectrum_AP_88",
    "FBI_Surveillance",     "Pretty_Fly_4_WiFi",    "skynet",
    "Network_Not_Found",    "DEFCON-Open",          "PineappleAP",
    "FreeWiFi_DontUse",     "Apartment_4B",         "Comcast_2.4",
    "GalaxyS22_Hotspot",    "Pixel_Hotspot",        "iPhone_of_dave",
    "myFi",                 "GUEST",                "Starbucks-WPA",
    "McDonalds_Free",       "Marriott_Lobby",       "AirportFree",
    "PrintServer_42",       "WD_NetworkDrive",      "RING_Cam",
    "Ecobee_42BC",          "Lutron_HUB",           "NOT_A_HONEYPOT",
    "vanlife_192_168",      "wifi_was_here",        "tell_my_wifi",
    "mom_use_this",         "FBI_VAN_99",           "iotsec_fail",
};

struct wd_ap_t {
    char     ssid[WD_SSID_MAX + 1];
    uint8_t  bssid[6];
    int8_t   rssi;
    uint8_t  channel;
    uint8_t  auth;        /* 0=WPA3 1=WPA2 2=WPA2/3 3=OPEN 4=WEP 5=PMKID */
    bool     captured;    /* glow red briefly */
    uint32_t born_ms;
};

static wd_ap_t   wd_aps[WD_AP_SLOTS];
static int       wd_count        = 0;
static uint32_t  wd_next_spawn   = 0;
static uint32_t  wd_next_chanhop = 0;
static uint32_t  wd_next_capture = 0;
static uint32_t  wd_next_streak  = 0;
static uint8_t   wd_chan         = 1;
static uint16_t  wd_total_seen   = 0;
static uint32_t  wd_streak_start = 0;
static int       wd_streak_y     = 0;
static int       wd_streak_dir   = 0;

static const char *wd_auth_label(uint8_t a)
{
    switch (a) {
    case 0: return "WPA3";
    case 1: return "WPA2";
    case 2: return "W23";
    case 3: return "OPEN";
    case 4: return "WEP ";
    default: return "M1*";
    }
}

static uint16_t wd_auth_color(uint8_t a)
{
    switch (a) {
    case 0: return T_ACCENT2;        /* WPA3 — magenta */
    case 1: return T_ACCENT;         /* WPA2 — cyan */
    case 2: return T_GOOD;           /* WPA2/3 — mint */
    case 3: return T_WARN;           /* OPEN — orange */
    case 4: return T_BAD;            /* WEP — red */
    default: return T_ACCENT2;       /* PMKID capture — magenta */
    }
}

static void wd_spawn_ap(void)
{
    /* Shift older APs down one slot. */
    for (int i = WD_AP_SLOTS - 1; i > 0; --i) {
        wd_aps[i] = wd_aps[i - 1];
    }
    wd_ap_t &a = wd_aps[0];
    const char *src = WD_SSID_POOL_TBL[esp_random() % WD_SSID_POOL];
    strncpy(a.ssid, src, WD_SSID_MAX);
    a.ssid[WD_SSID_MAX] = 0;
    for (int i = 0; i < 6; ++i) a.bssid[i] = (uint8_t)(esp_random() & 0xFF);
    a.rssi     = -30 - (int8_t)(esp_random() % 60);   /* -30..-89 */
    /* Channel: 80% 2.4G (1..13), 20% 5G subset. */
    if ((esp_random() & 0xFF) < 0xCC) {
        a.channel = 1 + (uint8_t)(esp_random() % 13);
    } else {
        static const uint8_t fiveG[] = { 36, 40, 44, 48, 149, 153, 157, 161 };
        a.channel = fiveG[esp_random() % (sizeof(fiveG))];
    }
    a.auth     = (uint8_t)(esp_random() % 5);   /* 0..4 — capture flag handles 5 */
    a.captured = false;
    a.born_ms  = millis();
    if (wd_count < WD_AP_SLOTS) wd_count++;
    wd_total_seen++;
}

static void wd_render(void)
{
    auto &d = M5Cardputer.Display;
    d.fillScreen(T_BG);

    /* ---- Top status bar ---- */
    d.setTextColor(T_ACCENT, T_BG);
    d.setCursor(2, 2);
    d.print("WARDRIVE.cinema");
    char hud[24];
    snprintf(hud, sizeof(hud), "ch:%-3u APs:%u", wd_chan, wd_total_seen);
    d.setTextColor(T_ACCENT2, T_BG);
    int hw = d.textWidth(hud);
    d.setCursor(SCR_W - hw - 2, 2);
    d.print(hud);
    /* Magenta double-divider matches the menu aesthetic. */
    d.drawFastHLine(0, 11, SCR_W, T_ACCENT2);
    d.drawFastHLine(0, 12, SCR_W, T_ACCENT2);

    /* ---- AP list ---- */
    int row_h = 11;
    for (int i = 0; i < wd_count; ++i) {
        const wd_ap_t &a = wd_aps[i];
        int y = 16 + i * row_h;
        if (y + row_h > SCR_H - 11) break;

        /* SSID — truncate to 14 chars to leave room for BSSID + RSSI + auth. */
        char ssidbuf[15];
        strncpy(ssidbuf, a.ssid, 14);
        ssidbuf[14] = 0;
        d.setTextColor(a.captured ? T_BAD : T_FG, T_BG);
        d.setCursor(2, y);
        d.print(ssidbuf);

        /* RSSI bar — 5 cells, fill proportional to strength. */
        int bx = 100;
        int by = y + 1;
        int strength = a.rssi >= -40 ? 5
                     : a.rssi >= -55 ? 4
                     : a.rssi >= -70 ? 3
                     : a.rssi >= -80 ? 2
                     : a.rssi >= -90 ? 1
                     : 0;
        for (int s = 0; s < 5; ++s) {
            int sx = bx + s * 6;
            uint16_t fc = s < strength ? (a.captured ? T_BAD : T_ACCENT) : T_DIM;
            d.fillRect(sx, by, 4, 7, fc);
        }

        /* RSSI numeric. */
        d.setTextColor(T_DIM, T_BG);
        char rs[6]; snprintf(rs, sizeof(rs), "%-3d", a.rssi);
        d.setCursor(135, y);
        d.print(rs);

        /* Channel. */
        d.setTextColor(T_ACCENT, T_BG);
        char chs[6]; snprintf(chs, sizeof(chs), "%-3u", a.channel);
        d.setCursor(160, y);
        d.print(chs);

        /* Auth chip. */
        const char *al = a.captured ? "PMKID" : wd_auth_label(a.auth);
        uint16_t   ac = a.captured ? T_BAD : wd_auth_color(a.auth);
        d.fillRect(SCR_W - 30, y, 28, 9, T_SEL_BG);
        d.drawRect(SCR_W - 30, y, 28, 9, ac);
        d.setTextColor(ac, T_SEL_BG);
        d.setCursor(SCR_W - 28, y + 1);
        d.print(al);
    }

    /* ---- Magenta packet streak ---- */
    if (wd_streak_start) {
        uint32_t elapsed = millis() - wd_streak_start;
        if (elapsed < 800) {
            int sx = (int)((int64_t)elapsed * (SCR_W + 40) / 800) - 20;
            if (wd_streak_dir < 0) sx = SCR_W - sx;
            d.fillRect(sx - 8, wd_streak_y, 16, 2, T_ACCENT2);
            d.fillRect(sx - 4, wd_streak_y - 1, 8, 4, T_ACCENT2);
        } else {
            wd_streak_start = 0;
        }
    }

    /* ---- Footer ---- */
    d.fillRect(0, SCR_H - 11, SCR_W, 11, T_BG);
    d.drawFastHLine(0, SCR_H - 11, SCR_W, T_ACCENT2);
    d.setTextColor(T_DIM, T_BG);
    d.setCursor(2, SCR_H - 9);
    d.print("// any key to wake");
    d.setTextColor(T_ACCENT, T_BG);
    char up[16];
    uint32_t s = millis() / 1000;
    snprintf(up, sizeof(up), "uptime %lu:%02lu",
             (unsigned long)(s / 60), (unsigned long)(s % 60));
    int upw = d.textWidth(up);
    d.setCursor(SCR_W - upw - 2, SCR_H - 9);
    d.print(up);
}

static void wd_tick(uint32_t now)
{
    if (now >= wd_next_spawn) {
        wd_spawn_ap();
        wd_next_spawn = now + 500 + (esp_random() % 400);
    }
    if (now >= wd_next_chanhop) {
        if ((esp_random() & 0xFF) < 0xCC) {
            wd_chan = 1 + (uint8_t)(esp_random() % 13);
        } else {
            static const uint8_t fg[] = { 36, 40, 44, 48, 149, 153, 157, 161 };
            wd_chan = fg[esp_random() % sizeof(fg)];
        }
        wd_next_chanhop = now + 180 + (esp_random() % 220);
    }
    if (now >= wd_next_capture && wd_count > 0) {
        int idx = esp_random() % wd_count;
        wd_aps[idx].captured = true;
        wd_next_capture = now + 3500 + (esp_random() % 4000);
    }
    /* Clear stale "captured" highlight after ~1.5 s. */
    for (int i = 0; i < wd_count; ++i) {
        if (wd_aps[i].captured && now - wd_aps[i].born_ms > 1500) {
            /* Don't fall through to first-pass clearing — we want the
             * red flash to land regardless of when the AP was spawned.
             * Use a separate per-AP timer if needed; for now just clear
             * once aps age out of the slot. */
        }
    }
    if (now >= wd_next_streak && wd_streak_start == 0) {
        wd_streak_start = now;
        wd_streak_y     = 18 + (int)(esp_random() % (SCR_H - 30));
        wd_streak_dir   = (esp_random() & 1) ? +1 : -1;
        wd_next_streak  = now + 1500 + (esp_random() % 2500);
    }
}

static void run_wardrive_cinema(void)
{
    /* Reset state so each entry feels fresh. (Wardrive is the most
     * complex screensaver; deterministic resets avoid weird carry-over
     * if a previous run was interrupted partway through a streak.) */
    wd_count        = 0;
    wd_total_seen   = 0;
    wd_chan         = 1;
    wd_streak_start = 0;
    wd_next_spawn   = millis();
    wd_next_chanhop = millis();
    wd_next_capture = millis() + 4000;
    wd_next_streak  = millis() + 2000;
    /* Pre-populate one screen worth so it doesn't start empty. */
    for (int i = 0; i < 4; ++i) wd_spawn_ap();

    while (input_poll() == PK_NONE) {
        uint32_t now = millis();
        wd_tick(now);
        wd_render();
        delay(45);   /* ~22 fps, plenty for this aesthetic */
    }
}

/* ============ MATRIX: full-screen rain ============ */

static void run_matrix_rain(void)
{
    auto &d = M5Cardputer.Display;
    d.fillScreen(T_BG);
    while (input_poll() == PK_NONE) {
        ui_matrix_rain(0, 0, SCR_W, SCR_H, T_FG);
        delay(33);
    }
}

/* ============ E-INK: "Breathing" wordmark ============ */

static int eink_drift_x = 0;
static int eink_drift_y = 0;

/* Convert a (white -> black) progress 0..255 to an RGB565 grey-scale
 * value for the E-INK theme so the wordmark fades in and out smoothly
 * even on a 1-bit aesthetic. */
static uint16_t eink_grey(uint8_t t)
{
    uint8_t v = (uint8_t)(255 - t);
    uint8_t r5 = (uint8_t)(v >> 3);
    uint8_t g6 = (uint8_t)(v >> 2);
    uint8_t b5 = (uint8_t)(v >> 3);
    return (uint16_t)((r5 << 11) | (g6 << 5) | b5);
}

static void run_eink_breathing(void)
{
    auto &d = M5Cardputer.Display;
    /* 22 s cycle: 8 s fade-in, 3 s hold, 8 s fade-out, 3 s blank. */
    const uint32_t T_FADE_IN = 8000;
    const uint32_t T_HOLD    = 3000;
    const uint32_t T_FADE_OUT = 8000;
    const uint32_t T_BLANK   = 3000;
    const uint32_t T_CYCLE   = T_FADE_IN + T_HOLD + T_FADE_OUT + T_BLANK;

    uint32_t cycle_start = millis();
    /* Pick an initial drift offset. */
    eink_drift_x = -25 + (int)(esp_random() % 50);
    eink_drift_y = -10 + (int)(esp_random() % 20);

    /* Center "POSEIDON" at size 3 = 18 wide x 24 tall per char. 8 chars
     * = 144 px wide. Center on (120 + drift_x, 67 + drift_y). */
    auto draw_word = [&](uint16_t color) {
        d.fillScreen(T_BG);
        d.setTextSize(3);
        d.setTextColor(color, T_BG);
        int tx = (SCR_W / 2) - 72 + eink_drift_x;
        int ty = (SCR_H / 2) - 12 + eink_drift_y;
        d.setCursor(tx, ty);
        d.print("POSEIDON");
        d.setTextSize(1);

        /* Tide-mark line at the bottom that grows / shrinks with the
         * breath. Width tracks the same fade phase as the text. */
        uint8_t tide = (uint8_t)((255u - ((uint16_t)((color >> 11) & 0x1F) * 8u)) & 0xFFu);
        int tide_w = (int)((tide * (SCR_W - 40)) / 255);
        d.drawFastHLine((SCR_W - tide_w) / 2, SCR_H - 8, tide_w, color);

        /* Footer hint. */
        d.setTextColor(T_DIM, T_BG);
        d.setCursor(2, SCR_H - 9);
        d.print("any key");
    };

    while (input_poll() == PK_NONE) {
        uint32_t now = millis();
        uint32_t phase = (now - cycle_start) % T_CYCLE;
        uint8_t  brightness;   /* 0 = invisible (bg), 255 = full text color */
        if      (phase < T_FADE_IN)               brightness = (uint8_t)((phase * 255u) / T_FADE_IN);
        else if (phase < T_FADE_IN + T_HOLD)      brightness = 255;
        else if (phase < T_FADE_IN + T_HOLD + T_FADE_OUT) {
            uint32_t fp = phase - T_FADE_IN - T_HOLD;
            brightness = (uint8_t)(255u - ((fp * 255u) / T_FADE_OUT));
        } else                                    brightness = 0;

        /* On cycle wrap, drift to a new position. */
        if (phase < 100 && now > cycle_start + T_CYCLE - 100) {
            cycle_start = now;
            eink_drift_x = -25 + (int)(esp_random() % 50);
            eink_drift_y = -10 + (int)(esp_random() % 20);
        }

        draw_word(eink_grey(brightness));
        delay(60);
    }
}

/* ============ dispatcher ============ */

bool screensaver_check_idle(void)
{
    load_settings();
    if (!s_enabled) return false;

    uint32_t last = input_last_input_ms();
    /* If the user hasn't pressed anything since boot the timestamp is
     * 0 — treat that as "device just came up, give them grace". */
    if (last == 0) return false;
    if (millis() - last < s_timeout_ms) return false;

    switch (theme_current_id()) {
    case THEME_MATRIX:    run_matrix_rain();      break;
    case THEME_EINK:      run_eink_breathing();   break;
    case THEME_POSEIDON:  /* fallthrough */
    default:              run_wardrive_cinema();  break;
    }
    return true;
}
