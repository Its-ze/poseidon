/*
 * evil_twin — chained AP-clone + captive portal + periodic deauth of
 * the real target AP, time-sliced between PHASE A (deauth burst) and
 * PHASE B (portal hosting).
 *
 * Time-slicing is forced because concurrent AP + raw-TX STA on the same
 * channel is fragile on Bruce-pinned libs (APSTA mode races
 * ieee80211_hostap_attach + thrashes TX pools). One mode at a time.
 *
 * Uses g_last_selected_ap as the target — user must run WiFi -> Scan
 * and ENTER on an AP first. SSID is cloned, channel is matched.
 */
#pragma once

void feat_evil_twin(void);
