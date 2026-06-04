/*
 * wifi_ap_helpers — shared raw-IDF AP bring-up + teardown.
 *
 * Lifted from wifi_portal.cpp:417-481 to consolidate the 5 new
 * raw-IDF AP sites that POS-AUDIT-010 migrates from the banned
 * Arduino WiFi.softAP path. Phase 2 (wifi-042 / POS-AUDIT-216)
 * will migrate the older inlines (wifi_portal, evil_twin,
 * ap_signal_test, wifi_ciw) to use this helper too.
 *
 * Why raw-IDF instead of WiFi.softAP:
 *   - Arduino WiFi.softAP triggers ieee80211_hostap_attach +0x2c
 *     crash on pinned Bruce libs.
 *   - Repeated softAP re-attach drives ESP_ERR_NO_MEM 257.
 *   - WiFi.mode(WIFI_MODE_APSTA) teardown doubles the buffer pools.
 *
 * The recipe: stop → deinit → 300 ms → destroy stray STA netif →
 * BTDM IDLE gate (POS-AUDIT-007) → netif_init → event_loop_create →
 * create_default_wifi_ap → esp_wifi_init with shrunk buf pool that
 * fits Cardputer's ~115 KB internal SRAM → set_mode(AP) → set_config
 * (SSID, ch, OPEN) → start → force channel.
 */
#pragma once

#include <Arduino.h>
#include <IPAddress.h>
#include <stdint.h>

/* Bring up an open WiFi AP via the raw-IDF path. Idempotent against
 * prior init state. Returns true on success, false if esp_wifi_init
 * or esp_wifi_start failed (caller should ui_toast and return).
 *
 * Defaults match wifi_portal's recipe — channel 1, not hidden, 4 max
 * stations. Callers that need more can override per site (net_attacks
 * uses hidden=true; net_wpad uses max_conn=10).
 *
 * The AP IP is left at lwIP's default 192.168.4.1 — callers that need
 * to know it can use wifi_raw_ap_ip() after this returns. */
bool wifi_raw_ap_up(const char *ssid,
                    uint8_t channel    = 1,
                    bool    hidden     = false,
                    uint8_t max_conn   = 4);

/* Teardown matching wifi_raw_ap_up. Safe to call even if bring-up
 * partially failed. */
void wifi_raw_ap_down(void);

/* Read back the AP-side IP (lwIP default 192.168.4.1). Returns
 * 0.0.0.0 if the netif handle can't be resolved (e.g. AP not up). */
IPAddress wifi_raw_ap_ip(void);
