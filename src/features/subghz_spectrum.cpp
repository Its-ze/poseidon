/*
 * subghz_spectrum.cpp — professional spectrum analyzer + waterfall.
 *
 * Three visualization modes with polished rendering:
 *   1. Bar spectrum: gradient bars + peak hold + grid + dBm scale
 *   2. Waterfall: scrolling heatmap spectrogram
 *   3. Waveform: live oscilloscope with grid + trigger level
 */
#include "../app.h"
#include "../theme.h"
#include "../ui.h"
#include "../input.h"
#include "../radio.h"
#include "../cc1101_hw.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <esp_heap_caps.h>

struct freq_range_t { float start; float end; const char *name; };
static const freq_range_t RANGES[] = {
    { 300.0f, 348.0f, "300-348" },
    { 387.0f, 464.0f, "387-464" },
    { 779.0f, 928.0f, "779-928" },
};
#define RANGE_COUNT 3

/* Smooth gradient: deep blue → cyan → green → yellow → red. */
static uint16_t rssi_color(int rssi)
{
    int n = rssi + 110;
    if (n < 0) n = 0;
    if (n > 80) n = 80;
    int p = (n * 255) / 80;
    auto &d = M5Cardputer.Display;
    if (p < 50)  return d.color565(0, 0, 40 + p * 2);
    if (p < 100) return d.color565(0, (p - 50) * 5, 140 - (p - 50) * 2);
    if (p < 170) return d.color565((p - 100) * 3, 255, 0);
    return d.color565(255, 255 - (p - 170) * 3, 0);
}

/* ---- Bar Spectrum ---- */

static void run_bar_spectrum(const freq_range_t &range)
{
    /* Double-buffered render: build the frame in a sprite, blit once.
     * Eliminates the tearing/flicker of the prior fillRect+redraw loop.
     * Plus EMA smoothing on RSSI values so bars don't jitter between
     * reads of the same signal (RSSI is noisy at ~1 dB precision). */
    auto &d = M5Cardputer.Display;
    const int GX = 24, GY = BODY_Y + 14, GW = SCR_W - 30, GH = BODY_H - 30;
    int bins = GW;
    float step = (range.end - range.start) / bins;

    float   smooth[232];
    int8_t  peak[232];
    for (int i = 0; i < bins; ++i) { smooth[i] = -110.0f; peak[i] = -110; }

    M5Canvas canvas(&d);
    canvas.setColorDepth(16);
    bool have_canvas = canvas.createSprite(GW, GH);

    ui_clear_body();
    /* Static chrome — drawn ONCE, not every frame. */
    d.drawRect(GX - 1, GY - 1, GW + 2, GH + 2, 0x4208);
    ui_text(4, BODY_Y + 2, T_ACCENT, "SPECTRUM %s MHz", range.name);
    d.setTextColor(0x4208, T_BG);
    d.setCursor(GX, GY + GH + 3);             d.printf("%.0f", range.start);
    d.setCursor(GX + GW - 24, GY + GH + 3);   d.printf("%.0f", range.end);
    ui_draw_footer("ESC=back  R=reset peaks");

    while (true) {
        for (int i = 0; i < bins; ++i) {
            cc1101_set_freq(range.start + i * step);
            delayMicroseconds(800);
            int raw = cc1101_get_rssi();
            /* EMA: 30% new, 70% old. Fast enough to track bursts,
             * smooth enough to not jitter between idle samples. */
            smooth[i] = smooth[i] * 0.7f + raw * 0.3f;
            int cur = (int)smooth[i];
            if (cur > peak[i]) peak[i] = cur;
        }
        /* Peak decay — slow fall-off after hit. */
        for (int i = 0; i < bins; ++i)
            if (peak[i] > (int)smooth[i] + 1) peak[i]--;

        if (have_canvas) {
            /* All rendering goes into the off-screen canvas. */
            canvas.fillSprite(0x0000);

            /* Grid lines (relative to canvas origin 0,0). */
            for (int db = -100; db <= -40; db += 20) {
                int y = GH - ((db + 110) * GH) / 80;
                if (y >= 0 && y < GH)
                    for (int x = 0; x < GW; x += 4)
                        canvas.drawPixel(x, y, 0x2104);
            }
            /* Bars. */
            for (int i = 0; i < bins; ++i) {
                int norm = (int)smooth[i] + 110;
                if (norm < 0) norm = 0;
                int h = (norm * GH) / 80;
                if (h > 0) {
                    for (int dy = 0; dy < h && dy < GH; ++dy) {
                        int fake_rssi = -110 + ((h - dy) * 80) / GH;
                        canvas.drawPixel(i, GH - 1 - dy, rssi_color(fake_rssi));
                    }
                }
                int pn = peak[i] + 110;
                if (pn > 0) {
                    int py = GH - (pn * GH) / 80;
                    if (py >= 0) canvas.drawPixel(i, py, 0xFFFF);
                }
            }
            canvas.pushSprite(GX, GY);
        } else {
            /* Fallback path if sprite allocation failed — flickers but
             * still works. Matches the old behaviour. */
            d.fillRect(GX, GY, GW, GH, 0x0000);
            for (int db = -100; db <= -40; db += 20) {
                int y = GY + GH - ((db + 110) * GH) / 80;
                for (int x = GX; x < GX + GW; x += 4)
                    d.drawPixel(x, y, 0x2104);
            }
            for (int i = 0; i < bins; ++i) {
                int norm = (int)smooth[i] + 110; if (norm < 0) norm = 0;
                int h = (norm * GH) / 80;
                for (int dy = 0; dy < h; ++dy) {
                    int fake_rssi = -110 + ((h - dy) * 80) / GH;
                    d.drawPixel(GX + i, GY + GH - 1 - dy, rssi_color(fake_rssi));
                }
            }
        }

        /* dB labels drawn outside the sprite (left gutter). */
        for (int db = -100; db <= -40; db += 20) {
            int y = GY + GH - ((db + 110) * GH) / 80;
            d.setTextColor(0x4208, T_BG);
            d.setCursor(1, y - 3); d.printf("%d", db);
        }

        /* Peak freq + RSSI tag — update in place over fixed-width area
         * to avoid flicker from changing text length. */
        int peak_i = 0; float peak_v = -120.0f;
        for (int i = 0; i < bins; ++i)
            if (smooth[i] > peak_v) { peak_v = smooth[i]; peak_i = i; }
        float peak_f = range.start + peak_i * step;
        d.fillRect(GX + GW / 2 - 40, GY + GH + 3, 82, 8, T_BG);
        d.setTextColor(T_FG, T_BG);
        d.setCursor(GX + GW / 2 - 36, GY + GH + 3);
        d.printf("pk:%.1f %ddB", peak_f, (int)peak_v);

        uint16_t k = input_poll();
        if (k == PK_ESC) {
            if (have_canvas) canvas.deleteSprite();
            return;
        }
        if (k == 'r' || k == 'R')
            for (int i = 0; i < bins; ++i) peak[i] = -110;
    }
}

/* ---- Waterfall / Spectrogram ---- */

/* POS-AUDIT-246 / rf-017: ring buffer is now lazy-allocated on entry
 * and freed on exit so the 28.8 KB doesn't sit in BSS for the lifetime
 * of every session — only the user who opens the waterfall pays for
 * it. 60 rows × 240 cols × 2 B = 28800 B; heap_caps_malloc with
 * MALLOC_CAP_INTERNAL forces internal SRAM (we don't have PSRAM and
 * the renderer's pushImage path expects internal-RAM pixels). */
#define WF_MAX_ROWS 60
#define WF_MAX_BINS SCR_W

static void run_waterfall(const freq_range_t &range)
{
    auto &d = M5Cardputer.Display;
    const int GX = 0, GY = 15, GW = WF_MAX_BINS, GH = WF_MAX_ROWS;
    float step = (range.end - range.start) / GW;

    uint16_t *ring = (uint16_t *)heap_caps_malloc(
        WF_MAX_ROWS * WF_MAX_BINS * sizeof(uint16_t), MALLOC_CAP_INTERNAL);
    if (!ring) { ui_toast("OOM", T_BAD, 1000); return; }
    memset(ring, 0, WF_MAX_ROWS * WF_MAX_BINS * sizeof(uint16_t));
    int head = 0, count = 0;

    d.fillScreen(T_BG);

    while (true) {
        /* Sweep one full row of freq bins. */
        uint16_t *row = &ring[head * GW];
        for (int i = 0; i < GW; ++i) {
            cc1101_set_freq(range.start + i * step);
            delayMicroseconds(500);
            row[i] = rssi_color(cc1101_get_rssi());
        }
        head = (head + 1) % GH;
        if (count < GH) count++;

        /* Render full-screen — newest row at bottom, oldest at top. */
        for (int r = 0; r < count; ++r) {
            int ri = (head - count + r + GH) % GH;
            d.pushImage(GX, GY + r, GW, 1, &ring[ri * GW]);
        }

        /* Top title strip + bottom range labels. Waterfall body spans
         * y=15..115, title above and endpoints below. */
        d.fillRect(0, 0, SCR_W, GY, T_BG);
        d.setTextColor(T_ACCENT2, T_BG);
        d.setCursor(4, 3); d.printf("WATERFALL %s", range.name);
        d.setTextColor(T_BAD, T_BG);
        d.setCursor(SCR_W - 22, 3); d.print("ESC");

        d.fillRect(0, GY + GH, SCR_W, SCR_H - (GY + GH), T_BG);
        d.setTextColor(T_DIM, T_BG);
        d.setCursor(4, GY + GH + 4);
        d.printf("%.0f", range.start);
        d.setCursor(SCR_W - 28, GY + GH + 4);
        d.printf("%.0f", range.end);

        uint16_t k = input_poll();
        if (k == PK_ESC) { free(ring); return; }
    }
}

/* ---- Oscilloscope Waveform ---- */

static void run_waveform(float freq)
{
    /* Digital-scope style: top half is RSSI trace (thin line, scrolling
     * right-to-left like a real scope — no filled bars, to differentiate
     * from the spectrum analyzer). Bottom half is a GDO0 pulse train —
     * the actual demodulated data line from the CC1101. That's what
     * shows activity when a car key transmits: a burst of square-wave
     * pulses even though RSSI only nudges. */
    auto &d = M5Cardputer.Display;
    cc1101_set_freq(freq);
    cc1101_set_rx();

    const int GX = 24;
    const int GW = SCR_W - 30;
    const int RSSI_Y = BODY_Y + 14;
    const int RSSI_H = 40;
    const int GDO_Y  = RSSI_Y + RSSI_H + 8;
    const int GDO_H  = 20;

    int8_t  rssi_ring[232];
    uint8_t gdo_ring[232];
    memset(rssi_ring, -110, sizeof(rssi_ring));
    memset(gdo_ring, 0, sizeof(gdo_ring));
    int head = 0;

    ui_clear_body();
    /* Static chrome drawn once — borders + labels outside the canvas. */
    d.drawRect(GX - 1, RSSI_Y - 1, GW + 2, RSSI_H + 2, 0x2945);
    d.drawRect(GX - 1, GDO_Y - 1,  GW + 2, GDO_H + 2,  0x2945);
    d.setTextColor(0x4208, T_BG);
    d.setCursor(1, RSSI_Y - 4);             d.print("-30");
    d.setCursor(1, RSSI_Y + RSSI_H - 6);    d.print("-110");
    d.setCursor(1, GDO_Y + GDO_H / 2 - 3);  d.print("GDO0");
    ui_draw_footer("+-=freq  ESC=back");

    /* Double-buffer both panels into one sprite. Panels are stacked
     * vertically with a 6 px gap — sprite covers rssi_panel + gap + gdo_panel. */
    const int CANVAS_H = RSSI_H + 8 + GDO_H;
    M5Canvas canvas(&d);
    canvas.setColorDepth(16);
    bool have_canvas = canvas.createSprite(GW, CANVAS_H);

    while (true) {
        /* One scroll tick: sample N fresh points, advance head. */
        const int BATCH = 8;
        for (int k = 0; k < BATCH; ++k) {
            /* Read GDO0 a few times inside one pixel-column to catch
             * fast pulses — store 1 if we saw any high in this window. */
            uint8_t seen_high = 0;
            for (int s = 0; s < 8; ++s) {
                if (digitalRead(CC1101_GDO0)) seen_high = 1;
                delayMicroseconds(12);
            }
            rssi_ring[head] = (int8_t)cc1101_get_rssi();
            gdo_ring[head]  = seen_high;
            head = (head + 1) % GW;
        }

        if (have_canvas) {
            canvas.fillSprite(0x0000);

            /* RSSI panel occupies sprite rows 0..RSSI_H-1. */
            int prev_y = RSSI_H - 1;
            for (int i = 0; i < GW; ++i) {
                int idx = (head + i) % GW;
                int n = rssi_ring[idx] + 110; if (n < 0) n = 0; if (n > 80) n = 80;
                int y = RSSI_H - 1 - (n * (RSSI_H - 1)) / 80;
                if (i > 0) canvas.drawLine(i - 1, prev_y, i, y, 0x07E0);
                prev_y = y;
            }
            /* GDO0 panel at sprite rows RSSI_H+8 .. RSSI_H+8+GDO_H-1. */
            const int GDO_Y0 = RSSI_H + 8;
            for (int i = 0; i < GW; ++i) {
                int idx = (head + i) % GW;
                if (gdo_ring[idx])
                    canvas.drawFastVLine(i, GDO_Y0 + 2, GDO_H - 4, 0x07FF);
            }
            canvas.pushSprite(GX, RSSI_Y);
        } else {
            /* Fallback without canvas. */
            d.fillRect(GX, RSSI_Y, GW, RSSI_H, 0x0000);
            d.fillRect(GX, GDO_Y,  GW, GDO_H,  0x0000);
            int prev_y = RSSI_Y + RSSI_H - 1;
            for (int i = 0; i < GW; ++i) {
                int idx = (head + i) % GW;
                int n = rssi_ring[idx] + 110; if (n < 0) n = 0; if (n > 80) n = 80;
                int y = RSSI_Y + RSSI_H - (n * RSSI_H) / 80;
                if (i > 0) d.drawLine(GX + i - 1, prev_y, GX + i, y, 0x07E0);
                prev_y = y;
            }
            for (int i = 0; i < GW; ++i) {
                int idx = (head + i) % GW;
                if (gdo_ring[idx])
                    d.drawFastVLine(GX + i, GDO_Y + 2, GDO_H - 4, 0x07FF);
            }
        }

        /* Header text — draw in fixed-width regions on main display to
         * avoid re-rendering in the sprite. */
        int cur_rssi = rssi_ring[(head + GW - 1) % GW];
        int pulse_count = 0;
        for (int i = 0; i < GW; ++i) pulse_count += gdo_ring[i];

        d.fillRect(4, BODY_Y + 2, 120, 8, T_BG);
        d.setTextColor(T_ACCENT, T_BG);
        d.setCursor(4, BODY_Y + 2); d.printf("SCOPE %.3f MHz", freq);
        d.fillRect(SCR_W - 60, BODY_Y + 2, 58, 8, T_BG);
        d.setTextColor(cur_rssi > -60 ? T_GOOD : T_DIM, T_BG);
        d.setCursor(SCR_W - 54, BODY_Y + 2);
        d.printf("%d dBm", cur_rssi);

        /* Pulse counter — fade from dim to cyan based on activity. */
        d.fillRect(4, SCR_H - 22, 120, 8, T_BG);
        d.setTextColor(pulse_count > 4 ? 0x07FF : T_DIM, T_BG);
        d.setCursor(4, SCR_H - 22);
        d.printf("GDO0 pulses: %3d", pulse_count);

        /* Poll input + cadence. */
        uint32_t t = millis();
        while (millis() - t < 20) {
            uint16_t k = input_poll();
            if (k == PK_ESC) {
                if (have_canvas) canvas.deleteSprite();
                return;
            }
            if (k == '+' || k == '=') { freq += 0.5f; cc1101_set_freq(freq); }
            if (k == '-')             { freq -= 0.5f; cc1101_set_freq(freq); }
        }
    }
}

/* ---- Mode/range picker ---- */

void feat_subghz_spectrum(void)
{
    radio_switch(RADIO_SUBGHZ);
    if (!cc1101_begin(433.0f)) {
        ui_toast("CC1101 not found", T_BAD, 1500);
        radio_switch(RADIO_NONE);
        return;
    }

    auto &d = M5Cardputer.Display;
    int mode = 0, range = 1;
    const char *modes[] = { "Bar Spectrum", "Waterfall", "Oscilloscope" };
    const char *descs[] = {
        "Gradient bars + peak hold + dBm grid",
        "Scrolling color heatmap spectrogram",
        "Live RSSI waveform with filled area"
    };

    while (true) {
        ui_clear_body();
        d.setTextColor(T_ACCENT2, T_BG);
        d.setCursor(4, BODY_Y + 2); d.print("SIGNAL ANALYZER");
        d.drawFastHLine(4, BODY_Y + 12, SCR_W - 8, T_ACCENT2);

        for (int i = 0; i < 3; ++i) {
            int y = BODY_Y + 20 + i * 22;
            bool s = (i == mode);
            if (s) {
                d.fillRoundRect(4, y - 2, SCR_W - 8, 20, 3, 0x18C3);
                d.drawRoundRect(4, y - 2, SCR_W - 8, 20, 3, T_ACCENT);
            }
            d.setTextColor(s ? T_ACCENT : T_FG, s ? 0x18C3 : T_BG);
            d.setCursor(10, y); d.printf("%s", modes[i]);
            d.setTextColor(s ? 0x07FF : T_DIM, s ? 0x18C3 : T_BG);
            d.setCursor(10, y + 10); d.printf("%s", descs[i]);
        }

        d.setTextColor(T_DIM, T_BG);
        d.setCursor(4, BODY_Y + 90);
        d.printf("band: %s MHz  (TAB to change)", RANGES[range].name);
        ui_draw_footer(";/.=mode  TAB=band  ENTER=go  ESC=quit");

        uint16_t k = input_poll();
        if (k == PK_NONE) { delay(20); continue; }
        if (k == PK_ESC) break;
        if (k == ';' || k == PK_UP)   mode = (mode + 2) % 3;
        if (k == '.' || k == PK_DOWN) mode = (mode + 1) % 3;
        if (k == '\t') range = (range + 1) % RANGE_COUNT;
        if (k == PK_ENTER) {
            if (mode == 0) run_bar_spectrum(RANGES[range]);
            else if (mode == 1) run_waterfall(RANGES[range]);
            else run_waveform(RANGES[range].start + (RANGES[range].end - RANGES[range].start) / 2);
            ui_clear_body();
        }
    }

    cc1101_end();
    radio_switch(RADIO_NONE);
}
