/*
 * satcom — satellite tracking subsystem for POSEIDON.
 *
 * Wraps SparkFun_SGP4_Arduino_Library + adds:
 *   - TLE fetch over HTTPS from Celestrak by NORAD catalog number
 *   - SD-cached TLE storage at /poseidon/tle/<norad>.tle
 *   - 24h pass prediction (30-sec step) with AOS/LOS + max elevation
 *   - GPS-derived observer position, system-time UTC
 *
 * Patterned after adammelancon/cardputer-satellite-tracker (MIT) with
 * the data-flow shape preserved; POSEIDON-native UI lives in
 * features/feat_satcom.cpp.
 */
#pragma once

#include <Arduino.h>
#include <stdint.h>

#define SATCOM_TLE_DIR  "/poseidon/tle"

struct satcom_tle_t {
    char     name[33];
    /* 80-byte line buffers match the SparkFun Sgp4 lib's internal
     * storage size. Standard TLE lines are 69 chars + null, but the
     * lib declares its init() params as char[130] and reads up to
     * column 69 — keeping these at 80 gives margin and matches the
     * lib's internal copy size to prevent any future surprise. */
    char     line1[80];
    char     line2[80];
    uint32_t norad;
    uint32_t fetched_ts;  /* epoch seconds of last successful fetch */
};

struct satcom_pos_t {
    double az_deg;          /* azimuth (0=N, 90=E, 180=S, 270=W) */
    double el_deg;          /* elevation (-90..90) */
    double lat_deg;
    double lon_deg;
    double alt_km;          /* sub-satellite altitude */
    double range_km;        /* distance from observer */
    double range_rate_kms;  /* doppler-relevant range rate */
    bool   valid;
};

struct satcom_pass_t {
    uint32_t aos_ts;        /* epoch sec — acquisition of signal */
    uint32_t los_ts;        /* epoch sec — loss of signal */
    double   max_el_deg;
    uint32_t max_el_ts;
};

/* Favorites — preloaded NORAD IDs for one-button targeting. Indexes
 * are stable across versions; SD cache is keyed by NORAD ID. */
struct satcom_favorite_t { uint32_t norad; const char *name; };
extern const satcom_favorite_t SATCOM_FAVORITES[];
extern const int SATCOM_FAVORITES_N;

/* Fetch TLE from Celestrak by NORAD ID. Writes the result to SD and
 * populates `out`. Requires WiFi STA connection. Returns true on
 * success. Cached file at SATCOM_TLE_DIR/<norad>.tle is checked
 * first and used if younger than max_age_sec. */
bool satcom_fetch_tle(uint32_t norad, satcom_tle_t *out, uint32_t max_age_sec = 86400 * 3);

/* Compute current sat position for the given observer. utc_sec is
 * the POSIX epoch seconds at the moment of computation. */
bool satcom_compute(const satcom_tle_t *tle,
                    double obs_lat_deg, double obs_lon_deg, double obs_alt_m,
                    uint32_t utc_sec,
                    satcom_pos_t *out);

/* Predict next N passes over `window_sec` seconds. Returns count
 * written to passes[]. 30-second step with min-elevation filter. */
int satcom_predict_passes(const satcom_tle_t *tle,
                          double obs_lat_deg, double obs_lon_deg, double obs_alt_m,
                          uint32_t start_utc_sec, uint32_t window_sec,
                          double min_el_deg,
                          satcom_pass_t *passes, int max_passes);
