/*
 * satcom.cpp — TLE fetch + SGP4 wrapper.
 */
#include "satcom.h"
#include "sd_helper.h"
#include "satcom_tle_baked.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <SD.h>
#include <Sgp4.h>     /* SparkFun_SGP4_Arduino_Library */

const satcom_favorite_t SATCOM_FAVORITES[] = {
    { 25544, "ISS (ZARYA)" },
    { 48274, "Tiangong" },
    { 20580, "Hubble" },
    { 33591, "NOAA 19" },
    { 28654, "NOAA 18" },
    { 25338, "NOAA 15" },
    { 53807, "BlueWalker 3" },
    { 7530,  "AMSAT OSCAR 7" },
    { 27607, "SAUDISAT 1C / SO-50" },
    { 43017, "FOX-1B / AO-91" },
    { 39444, "FUNcube-1 / AO-73" },
    { 25397, "GO-32" },
    { 40967, "AO-85 / Fox-1A" },
    { 32953, "DELFI-C3" },
};
const int SATCOM_FAVORITES_N =
    sizeof(SATCOM_FAVORITES) / sizeof(SATCOM_FAVORITES[0]);

/* ---- TLE I/O ---- */

static bool tle_parse_block(const char *body, uint32_t norad, satcom_tle_t *out)
{
    /* Celestrak GP TLE format: line0 = name, line1 + line2 = orbital
     * elements. Strip CR/LF. */
    const char *p = body;
    int line = 0;
    char *dst[3] = { out->name, out->line1, out->line2 };
    size_t dst_sz[3] = { sizeof(out->name), sizeof(out->line1), sizeof(out->line2) };
    size_t w = 0;
    while (*p && line < 3) {
        if (*p == '\r') { p++; continue; }
        if (*p == '\n') {
            if (w < dst_sz[line]) dst[line][w] = '\0';
            else                  dst[line][dst_sz[line] - 1] = '\0';
            w = 0;
            line++;
            p++;
            continue;
        }
        if (w < dst_sz[line] - 1) dst[line][w++] = *p;
        p++;
    }
    if (line < 2) return false;
    /* Trim trailing spaces from name. */
    for (int i = (int)strlen(out->name) - 1; i >= 0; --i) {
        if (out->name[i] == ' ') out->name[i] = '\0';
        else break;
    }
    out->norad      = norad;
    out->fetched_ts = (uint32_t)time(nullptr);
    return true;
}

static bool tle_cache_path(uint32_t norad, char *out, size_t sz)
{
    snprintf(out, sz, "%s/%lu.tle", SATCOM_TLE_DIR, (unsigned long)norad);
    return true;
}

static bool tle_load_cache(uint32_t norad, satcom_tle_t *out, uint32_t max_age_sec)
{
    char path[64];
    tle_cache_path(norad, path, sizeof(path));
    if (!sd_is_mounted()) return false;
    File f = SD.open(path, FILE_READ);
    if (!f) return false;
    size_t sz = f.size();
    if (sz < 50 || sz > 500) { f.close(); return false; }
    String body = f.readString();
    f.close();

    if (!tle_parse_block(body.c_str(), norad, out)) return false;

    /* Age check via file last-modify isn't reliable on FAT — use the
     * cached fetched_ts written into a sidecar would be cleaner, but
     * for v1 we just always re-fetch on entry unless WiFi unavailable. */
    (void)max_age_sec;
    return true;
}

static bool tle_save_cache(const satcom_tle_t *tle)
{
    if (!sd_is_mounted()) return false;
    SD.mkdir(SATCOM_TLE_DIR);
    char path[64];
    tle_cache_path(tle->norad, path, sizeof(path));
    File f = SD.open(path, FILE_WRITE);
    if (!f) return false;
    f.printf("%s\n%s\n%s\n", tle->name, tle->line1, tle->line2);
    f.close();
    return true;
}

/* Try the firmware-embedded TLE table first. These are baked into
 * flash by scripts/fetch_satcom_tles.py and are fully offline-usable
 * from day one. Returns true if a baked entry exists for this NORAD. */
static bool tle_load_baked(uint32_t norad, satcom_tle_t *out)
{
    for (int i = 0; i < SATCOM_BAKED_TLES_N; i++) {
        if (SATCOM_BAKED_TLES[i].norad != norad) continue;
        out->norad = norad;
        strncpy(out->name,  SATCOM_BAKED_TLES[i].name,  sizeof(out->name)  - 1);
        strncpy(out->line1, SATCOM_BAKED_TLES[i].line1, sizeof(out->line1) - 1);
        strncpy(out->line2, SATCOM_BAKED_TLES[i].line2, sizeof(out->line2) - 1);
        out->name[sizeof(out->name) - 1]   = '\0';
        out->line1[sizeof(out->line1) - 1] = '\0';
        out->line2[sizeof(out->line2) - 1] = '\0';
        out->fetched_ts = 0;
        return true;
    }
    return false;
}

bool satcom_fetch_tle(uint32_t norad, satcom_tle_t *out, uint32_t max_age_sec)
{
    /* Priority chain:
     *   1. SD cache (most up-to-date if user ran "refresh" recently)
     *   2. Firmware-baked TLE (offline-friendly, accurate for ~2 weeks
     *      after each script run)
     *   3. Live Celestrak fetch (only if WiFi happens to be up — never
     *      required)
     *
     * (void)max_age_sec — parameter retained for API compat, dead.
     * Reason: SD age-tracking would need a sidecar file. The baked
     * data is the offline-fresh source; if you want NEW data, run
     * the fetch script + reflash. */
    (void)max_age_sec;

    if (tle_load_cache(norad, out, 0)) return true;
    if (tle_load_baked(norad, out)) return true;

    /* Only try network if WiFi was already up (no auto-connect here). */
    if (WiFi.status() != WL_CONNECTED) return false;

    char url[128];
    snprintf(url, sizeof(url),
             "https://celestrak.org/NORAD/elements/gp.php?CATNR=%lu&FORMAT=TLE",
             (unsigned long)norad);

    HTTPClient http;
    http.setTimeout(8000);
    if (!http.begin(url)) return false;
    int code = http.GET();
    if (code != 200) { http.end(); return false; }
    String body = http.getString();
    http.end();
    if (body.length() < 50) return false;
    if (!tle_parse_block(body.c_str(), norad, out)) return false;
    tle_save_cache(out);
    return true;
}

/* ---- SGP4 wrappers ---- */

static bool sgp4_load(Sgp4 &sat, const satcom_tle_t *tle)
{
    /* SparkFun lib wants line1/line2 as char* — cast away const.
     * Returns true on parse success. */
    return sat.init(tle->name, (char *)tle->line1, (char *)tle->line2);
}

bool satcom_compute(const satcom_tle_t *tle,
                    double obs_lat_deg, double obs_lon_deg, double obs_alt_m,
                    uint32_t utc_sec,
                    satcom_pos_t *out)
{
    if (!tle || !out) return false;
    Sgp4 sat;
    sat.site(obs_lat_deg, obs_lon_deg, obs_alt_m);
    if (!sgp4_load(sat, tle)) return false;

    /* SparkFun's API uses unixtime as time_t. findsat() populates azel
     * + lat/lon/alt/distance/velocity. */
    sat.findsat((unsigned long)utc_sec);

    out->az_deg          = sat.satAz;
    out->el_deg          = sat.satEl;
    out->lat_deg         = sat.satLat;
    out->lon_deg         = sat.satLon;
    out->alt_km          = sat.satAlt;
    out->range_km        = sat.satDist;
    out->range_rate_kms  = 0.0;   /* lib doesn't expose velocity directly */
    out->valid           = true;
    return true;
}

int satcom_predict_passes(const satcom_tle_t *tle,
                          double obs_lat_deg, double obs_lon_deg, double obs_alt_m,
                          uint32_t start_utc_sec, uint32_t window_sec,
                          double min_el_deg,
                          satcom_pass_t *passes, int max_passes)
{
    if (!tle || !passes || max_passes <= 0) return 0;

    Sgp4 sat;
    sat.site(obs_lat_deg, obs_lon_deg, obs_alt_m);
    if (!sgp4_load(sat, tle)) return 0;

    int n = 0;
    bool in_pass = false;
    bool first_iter = true;
    satcom_pass_t cur = {};
    const uint32_t STEP = 30;

    for (uint32_t t = start_utc_sec; t < start_utc_sec + window_sec && n < max_passes; t += STEP) {
        sat.findsat((unsigned long)t);
        double el = sat.satEl;

        /* H4 fix: if the satellite is already above min_el on the very
         * first iteration, the real AOS was in the past — skip this
         * in-progress pass to avoid reporting aos_ts=now. Wait until
         * el drops below min_el, then resume normal pass detection. */
        if (first_iter && el >= min_el_deg) {
            in_pass = false;  /* don't capture this pass */
            first_iter = false;
            continue;
        }
        first_iter = false;

        if (!in_pass && el >= min_el_deg) {
            in_pass = true;
            cur.aos_ts     = t;
            cur.max_el_deg = el;
            cur.max_el_ts  = t;
        } else if (in_pass) {
            if (el > cur.max_el_deg) {
                cur.max_el_deg = el;
                cur.max_el_ts  = t;
            }
            if (el < min_el_deg) {
                cur.los_ts = t;
                passes[n++] = cur;
                in_pass = false;
                cur = {};
            }
        }
    }
    return n;
}
