/*
 * stubs.cpp — only the About screen now. Everything else is implemented.
 */
#include "app.h"
#include "../theme.h"
#include "ui.h"
#include "input.h"
#include "../version.h"

void feat_about(void)
{
    ui_clear_body();
    auto &d = M5Cardputer.Display;
    d.setTextColor(T_ACCENT, T_BG);
    d.setCursor(4, BODY_Y + 2);
    d.print("POSEIDON");
    d.drawFastHLine(4, BODY_Y + 12, 90, T_ACCENT);
    d.setTextColor(T_FG, T_BG);
    d.setCursor(4, BODY_Y + 22); d.printf("v%s", poseidon_version());
    d.setCursor(4, BODY_Y + 32); d.printf("built %s", poseidon_build_date());
    d.setCursor(4, BODY_Y + 44); d.print("keyboard-first pentesting");
    d.setCursor(4, BODY_Y + 54); d.print("M5Stack Cardputer");
    d.setTextColor(T_DIM, T_BG);
    d.setCursor(4, BODY_Y + 70); d.print("github.com/GeneralDussDuss/poseidon");
    d.setCursor(4, BODY_Y + 80); d.print("commander of the deep");
    ui_draw_footer("`=back");
    while (true) {
        uint16_t k = input_poll();
        if (k == PK_NONE) { delay(20); continue; }
        if (k == PK_ESC) break;
    }
}

void feat_evil_twin(void)
{
    ui_clear_body();
    auto &d = M5Cardputer.Display;
    d.setTextColor(T_WARN, T_BG);
    d.setCursor(4, BODY_Y + 2);
    d.print("EVIL TWIN DISABLED");
    d.drawFastHLine(4, BODY_Y + 12, 132, T_WARN);
    d.setTextColor(T_FG, T_BG);
    d.setCursor(4, BODY_Y + 24);
    d.print("Stability build");
    d.setCursor(4, BODY_Y + 38);
    d.print("Use Portal/AP Clone");
    d.setCursor(4, BODY_Y + 52);
    d.print("separately for now.");
    ui_draw_footer("`=back");
    while (true) {
        uint16_t k = input_poll();
        if (k == PK_NONE) { delay(20); continue; }
        if (k == PK_ESC) break;
    }
}
