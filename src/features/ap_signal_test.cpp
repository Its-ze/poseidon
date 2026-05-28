/*
 * ap_signal_test — diagnostic: is the AP path actually broadcasting?
 *
 * Brings up a known-good SSID ("POSEIDON-SIGTEST", open) on a pickable
 * channel (1/6/11) using the SAME raw-IDF recipe as wifi_portal — the
 * only path that survives Bruce libs (Arduino's WiFi.softAP crashes in
 * ieee80211_hostap_attach +0x2c).
 *
 * Verify from any phone or `netsh wlan show networks mode=bssid`.
 * If the SSID doesn't appear, the AP stack is the problem, not the
 * higher-level feature that called it.
 *
 * Display shows live: SSID, BSSID, channel, uptime, connected STAs,
 * TX power. ;/. cycles channel (full teardown + re-up). ESC exits
 * with full esp_wifi_stop + esp_wifi_deinit teardown.
 */
#include "app.h"
#include "../theme.h"
#include "ui.h"
#include "input.h"
#include "radio.h"
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_netif.h>
#include <esp_event.h>
#include <esp_err.h>
#include <esp_bt.h>

#define SIGTEST_SSID "POSEIDON-SIGTEST"

static const uint8_t CHANNELS[] = { 1, 6, 11 };
#define CHANNELS_N (sizeof(CHANNELS) / sizeof(CHANNELS[0]))

/* Bring up the softAP on `ch`. Returns true on success. Mirrors the
 * wifi_portal recipe verbatim — DO NOT diverge without retesting on
 * a clean cold-boot, because subtle buffer-count / channel-set ordering
 * differences regress to silent no-beacon. */
static bool ap_bring_up(uint8_t ch)
{
    esp_wifi_set_promiscuous(false);
    /* If a STA-mode feature (Scan, Clients, etc.) already inited WiFi,
     * we have to tear it down before we can re-init in AP mode. The
     * netif side has to be destroyed too — if we leave the STA netif
     * around and create the AP netif on top, the event handlers get
     * crossed and AP_START never fires. */
    wifi_mode_t pre_mode = WIFI_MODE_NULL;
    if (esp_wifi_get_mode(&pre_mode) == ESP_OK) {
        esp_wifi_stop();
        esp_wifi_deinit();
        delay(50);
    }
    /* BTDM memory release is REQUIRED for AP-mode features on Bruce libs.
     * Without it, ieee80211_hostap_attach hits a null-deref at offset 0x2c
     * because AP-side struct allocation fails. esp_bt_controller_disable
     * alone is NOT enough — the memory region itself must be released.
     *
     * SIDE EFFECT (unavoidable on these libs): one-way until POWER CYCLE.
     * BLE features (Scan/Spam/SourApple/etc.) are dead for the rest of
     * the session after this. Software reset (ESP.restart) does NOT
     * restore the BTDM controller. User must unplug + replug to recover
     * BLE. The AP-mode menu entries' = info text should warn about this. */
    esp_bt_controller_mem_release(ESP_BT_MODE_BTDM);

    esp_log_level_set("wifi",      ESP_LOG_INFO);
    esp_log_level_set("wifi_init", ESP_LOG_INFO);
    esp_netif_init();
    esp_event_loop_create_default();
    /* Only create the AP netif the first time. If we created an STA
     * netif earlier this session, it's still hanging around — that's
     * fine, our AP netif coexists logically. */
    static bool s_ap_netif_created = false;
    if (!s_ap_netif_created) {
        esp_netif_create_default_wifi_ap();
        s_ap_netif_created = true;
    }

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    /* Shrunk buffer counts — match wifi_portal exactly. Defaults OOM. */
    cfg.static_tx_buf_num  = 0;
    cfg.dynamic_tx_buf_num = 16;
    cfg.tx_buf_type        = 1;
    cfg.cache_tx_buf_num   = 4;
    cfg.static_rx_buf_num  = 4;
    cfg.dynamic_rx_buf_num = 16;
    cfg.ampdu_tx_enable    = 0;
    cfg.ampdu_rx_enable    = 0;
    esp_err_t rc = esp_wifi_init(&cfg);
    Serial.printf("[sigtest] wifi_init rc=%d\n", (int)rc);
    if (rc != ESP_OK) return false;
    esp_wifi_set_storage(WIFI_STORAGE_RAM);
    esp_wifi_set_mode(WIFI_MODE_AP);

    wifi_config_t apc = {};
    strncpy((char *)apc.ap.ssid, SIGTEST_SSID, sizeof(apc.ap.ssid) - 1);
    apc.ap.ssid_len        = strlen(SIGTEST_SSID);
    apc.ap.channel         = ch;
    apc.ap.authmode        = WIFI_AUTH_OPEN;
    apc.ap.max_connection  = 4;
    apc.ap.beacon_interval = 100;
    apc.ap.ssid_hidden     = 0;
    rc = esp_wifi_set_config(WIFI_IF_AP, &apc);
    Serial.printf("[sigtest] set_config rc=%d\n", (int)rc);
    rc = esp_wifi_start();
    Serial.printf("[sigtest] wifi_start rc=%d\n", (int)rc);
    if (rc != ESP_OK) {
        esp_wifi_deinit();
        return false;
    }
    /* CRITICAL: force channel post-start; config channel can be ignored. */
    esp_wifi_set_channel(ch, WIFI_SECOND_CHAN_NONE);

    /* Settle 1500 ms for AP_START to fully process / beacons to begin. */
    uint32_t t0 = millis();
    while (millis() - t0 < 1500) { delay(20); }
    return true;
}

static void ap_tear_down(void)
{
    esp_wifi_stop();
    esp_wifi_deinit();
}

static void draw_static(uint8_t ch_idx)
{
    auto &d = M5Cardputer.Display;
    ui_clear_body();
    d.setTextColor(T_ACCENT, T_BG);
    d.setCursor(4, BODY_Y + 2); d.print("AP SIGNAL TEST");
    d.drawFastHLine(4, BODY_Y + 12, SCR_W - 8, T_ACCENT);
    d.setTextColor(T_FG, T_BG);
    d.setCursor(4, BODY_Y + 18); d.printf("SSID: %s", SIGTEST_SSID);
    /* Channel hint row (top-right): show all three, mark active. */
    d.setTextColor(T_DIM, T_BG);
    d.setCursor(SCR_W - 78, BODY_Y + 2); d.print("ch:");
    for (uint8_t i = 0; i < CHANNELS_N; ++i) {
        d.setTextColor(i == ch_idx ? T_ACCENT : T_DIM, T_BG);
        d.setCursor(SCR_W - 60 + i * 20, BODY_Y + 2);
        d.printf("%u", (unsigned)CHANNELS[i]);
    }
    ui_draw_footer(";/.=ch  `=exit");
}

void feat_ap_signal_test(void)
{
    radio_switch(RADIO_WIFI);

    uint8_t ch_idx = 0;
    if (!ap_bring_up(CHANNELS[ch_idx])) {
        ui_toast("AP bring-up failed", T_BAD, 1500);
        return;
    }

    draw_static(ch_idx);
    auto &d = M5Cardputer.Display;

    uint32_t t_start = millis();
    uint32_t last    = 0;
    bool need_full_redraw = false;

    while (true) {
        if (need_full_redraw) {
            need_full_redraw = false;
            draw_static(ch_idx);
            t_start = millis();
            last    = 0;
        }

        if (millis() - last > 500) {
            last = millis();

            uint8_t mac[6] = {0};
            esp_wifi_get_mac(WIFI_IF_AP, mac);

            uint8_t cur_ch = 0;
            wifi_second_chan_t sc;
            esp_wifi_get_channel(&cur_ch, &sc);

            int8_t pwr = 0;
            esp_wifi_get_max_tx_power(&pwr);

            wifi_sta_list_t stas = {};
            esp_wifi_ap_get_sta_list(&stas);

            uint32_t up = (millis() - t_start) / 1000;

            /* Clear the dynamic block (y+30..y+96). */
            d.fillRect(0, BODY_Y + 30, SCR_W, 70, T_BG);
            d.setTextColor(T_FG, T_BG);
            d.setCursor(4, BODY_Y + 30);
            d.printf("BSSID: %02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            d.setCursor(4, BODY_Y + 42); d.printf("CH:    %u", (unsigned)cur_ch);
            d.setCursor(4, BODY_Y + 54); d.printf("UP:    %lus", (unsigned long)up);
            d.setCursor(4, BODY_Y + 66); d.printf("STA:   %u", (unsigned)stas.num);
            /* TX power is in 0.25 dBm units. */
            d.setCursor(4, BODY_Y + 78);
            d.printf("TX:    %d.%02d dBm", pwr / 4, (pwr & 3) * 25);

            ui_draw_status(radio_name(), "sigtest");
        }

        uint16_t k = input_poll();
        if (k == PK_ESC) break;
        if (k == ';' || k == '.' || k == PK_UP || k == PK_DOWN) {
            if (k == ';' || k == PK_UP) {
                ch_idx = (ch_idx == 0) ? (CHANNELS_N - 1) : (ch_idx - 1);
            } else {
                ch_idx = (ch_idx + 1) % CHANNELS_N;
            }
            /* Full teardown + re-up. esp_wifi_set_channel alone is not
             * enough on some IDF builds — beacon TX context tied to
             * channel at start. Safest is a clean cycle. */
            ap_tear_down();
            if (!ap_bring_up(CHANNELS[ch_idx])) {
                ui_toast("AP bring-up failed", T_BAD, 1500);
                return;
            }
            need_full_redraw = true;
        }
        if (k == PK_NONE) { delay(5); }
    }

    ap_tear_down();
}
