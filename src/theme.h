/*
 * theme.h — three curated palettes.
 *
 *   POSEIDON cyberpunk: cyan / magenta / purple on black with strategic
 *                       magenta splashes. The default. Designed for
 *                       readability (T_DIM is medium ice-cyan, not grey).
 *   MATRIX hacker:      bright phosphor green on pure black. Souped-up
 *                       cinematic rain in the ambient layer.
 *   E-INK paper:        black on white. Daytime / outdoor / minimal.
 *                       Ambient is a no-op so the page stays clean.
 *
 * The other earlier palettes (PHANTOM, AMBER, TRON-as-its-own-theme,
 * HI-CONTRAST) got scrapped — better to have three great ones than
 * seven mediocre ones.
 */
#pragma once

#include <Arduino.h>

struct poseidon_theme_t {
    const char *name;
    uint16_t bg;          /* main background */
    uint16_t fg;          /* primary text */
    uint16_t accent;      /* titles, highlights */
    uint16_t accent2;     /* secondary accent (borders, rules) */
    uint16_t warn;        /* warnings */
    uint16_t bad;         /* errors, attacks */
    uint16_t good;        /* success, captures */
    uint16_t dim;         /* muted text, hints */
    uint16_t sel_bg;      /* selected row background */
    uint16_t sel_border;  /* selected row border */
    uint16_t status_bg;   /* status bar gradient top */
    uint16_t status_bg2;  /* status bar gradient bottom */
    uint16_t footer_bg;   /* footer gradient */
    uint16_t rule;        /* divider lines */
};

enum theme_id_t {
    THEME_POSEIDON = 0,   /* cyberpunk cyan/magenta/purple — default */
    THEME_MATRIX,         /* souped-up hacker green-on-black */
    THEME_EINK,           /* paper white, daytime / minimal */
    THEME__COUNT
};

void theme_init(void);               /* load from NVS on boot */
void theme_set(theme_id_t id);       /* apply + persist to NVS */
void theme_preview(theme_id_t id);   /* apply in RAM only — no NVS write */
theme_id_t theme_current_id(void);
const poseidon_theme_t &theme(void);

/* Convenience — replaces COL_* macros. */
#define T_BG       (theme().bg)
#define T_FG       (theme().fg)
#define T_ACCENT   (theme().accent)
#define T_ACCENT2  (theme().accent2)
#define T_WARN     (theme().warn)
#define T_BAD      (theme().bad)
#define T_GOOD     (theme().good)
#define T_DIM      (theme().dim)
#define T_SEL_BG   (theme().sel_bg)
#define T_SEL_BD   (theme().sel_border)
