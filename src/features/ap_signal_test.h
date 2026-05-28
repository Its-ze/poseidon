/*
 * ap_signal_test.h — permanent AP-broadcast diagnostic.
 *
 * Brings up "POSEIDON-SIGTEST" (open, channel 1/6/11 selectable) via
 * the raw-IDF recipe shared with wifi_portal. Renders a live readout
 * of SSID, BSSID, channel, uptime, clients, TX power so the operator
 * can confirm the AP path is actually beaconing.
 */
#pragma once

void feat_ap_signal_test(void);
