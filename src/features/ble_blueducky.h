/*
 * ble_blueducky — BlueDucky-style BLE HID keystroke injection.
 *
 * Targets unpatched Android (CVE-2023-45866). Reuses the DuckyScript
 * payloads from badusb_extras_data.h. Personal-use only.
 */
#pragma once

void feat_ble_blueducky(void);
