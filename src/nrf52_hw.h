#pragma once
#include <Arduino.h>

/*
 * NRF52Hardware — driver for the Adafruit Feather nRF52840 Bluefruit.
 *
 * The Feather sits on the Cardputer-Adv's top GPIO header as a "hat",
 * same physical slot as the LoRa-1262 or Hydra RF Cap. Only one hat
 * is connected at a time — no pin conflicts.
 *
 * Wiring (Cardputer-Adv top hat header → Feather):
 *   G3  → Feather RX    (POSEIDON TX → Feather)
 *   G4 → Feather TX    (Feather TX → POSEIDON RX)
 *   3V3 → Feather 3V
 *   GND → Feather GND
 *
 * Uses UART1 at 115200 baud. The Feather runs custom firmware that:
 *   - Accepts text commands ("BLE_SCAN", "ZB_SNIFF 15", etc.)
 *   - Streams results back as "DEV:", "PKT:", "ZB:" prefixed lines
 *   - Drives its own NeoPixel (pin D8) based on LED mode commands
 *   - Responds to "PING" with "PONG" for detection
 *
 * Protocol:
 *   Command:  "CMD_NAME [args]\n"
 *   Response: data lines, then "OK\n" or "ERR:<msg>\n"
 *   Stream:   continuous lines until "STOP\n" is sent
 *   LED:      "LED:<mode_id>[,<revert_id>,<duration_ms>]\n"
 */
class NRF52Hardware {
public:
    static bool begin();
    static void end();
    static bool is_up();

    /* Send a command and wait for OK/ERR. Response contains all
     * non-OK/ERR lines received. Timeout in ms. */
    static bool send_command(const char *cmd, String &response,
                             uint32_t timeout_ms = 1000);

    /* Stream reading for passive sniffing modes. */
    static bool available();
    static String read_line();
};
