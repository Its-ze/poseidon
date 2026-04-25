/*
 * screensaver — full-screen idle takeover with per-theme painters.
 *
 * Wired into the menu loops via screensaver_check_idle() — call when
 * input_poll() returned PK_NONE. Internally checks the idle threshold
 * against input_last_input_ms() and, if exceeded, runs a full-screen
 * theme-appropriate animation loop until any keypress arrives. The
 * triggering keypress is consumed (not surfaced to the menu) so the
 * user doesn't accidentally launch a feature on wake.
 *
 * Per-theme:
 *   POSEIDON  ->  WARDRIVE.cinema — fake wardrive scroll. SSIDs +
 *                 BSSIDs + RSSI bars + auth flags streaming in,
 *                 channel hop indicator, occasional "CAPTURED" red
 *                 flash, magenta packets streaking diagonally.
 *   MATRIX    ->  full-screen ui_matrix_rain at T_FG brightness.
 *                 Cinematic.
 *   E-INK     ->  "Breathing" — POSEIDON wordmark fades in/out on
 *                 a 22 s cycle, drifts position between cycles to
 *                 prevent burn-in.
 *
 * NVS-backed enable flag (default true) and timeout (default 120 s)
 * in namespace "pscr". Disable via System -> Screensaver toggle.
 */
#pragma once

#include <Arduino.h>

/* Returns true if the screensaver actually ran (caller should redraw
 * its UI on return). Returns false if still idle-but-not-yet or if the
 * screensaver is disabled. */
bool screensaver_check_idle(void);

bool     screensaver_enabled(void);
void     screensaver_enabled_set(bool on);
uint32_t screensaver_timeout_ms(void);
void     screensaver_timeout_set(uint32_t ms);
