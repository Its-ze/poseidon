/*
 * wifi_types — shared types for WiFi features so the scan result can
 * be handed to deauth / portal / ap-clone / karma without each module
 * redefining it.
 */
#pragma once

#include <Arduino.h>

struct ap_t {
    char     ssid[33];
    uint8_t  bssid[6];
    int8_t   rssi;
    uint8_t  channel;
    uint8_t  auth;
    bool     is_5g;   /* source: local S3 scan = false, C5 satellite = true */
    bool     wps;     /* WPS IE present in beacon — router is PIN-attackable
                         (Pixie Dust / Reaver). Populated from
                         wifi_ap_record_t.wps for local scans; always false
                         for C5 satellite results since those go through a
                         different IE-parse path. */
};

/* The last AP the user highlighted in the scan screen. */
extern ap_t g_last_selected_ap;
extern bool g_last_selected_valid;
