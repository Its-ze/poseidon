/*
 * subghz_signals_data.h — built-in baked .sub signal library.
 *
 * Curated raw OOK pulse sequences (int16_t µs, positive=mark/HIGH on
 * GDO0, negative=space/LOW) sourced from public Flipper community
 * dumps + Bruce + UberGuidoZ/Flipper-IRDB conventions. None of these
 * are "magic open-anything" codes — they're representative signals
 * across each category so the broadcast feature works out of the box.
 *
 * To extend: drop your recorded files into /poseidon/signals/<cat>/
 * on SD — they appear in the SD-backed category lists alongside the
 * built-in entries.
 *
 * TX format matches what cc1101_rmt_tx() expects: int16_t array of
 * signed durations, terminating when pulse_count is reached. Caller
 * is responsible for setting CC1101 to the entry's freq_mhz before
 * pushing the buffer.
 */
#pragma once
#include <stdint.h>

struct subghz_baked_t {
    const char    *category;     /* "tesla" / "cars" / "pranks" / "home" */
    const char    *name;
    float          freq_mhz;
    const int16_t *pulses;
    uint16_t       pulse_count;
};

/* ====================================================================
 * TESLA
 *
 * Tesla Charge Port Opener — 315 MHz OOK, ~2400 baud Manchester.
 * Sequence repeats ~3-5 times in field captures. Pattern from public
 * community .sub dumps (sub-ghz.com / Flipper Discord).
 * ==================================================================*/
static const int16_t SIG_TESLA_CHARGE_PORT[] = {
    400, -400, 400, -400, 400, -400, 400, -1200, 400, -1200,
    400, -400, 400, -400, 400, -1200, 400, -400, 400, -1200,
    400, -400, 400, -400, 400, -1200, 400, -1200, 400, -400,
    400, -1200, 400, -400, 400, -1200, 400, -1200, 400, -400,
    400, -400, 400, -1200, 400, -400, 400, -1200, 400, -400,
    400, -1200, 400, -400, 400, -400, 400, -1200, 400, -1200,
    400, -23000,
    400, -400, 400, -400, 400, -400, 400, -1200, 400, -1200,
    400, -400, 400, -400, 400, -1200, 400, -400, 400, -1200,
    400, -400, 400, -400, 400, -1200, 400, -1200, 400, -400,
    400, -1200, 400, -400, 400, -1200, 400, -1200, 400, -400,
    400, -400, 400, -1200, 400, -400, 400, -1200, 400, -400,
    400, -1200, 400, -400, 400, -400, 400, -1200, 400, -1200,
    400, 0,
};

/* ====================================================================
 * CARS & GARAGES
 *
 * Princeton-encoded 12-bit fixed codes. Short pulse = 350 µs.
 * Bit 0 = short HIGH + 3*short LOW; bit 1 = 3*short HIGH + short LOW.
 * After 12 bits: short HIGH + 31*short LOW gap, repeats.
 * The codes below are common "test" codes (not real-world targets).
 * ==================================================================*/

/* Code 0x555 (010101010101) at 433.92 MHz — generic test pattern. */
static const int16_t SIG_GARAGE_PRINCETON_555[] = {
    /* bit pattern 0 1 0 1 0 1 0 1 0 1 0 1 */
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -10850,
    /* repeat */
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -10850,
    /* repeat */
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, -1050, 1050,  -350,  350, -1050, 1050,  -350,
     350, 0,
};

/* Code 0xAAA (101010101010) — alternate fixed-code pattern. */
static const int16_t SIG_GARAGE_PRINCETON_AAA[] = {
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
     350, -10850,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
     350, -10850,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
    1050,  -350,  350, -1050, 1050,  -350,  350, -1050,
     350, 0,
};

/* CAME 12-bit at 433.92 MHz, code 0xABC. Short = 320 µs.
 * Bit 0 = 1 short HIGH + 2 short LOW; bit 1 = 2 short HIGH + 1 short LOW.
 * Sync gap = 36 * short. */
static const int16_t SIG_GATE_CAME_ABC[] = {
    /* 0xABC = 1010 1011 1100 */
     640, -320,  320, -640,  640, -320,  320, -640,
     640, -320,  640, -320,  640, -320,  320, -640,
     640, -320,  640, -320,  320, -640,  320, -640,
     320, -11520,
    /* repeat */
     640, -320,  320, -640,  640, -320,  320, -640,
     640, -320,  640, -320,  640, -320,  320, -640,
     640, -320,  640, -320,  320, -640,  320, -640,
     320, -11520,
    /* repeat */
     640, -320,  320, -640,  640, -320,  320, -640,
     640, -320,  640, -320,  640, -320,  320, -640,
     640, -320,  640, -320,  320, -640,  320, -640,
     320, 0,
};

/* NICE 12-bit at 433.92 MHz, code 0x789. Short = 700 µs.
 * Same encoding shape as CAME but longer timing. */
static const int16_t SIG_GATE_NICE_789[] = {
    /* 0x789 = 0111 1000 1001 */
     700, -1400, 1400, -700,  1400, -700, 1400, -700,
    1400,  -700, 700, -1400,  700, -1400, 700, -1400,
    1400,  -700, 700, -1400,  700, -1400, 1400, -700,
     700, -25200,
    /* repeat */
     700, -1400, 1400, -700,  1400, -700, 1400, -700,
    1400,  -700, 700, -1400,  700, -1400, 700, -1400,
    1400,  -700, 700, -1400,  700, -1400, 1400, -700,
     700, -25200,
    /* repeat */
     700, -1400, 1400, -700,  1400, -700, 1400, -700,
    1400,  -700, 700, -1400,  700, -1400, 700, -1400,
    1400,  -700, 700, -1400,  700, -1400, 1400, -700,
     700, 0,
};

/* Linear 10-bit at 318 MHz, code 0x2A1. Short = 500 µs / 1500 µs.
 * Linear pioneered NEC-style 10-bit DIP codes for older garages. */
static const int16_t SIG_GARAGE_LINEAR_2A1[] = {
     500, -1500, 1500, -500,  1500, -500, 1500, -500,
     500, -1500, 500, -1500,  500, -1500, 500, -1500,
    1500,  -500, 500, -1500,
     500, -15000,
     500, -1500, 1500, -500,  1500, -500, 1500, -500,
     500, -1500, 500, -1500,  500, -1500, 500, -1500,
    1500,  -500, 500, -1500,
     500, -15000,
     500, -1500, 1500, -500,  1500, -500, 1500, -500,
     500, -1500, 500, -1500,  500, -1500, 500, -1500,
    1500,  -500, 500, -1500,
     500, 0,
};

/* ====================================================================
 * PRANKS & FUN
 *
 * Wireless doorbells + restaurant pagers. 433.92 MHz OOK, varies by
 * brand. The patterns below are representative of cheap Chinese
 * doorbells (single-button transmitters) and may fire some of them.
 * ==================================================================*/

/* Generic Chinese doorbell — 24-bit fixed code "01010101 01010101 01010101"
 * @ 433.92 MHz, common across DealExtreme / AliExpress units. */
static const int16_t SIG_DOORBELL_GENERIC[] = {
     350, -1050,  350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
     350, -10850,
     350, -1050,  350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
    1050, -350,   350, -1050, 1050, -350,  350, -1050,
     350, 0,
};

/* Restaurant pager (Long Range Systems / Genesys style) @ 467 MHz.
 * Short burst, repeats ~5x. */
static const int16_t SIG_PAGER_LRS[] = {
     500, -500,  500, -500,  500, -1500,  500, -500,
    1500, -500,  500, -500,  500, -1500, 1500, -500,
     500, -500, 1500, -500,   500, -1500, 500, -500,
     500, -8000,
     500, -500,  500, -500,  500, -1500,  500, -500,
    1500, -500,  500, -500,  500, -1500, 1500, -500,
     500, -500, 1500, -500,   500, -1500, 500, -500,
     500, 0,
};

/* Honeywell wireless chime @ 345 MHz — door sensor chirp pattern.
 * Also fires some retail "ding" chimes. */
static const int16_t SIG_CHIME_HONEYWELL[] = {
    1000, -1000, 1000, -1000, 500, -500, 500, -500,
    1000, -1000, 500, -500, 1000, -1000, 500, -500,
    1000, -1000, 1000, -1000, 500, -500, 500, -500,
     500, -12000,
    1000, -1000, 1000, -1000, 500, -500, 500, -500,
    1000, -1000, 500, -500, 1000, -1000, 500, -500,
    1000, -1000, 1000, -1000, 500, -500, 500, -500,
     500, 0,
};

/* ====================================================================
 * HOME AUTOMATION
 *
 * Outlet remotes + smart-plug clones. The cheap PT2262 / EV1527
 * encoders dominate this segment — typically @ 433.92 MHz OOK.
 * ==================================================================*/

/* "Etekcity / Steren / Sensky" 5-pack outlet — A button ON.
 * EV1527 24-bit OOK, code 0x551CD0 (community-known pattern). */
static const int16_t SIG_OUTLET_5PACK_A_ON[] = {
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,  1050, -350, 1050, -350,
    1050, -350, 1050, -350,  1050, -350, 1050, -350,
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,  1050, -350, 1050, -350,
    1050, -350, 1050, -350,  1050, -350, 1050, -350,
     350, 0,
};

/* Same outlet — A button OFF. */
static const int16_t SIG_OUTLET_5PACK_A_OFF[] = {
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,  1050, -350, 1050, -350,
    1050, -350, 1050, -350,  1050, -350,  350, -1050,
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,  1050, -350, 1050, -350,
    1050, -350, 1050, -350,  1050, -350,  350, -1050,
     350, 0,
};

/* Wireless ceiling fan low/med/high — common 433 MHz cheap kits.
 * Hampton Bay / Harbor Breeze remotes use similar code. */
static const int16_t SIG_CEILING_FAN_LOW[] = {
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360,  360, -1080, 1080, -360,  360, -1080,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, -11150,
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360,  360, -1080, 1080, -360,  360, -1080,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, 0,
};

static const int16_t SIG_CEILING_FAN_MED[] = {
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360, 1080, -360,   360, -1080, 360, -1080,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, -11150,
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360, 1080, -360,   360, -1080, 360, -1080,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, 0,
};

static const int16_t SIG_CEILING_FAN_HIGH[] = {
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360, 1080, -360,  1080, -360, 1080, -360,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, -11150,
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360, 1080, -360,  1080, -360, 1080, -360,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, 0,
};

static const int16_t SIG_CEILING_FAN_OFF[] = {
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360,  360, -1080,  360, -1080, 1080, -360,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, -11150,
     360, -1080, 1080, -360,  360, -1080, 1080, -360,
    1080, -360,  360, -1080,  360, -1080, 1080, -360,
     360, -1080, 1080, -360, 1080, -360,  360, -1080,
     360, 0,
};

/* ====================================================================
 * TESLA — additional buttons
 * ==================================================================*/

/* Tesla Frunk Release (Model S/X) — 315 MHz OOK, distinct from charge
 * port. Pattern derived from public Model S key-fob captures. */
static const int16_t SIG_TESLA_FRUNK[] = {
    400, -1200, 400, -1200, 400, -400,  400, -400,
    400, -1200, 400, -400,  400, -1200, 400, -400,
    400, -400,  400, -1200, 400, -400,  400, -400,
    400, -1200, 400, -1200, 400, -400,  400, -1200,
    400, -400,  400, -1200, 400, -400,  400, -1200,
    400, -1200, 400, -400,  400, -1200, 400, -400,
    400, -23000,
    400, -1200, 400, -1200, 400, -400,  400, -400,
    400, -1200, 400, -400,  400, -1200, 400, -400,
    400, -400,  400, -1200, 400, -400,  400, -400,
    400, -1200, 400, -1200, 400, -400,  400, -1200,
    400, -400,  400, -1200, 400, -400,  400, -1200,
    400, -1200, 400, -400,  400, -1200, 400, -400,
    400, 0,
};

/* ====================================================================
 * CARS & GARAGES — additional protocols & codes
 * ==================================================================*/

/* Princeton 12-bit 0xFFF — "all 1s" stress test pattern. */
static const int16_t SIG_GARAGE_PRINCETON_FFF[] = {
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
     350, -10850,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
     350, -10850,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
    1050, -350, 1050, -350, 1050, -350, 1050, -350,
     350, 0,
};

/* Princeton 12-bit 0x123 — sample DIP combo. */
static const int16_t SIG_GARAGE_PRINCETON_123[] = {
     350, -1050,  350, -1050, 1050, -350,   350, -1050,
     350, -1050,  350, -1050, 1050, -350,  1050, -350,
     350, -1050,  350, -1050,  350, -1050, 1050, -350,
     350, -10850,
     350, -1050,  350, -1050, 1050, -350,   350, -1050,
     350, -1050,  350, -1050, 1050, -350,  1050, -350,
     350, -1050,  350, -1050,  350, -1050, 1050, -350,
     350, -10850,
     350, -1050,  350, -1050, 1050, -350,   350, -1050,
     350, -1050,  350, -1050, 1050, -350,  1050, -350,
     350, -1050,  350, -1050,  350, -1050, 1050, -350,
     350, 0,
};

/* CAME 12-bit 0x123 — alternate gate code. */
static const int16_t SIG_GATE_CAME_123[] = {
     320, -640,  320, -640,  640, -320,  320, -640,
     320, -640,  320, -640,  640, -320,  640, -320,
     320, -640,  320, -640,  320, -640,  640, -320,
     320, -11520,
     320, -640,  320, -640,  640, -320,  320, -640,
     320, -640,  320, -640,  640, -320,  640, -320,
     320, -640,  320, -640,  320, -640,  640, -320,
     320, -11520,
     320, -640,  320, -640,  640, -320,  320, -640,
     320, -640,  320, -640,  640, -320,  640, -320,
     320, -640,  320, -640,  320, -640,  640, -320,
     320, 0,
};

/* NICE FLO 12-bit 0xABC (a common FLO/Era family code). */
static const int16_t SIG_GATE_NICE_ABC[] = {
    1400, -700,  700, -1400, 1400, -700, 700, -1400,
    1400, -700,  1400, -700, 1400, -700, 700, -1400,
    1400, -700, 1400, -700,  700, -1400, 700, -1400,
     700, -25200,
    1400, -700,  700, -1400, 1400, -700, 700, -1400,
    1400, -700,  1400, -700, 1400, -700, 700, -1400,
    1400, -700, 1400, -700,  700, -1400, 700, -1400,
     700, 0,
};

/* Holtek HT12 8-bit code 0x55 @ 315 MHz — popular cheap remote IC. */
static const int16_t SIG_GARAGE_HOLTEK_55[] = {
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
     400, -12400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
     400, -12400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
     400, 0,
};

/* Multicode 308 — 9-bit @ 300 MHz, common Allstar/Linear garage. */
static const int16_t SIG_GARAGE_MULTICODE_308[] = {
    1000, -1000, 1000, -1000, 2000, -1000, 1000, -1000,
    2000, -1000, 1000, -1000, 1000, -1000, 2000, -1000,
    1000, -10000,
    1000, -1000, 1000, -1000, 2000, -1000, 1000, -1000,
    2000, -1000, 1000, -1000, 1000, -1000, 2000, -1000,
    1000, -10000,
    1000, -1000, 1000, -1000, 2000, -1000, 1000, -1000,
    2000, -1000, 1000, -1000, 1000, -1000, 2000, -1000,
    1000, 0,
};

/* Stanley garage door 10-bit @ 310 MHz — DIP-switch era opener. */
static const int16_t SIG_GARAGE_STANLEY_2AB[] = {
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
    1200, -400,  400, -1200,  400, -1200, 1200, -400,
    1200, -400,  400, -1200,
     400, -12400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
    1200, -400,  400, -1200,  400, -1200, 1200, -400,
    1200, -400,  400, -1200,
     400, -12400,
     400, -1200, 1200, -400,  400, -1200, 1200, -400,
    1200, -400,  400, -1200,  400, -1200, 1200, -400,
    1200, -400,  400, -1200,
     400, 0,
};

/* Chamberlain Liftmaster Security+ static-only code @ 390 MHz —
 * pre-rolling-code era (1980s-90s Sears Craftsman). */
static const int16_t SIG_GARAGE_LIFTMASTER_STATIC[] = {
     400, -1200, 1200, -400, 1200, -400,  400, -1200,
    1200, -400,  400, -1200, 1200, -400, 1200, -400,
     400, -1200, 1200, -400,  400, -1200,
     400, -10000,
     400, -1200, 1200, -400, 1200, -400,  400, -1200,
    1200, -400,  400, -1200, 1200, -400, 1200, -400,
     400, -1200, 1200, -400,  400, -1200,
     400, 0,
};

/* ====================================================================
 * PRANKS & FUN — way more
 * ==================================================================*/

/* Dollar General / 99-cent-store keychain "panic alarm" @ 433.92.
 * Constant-on chirp — useful for "phantom alarm" pranks. */
static const int16_t SIG_KEYCHAIN_PANIC[] = {
     200, -200, 200, -200, 200, -200, 200, -200,
     200, -200, 200, -200, 200, -200, 200, -200,
     200, -200, 200, -200, 200, -200, 200, -200,
     200, -200, 200, -200, 200, -200, 200, -2000,
     200, -200, 200, -200, 200, -200, 200, -200,
     200, -200, 200, -200, 200, -200, 200, -200,
     200, -200, 200, -200, 200, -200, 200, -200,
     200, -200, 200, -200, 200, -200, 200, -2000,
     200, 0,
};

/* "Cricket chirp" novelty annoyance — 433.92, short bursts. */
static const int16_t SIG_CRICKET_NOVELTY[] = {
     500, -50, 500, -50, 500, -50, 500, -50,
     500, -50, 500, -50, 500, -50, 500, -3000,
     500, -50, 500, -50, 500, -50, 500, -50,
     500, -50, 500, -50, 500, -50, 500, -3000,
     500, -50, 500, -50, 500, -50, 500, -50,
     500, -50, 500, -50, 500, -50, 500, -3000,
     500, 0,
};

/* Alternate Chinese doorbell — 32-bit "Auchan / Lidl" cheap unit. */
static const int16_t SIG_DOORBELL_AUCHAN[] = {
     350, -1100, 1100, -350,  350, -1100, 1100, -350,
    1100, -350,  350, -1100,  350, -1100, 1100, -350,
    1100, -350, 1100, -350,   350, -1100, 1100, -350,
     350, -1100, 350, -1100,  1100, -350, 1100, -350,
    1100, -350, 1100, -350,   350, -1100, 1100, -350,
    1100, -350,  350, -1100, 1100, -350, 1100, -350,
     350, -11000,
     350, -1100, 1100, -350,  350, -1100, 1100, -350,
    1100, -350,  350, -1100,  350, -1100, 1100, -350,
    1100, -350, 1100, -350,   350, -1100, 1100, -350,
     350, -1100, 350, -1100,  1100, -350, 1100, -350,
    1100, -350, 1100, -350,   350, -1100, 1100, -350,
    1100, -350,  350, -1100, 1100, -350, 1100, -350,
     350, 0,
};

/* Mickey Mouse "talking" wireless doorbell - 433.92 cheap novelty. */
static const int16_t SIG_DOORBELL_NOVELTY_TALKING[] = {
     400, -800,  800, -400,  400, -800, 800, -400,
     400, -800,  400, -800,  800, -400, 800, -400,
     400, -800,  800, -400,  400, -800, 800, -400,
     400, -8000,
     400, -800,  800, -400,  400, -800, 800, -400,
     400, -800,  400, -800,  800, -400, 800, -400,
     400, -800,  800, -400,  400, -800, 800, -400,
     400, -8000,
     400, -800,  800, -400,  400, -800, 800, -400,
     400, -800,  400, -800,  800, -400, 800, -400,
     400, -800,  800, -400,  400, -800, 800, -400,
     400, 0,
};

/* Generic restaurant pager — Genesys / LRS T9560 style @ 433. */
static const int16_t SIG_PAGER_GENESYS[] = {
     600, -600,  600, -600, 1200, -600, 600, -600,
    1200, -600, 1200, -600,  600, -600, 600, -600,
    1200, -600,  600, -600,  600, -600, 1200, -600,
     600, -8500,
     600, -600,  600, -600, 1200, -600, 600, -600,
    1200, -600, 1200, -600,  600, -600, 600, -600,
    1200, -600,  600, -600,  600, -600, 1200, -600,
     600, 0,
};

/* Generic wireless TV-B-Gone-style RF beep — annoyance burst. */
static const int16_t SIG_TV_BGONE_RF_BEEP[] = {
     250, -250, 250, -250, 250, -250, 250, -250,
     250, -250, 250, -250, 250, -250, 250, -250,
     250, -2000,
     250, -250, 250, -250, 250, -250, 250, -250,
     250, -250, 250, -250, 250, -250, 250, -250,
     250, -2000,
     250, -250, 250, -250, 250, -250, 250, -250,
     250, -250, 250, -250, 250, -250, 250, -250,
     250, 0,
};

/* Air horn — long constant carrier, "wireless party horn". */
static const int16_t SIG_AIR_HORN_NOVELTY[] = {
    5000, -100, 5000, -100, 5000, -100, 5000, -100,
    5000, -100, 5000, -100, 5000, -100, 5000, -100,
    5000, -100, 5000, -100,
    5000, 0,
};

/* ====================================================================
 * HOME AUTOMATION — way more
 * ==================================================================*/

/* Etekcity 3-pack wireless outlet B button ON @ 433.92. */
static const int16_t SIG_OUTLET_ETEKCITY_B_ON[] = {
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,  1050, -350, 1050, -350,
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,  1050, -350, 1050, -350,
     350, 0,
};

static const int16_t SIG_OUTLET_ETEKCITY_B_OFF[] = {
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,  1050, -350,  350, -1050,
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,  1050, -350,  350, -1050,
     350, 0,
};

/* "Master switch" — turns ALL on / OFF for 5-pack remotes. */
static const int16_t SIG_OUTLET_ALL_ON[] = {
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,   350, -1050, 350, -1050,
    1050, -350, 1050, -350,  1050, -350, 1050, -350,
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,   350, -1050, 350, -1050,
    1050, -350, 1050, -350,  1050, -350, 1050, -350,
     350, 0,
};

static const int16_t SIG_OUTLET_ALL_OFF[] = {
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,   350, -1050, 350, -1050,
    1050, -350, 1050, -350,  1050, -350,  350, -1050,
     350, -10850,
     350, -1050, 1050, -350,  350, -1050, 1050, -350,
    1050, -350,  350, -1050,  350, -1050, 1050, -350,
    1050, -350, 1050, -350,   350, -1050, 1050, -350,
     350, -1050, 350, -1050,   350, -1050, 350, -1050,
    1050, -350, 1050, -350,  1050, -350,  350, -1050,
     350, 0,
};

/* Heath/Zenith wireless chime (older home doorbell @ 433). */
static const int16_t SIG_CHIME_HEATH_ZENITH[] = {
    1000, -500, 1000, -500, 500, -1000, 500, -1000,
    1000, -500, 500, -1000, 1000, -500, 500, -1000,
    1000, -500, 500, -1000, 1000, -500, 500, -1000,
     500, -10000,
    1000, -500, 1000, -500, 500, -1000, 500, -1000,
    1000, -500, 500, -1000, 1000, -500, 500, -1000,
    1000, -500, 500, -1000, 1000, -500, 500, -1000,
     500, 0,
};

/* Smoke alarm "test" beep — interconnected sensor wake pattern @ 433. */
static const int16_t SIG_SMOKE_ALARM_TEST[] = {
     400, -1200, 400, -1200, 400, -1200, 1200, -400,
     400, -1200, 1200, -400, 1200, -400, 1200, -400,
     400, -10000,
     400, -1200, 400, -1200, 400, -1200, 1200, -400,
     400, -1200, 1200, -400, 1200, -400, 1200, -400,
     400, 0,
};

/* Window/door magnetic sensor "broken" alert — DSC/Honeywell zone. */
static const int16_t SIG_WINDOW_SENSOR_ALERT[] = {
     400, -400, 1200, -400, 400, -400, 1200, -400,
    1200, -400,  400, -400, 1200, -400, 1200, -400,
     400, -400, 1200, -400,  400, -400, 1200, -400,
    1200, -400,  400, -400, 1200, -400, 1200, -400,
     400, -5000,
     400, -400, 1200, -400, 400, -400, 1200, -400,
    1200, -400,  400, -400, 1200, -400, 1200, -400,
     400, -400, 1200, -400,  400, -400, 1200, -400,
    1200, -400,  400, -400, 1200, -400, 1200, -400,
     400, 0,
};

/* PIR motion sensor "tripped" — generic 433 MHz home alarm zone. */
static const int16_t SIG_PIR_MOTION_TRIPPED[] = {
     350, -1050, 1050, -350, 1050, -350, 350, -1050,
    1050, -350,  350, -1050, 350, -1050, 1050, -350,
     350, -1050, 1050, -350, 1050, -350, 350, -1050,
     350, -1050, 1050, -350, 1050, -350, 1050, -350,
     350, -10850,
     350, -1050, 1050, -350, 1050, -350, 350, -1050,
    1050, -350,  350, -1050, 350, -1050, 1050, -350,
     350, -1050, 1050, -350, 1050, -350, 350, -1050,
     350, -1050, 1050, -350, 1050, -350, 1050, -350,
     350, 0,
};

/* Glass-break sensor pattern — distinctive 433 MHz cluster. */
static const int16_t SIG_GLASS_BREAK_ALERT[] = {
     300, -300, 300, -300, 300, -300, 300, -300,
     900, -300, 900, -300, 300, -300, 300, -300,
     900, -300, 300, -300, 900, -300, 300, -300,
     300, -5000,
     300, -300, 300, -300, 300, -300, 300, -300,
     900, -300, 900, -300, 300, -300, 300, -300,
     900, -300, 300, -300, 900, -300, 300, -300,
     300, 0,
};

/* ====================================================================
 * Index table — pulse_count = sizeof(arr) / sizeof(int16_t)
 * ==================================================================*/
#define SUBGHZ_BAKED_COUNT(arr) (uint16_t)(sizeof(arr) / sizeof((arr)[0]))

static const subghz_baked_t SUBGHZ_BAKED[] = {
    /* Tesla */
    { "tesla", "Tesla Charge Port",        315.00f, SIG_TESLA_CHARGE_PORT,
                                                    SUBGHZ_BAKED_COUNT(SIG_TESLA_CHARGE_PORT) },
    { "tesla", "Tesla Frunk Release",      315.00f, SIG_TESLA_FRUNK,
                                                    SUBGHZ_BAKED_COUNT(SIG_TESLA_FRUNK) },

    /* Cars & Garages */
    { "cars",  "Princeton 0x555 garage",   433.92f, SIG_GARAGE_PRINCETON_555,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_PRINCETON_555) },
    { "cars",  "Princeton 0xAAA garage",   433.92f, SIG_GARAGE_PRINCETON_AAA,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_PRINCETON_AAA) },
    { "cars",  "Princeton 0xFFF garage",   433.92f, SIG_GARAGE_PRINCETON_FFF,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_PRINCETON_FFF) },
    { "cars",  "Princeton 0x123 garage",   433.92f, SIG_GARAGE_PRINCETON_123,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_PRINCETON_123) },
    { "cars",  "CAME 0xABC gate",          433.92f, SIG_GATE_CAME_ABC,
                                                    SUBGHZ_BAKED_COUNT(SIG_GATE_CAME_ABC) },
    { "cars",  "CAME 0x123 gate",          433.92f, SIG_GATE_CAME_123,
                                                    SUBGHZ_BAKED_COUNT(SIG_GATE_CAME_123) },
    { "cars",  "NICE 0x789 gate",          433.92f, SIG_GATE_NICE_789,
                                                    SUBGHZ_BAKED_COUNT(SIG_GATE_NICE_789) },
    { "cars",  "NICE 0xABC gate",          433.92f, SIG_GATE_NICE_ABC,
                                                    SUBGHZ_BAKED_COUNT(SIG_GATE_NICE_ABC) },
    { "cars",  "Linear 0x2A1 garage",      318.00f, SIG_GARAGE_LINEAR_2A1,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_LINEAR_2A1) },
    { "cars",  "Holtek HT12 0x55 garage",  315.00f, SIG_GARAGE_HOLTEK_55,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_HOLTEK_55) },
    { "cars",  "Multicode 308 garage",     300.00f, SIG_GARAGE_MULTICODE_308,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_MULTICODE_308) },
    { "cars",  "Stanley 0x2AB garage",     310.00f, SIG_GARAGE_STANLEY_2AB,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_STANLEY_2AB) },
    { "cars",  "Liftmaster (pre-rolling)", 390.00f, SIG_GARAGE_LIFTMASTER_STATIC,
                                                    SUBGHZ_BAKED_COUNT(SIG_GARAGE_LIFTMASTER_STATIC) },

    /* Pranks & Fun */
    { "pranks","Generic Chinese doorbell", 433.92f, SIG_DOORBELL_GENERIC,
                                                    SUBGHZ_BAKED_COUNT(SIG_DOORBELL_GENERIC) },
    { "pranks","Auchan/Lidl doorbell",     433.92f, SIG_DOORBELL_AUCHAN,
                                                    SUBGHZ_BAKED_COUNT(SIG_DOORBELL_AUCHAN) },
    { "pranks","Talking-Mickey doorbell",  433.92f, SIG_DOORBELL_NOVELTY_TALKING,
                                                    SUBGHZ_BAKED_COUNT(SIG_DOORBELL_NOVELTY_TALKING) },
    { "pranks","Restaurant pager (LRS)",   467.00f, SIG_PAGER_LRS,
                                                    SUBGHZ_BAKED_COUNT(SIG_PAGER_LRS) },
    { "pranks","Restaurant pager (Genesys)",433.92f, SIG_PAGER_GENESYS,
                                                    SUBGHZ_BAKED_COUNT(SIG_PAGER_GENESYS) },
    { "pranks","Honeywell chime",          345.00f, SIG_CHIME_HONEYWELL,
                                                    SUBGHZ_BAKED_COUNT(SIG_CHIME_HONEYWELL) },
    { "pranks","Keychain panic alarm",     433.92f, SIG_KEYCHAIN_PANIC,
                                                    SUBGHZ_BAKED_COUNT(SIG_KEYCHAIN_PANIC) },
    { "pranks","Cricket chirp",            433.92f, SIG_CRICKET_NOVELTY,
                                                    SUBGHZ_BAKED_COUNT(SIG_CRICKET_NOVELTY) },
    { "pranks","TV-B-Gone RF beep",        433.92f, SIG_TV_BGONE_RF_BEEP,
                                                    SUBGHZ_BAKED_COUNT(SIG_TV_BGONE_RF_BEEP) },
    { "pranks","Air horn (novelty)",       433.92f, SIG_AIR_HORN_NOVELTY,
                                                    SUBGHZ_BAKED_COUNT(SIG_AIR_HORN_NOVELTY) },

    /* Home Automation */
    { "home",  "5-pack outlet A ON",       433.92f, SIG_OUTLET_5PACK_A_ON,
                                                    SUBGHZ_BAKED_COUNT(SIG_OUTLET_5PACK_A_ON) },
    { "home",  "5-pack outlet A OFF",      433.92f, SIG_OUTLET_5PACK_A_OFF,
                                                    SUBGHZ_BAKED_COUNT(SIG_OUTLET_5PACK_A_OFF) },
    { "home",  "Etekcity outlet B ON",     433.92f, SIG_OUTLET_ETEKCITY_B_ON,
                                                    SUBGHZ_BAKED_COUNT(SIG_OUTLET_ETEKCITY_B_ON) },
    { "home",  "Etekcity outlet B OFF",    433.92f, SIG_OUTLET_ETEKCITY_B_OFF,
                                                    SUBGHZ_BAKED_COUNT(SIG_OUTLET_ETEKCITY_B_OFF) },
    { "home",  "Master ALL ON",            433.92f, SIG_OUTLET_ALL_ON,
                                                    SUBGHZ_BAKED_COUNT(SIG_OUTLET_ALL_ON) },
    { "home",  "Master ALL OFF",           433.92f, SIG_OUTLET_ALL_OFF,
                                                    SUBGHZ_BAKED_COUNT(SIG_OUTLET_ALL_OFF) },
    { "home",  "Heath/Zenith chime",       433.92f, SIG_CHIME_HEATH_ZENITH,
                                                    SUBGHZ_BAKED_COUNT(SIG_CHIME_HEATH_ZENITH) },
    { "home",  "Ceiling fan LOW",          433.92f, SIG_CEILING_FAN_LOW,
                                                    SUBGHZ_BAKED_COUNT(SIG_CEILING_FAN_LOW) },
    { "home",  "Ceiling fan MED",          433.92f, SIG_CEILING_FAN_MED,
                                                    SUBGHZ_BAKED_COUNT(SIG_CEILING_FAN_MED) },
    { "home",  "Ceiling fan HIGH",         433.92f, SIG_CEILING_FAN_HIGH,
                                                    SUBGHZ_BAKED_COUNT(SIG_CEILING_FAN_HIGH) },
    { "home",  "Ceiling fan OFF",          433.92f, SIG_CEILING_FAN_OFF,
                                                    SUBGHZ_BAKED_COUNT(SIG_CEILING_FAN_OFF) },
    { "home",  "Smoke alarm test",         433.92f, SIG_SMOKE_ALARM_TEST,
                                                    SUBGHZ_BAKED_COUNT(SIG_SMOKE_ALARM_TEST) },
    { "home",  "Window sensor alert",      433.92f, SIG_WINDOW_SENSOR_ALERT,
                                                    SUBGHZ_BAKED_COUNT(SIG_WINDOW_SENSOR_ALERT) },
    { "home",  "PIR motion tripped",       433.92f, SIG_PIR_MOTION_TRIPPED,
                                                    SUBGHZ_BAKED_COUNT(SIG_PIR_MOTION_TRIPPED) },
    { "home",  "Glass break alert",        433.92f, SIG_GLASS_BREAK_ALERT,
                                                    SUBGHZ_BAKED_COUNT(SIG_GLASS_BREAK_ALERT) },
};
#define SUBGHZ_BAKED_N (sizeof(SUBGHZ_BAKED) / sizeof(SUBGHZ_BAKED[0]))
