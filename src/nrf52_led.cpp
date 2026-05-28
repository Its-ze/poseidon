/*
 * nrf52_led.cpp — NeoPixel command sender for Feather nRF52840.
 *
 * The Feather's built-in NeoPixel (pin D8 on the nRF52) is driven
 * entirely by the Feather's own firmware. We just send mode IDs over
 * UART and the Feather runs the animation autonomously.
 *
 * ──────────────────────────────────────────────────────────────
 *  ANIMATION REFERENCE — what each mode looks like on the pixel
 * ──────────────────────────────────────────────────────────────
 *
 *  OFF          ▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓▓  dead dark, nothing
 *
 *  BOOT         ░░▒▒▓▓██ CYAN ██▓▓▒▒░░     ember slowly igniting
 *               tiny cyan glow fading up over 2s, like a reactor
 *               powering on. Holds at 40% brightness.
 *
 *  CONNECTED    ██ ELECTRIC BLUE ██         solid #0033FF with a
 *               subtle 4s sinusoidal pulse between 60-100%.
 *               The "I'm alive" heartbeat.
 *
 *  IDLE         ░░▒▒▓▓██ CYAN ██▓▓▒▒░░     deep ocean breathing.
 *               #00FFFF fading 10%→80%→10% over 3s. Slow, calm,
 *               meditative. The Cardputer is ready but dormant.
 *
 *  BLE_SCAN     CYAN → MAGENTA → E_BLUE    color morphs through
 *               the POSEIDON palette. 1.5s per transition.
 *               Looks like a scanner sweeping through frequencies.
 *
 *  LONG_RANGE   ████ DEEP BLUE ████         #001188 throbs at
 *               0.25 Hz (very slow, deep). Every 4th cycle, a
 *               sharp cyan (#00FFFF) spike flashes for 100ms
 *               like a sonar ping reaching out into the void.
 *
 *  ZIGBEE       ▓GREEN▓ ░░ ▓AMBER▓ ░░      alternating #00FF44
 *               and #FF7700 at 2 Hz. Looks like a traffic light
 *               negotiation — the 802.15.4 mesh handshake vibe.
 *
 *  BLE_SNIFF    ▓PURPLE▓MAGENTA▓PURPLE▓     rapid flicker between
 *               #8800FF and #FF00FF at 8-12 Hz (irregular).
 *               Simulates raw data streaming through a wire.
 *               Intensity varies randomly ±30% for that analog
 *               oscilloscope feel.
 *
 *  PROMISC      TEAL → CYAN → TEAL          soft color ripple
 *               #00AA88 ↔ #00FFFF at 0.8 Hz. Gentle, passive.
 *               "I'm watching everything but touching nothing."
 *
 *  DEAUTH       ██BLOOD██ ░░ ██BLOOD██      #FF0022 hard strobe
 *               at 4 Hz. Sharp on/off, no fade. Aggressive.
 *               Every 8th flash holds for 200ms (stutter effect).
 *
 *  MITM         RED ↔ MAGENTA               #FF0022 ↔ #FF00FF
 *               alternating at 3 Hz. The two-faced attack —
 *               client sees one color, AP sees the other.
 *
 *  MOUSEJACK    AMBER ↔ CYAN                #FF7700 ↔ #00FFFF
 *               alternating at 2 Hz with a 50ms blackout gap.
 *               Looks like keystrokes being injected — tap, tap.
 *
 *  FLOOD        ▓PURPLE▓▓▓ ░░ (repeat)      triple-blink #8800FF
 *               (80ms on, 40ms off × 3), then 400ms dark pause.
 *               The "packet storm" rhythm.
 *
 *  CAPTURE      ████ GOLD BURST ████        #FFB300 at full
 *               brightness for 300ms, then exponential decay
 *               (tau=800ms) fading into the IDLE cyan breathing.
 *               The "GOT ONE" moment.
 *
 *  PMKID        ▓CYAN▓ → ██GOLD██ → fade   #00FFFF flash 150ms
 *               then #FFB300 hold 2s, then 1s fade to idle.
 *
 *  HANDSHAKE    ▓MAGENTA▓ → ██GOLD██        #FF00FF flash 200ms
 *               then #FFB300 hold 3s (longer than PMKID because
 *               handshakes are rarer and more valuable), fade.
 *
 *  LEVEL_UP     RAINBOW EXPLOSION           rapid cycle through
 *               cyan→magenta→neon green→amber→purple→electric
 *               blue at 15 Hz for 3 seconds. Full brightness.
 *               The Feather's pixel goes absolutely nuts.
 *               Then settles into a gold breathing pulse for 2s
 *               before reverting.
 *
 *  ERROR        ░░▒▒▓▓ DEEP RED ▓▓▒▒░░     #880000 slow breathing
 *               at 0.5 Hz. Ominous, something is wrong.
 *
 *  NO_TARGET    ▓AMBER▓ ░░░░░ ▓AMBER▓      #FF7700 blink once
 *               per 2 seconds. "I'm looking but there's nothing."
 * ──────────────────────────────────────────────────────────────
 */
#include "nrf52_led.h"
#include "nrf52_hw.h"
#include <Arduino.h>

static nrf52_led_mode_t s_current = NRF52_LED_OFF;

const char *nrf52_led_mode_name(nrf52_led_mode_t mode)
{
    switch (mode) {
    case NRF52_LED_OFF:        return "OFF";
    case NRF52_LED_BOOT:       return "BOOT";
    case NRF52_LED_CONNECTED:  return "CONNECTED";
    case NRF52_LED_IDLE:       return "IDLE";
    case NRF52_LED_BLE_SCAN:   return "BLE_SCAN";
    case NRF52_LED_LONG_RANGE: return "LONG_RANGE";
    case NRF52_LED_ZIGBEE:     return "ZIGBEE";
    case NRF52_LED_BLE_SNIFF:  return "BLE_SNIFF";
    case NRF52_LED_PROMISC:    return "PROMISC";
    case NRF52_LED_DEAUTH:     return "DEAUTH";
    case NRF52_LED_MITM:       return "MITM";
    case NRF52_LED_MOUSEJACK:  return "MOUSEJACK";
    case NRF52_LED_FLOOD:      return "FLOOD";
    case NRF52_LED_CAPTURE:    return "CAPTURE";
    case NRF52_LED_PMKID:      return "PMKID";
    case NRF52_LED_HANDSHAKE:  return "HANDSHAKE";
    case NRF52_LED_LEVEL_UP:   return "LEVEL_UP";
    case NRF52_LED_ERROR:      return "ERROR";
    case NRF52_LED_NO_TARGET:    return "NO_TARGET";
    case NRF52_LED_SCOUT_LOCKED: return "SCOUT_LOCKED";
    case NRF52_LED_SCOUT_STRIKE: return "SCOUT_STRIKE";
    case NRF52_LED_COMBO_DEAUTH: return "COMBO_DEAUTH";
    case NRF52_LED_COMBO_WAIT:   return "COMBO_WAIT";
    case NRF52_LED_COMBO_CAPTURE:return "COMBO_CAPTURE";
    }
    return "?";
}

void nrf52_led_set(nrf52_led_mode_t mode)
{
    if (!NRF52Hardware::is_up()) return;
    if (mode == s_current) return;

    s_current = mode;
    char cmd[24];
    snprintf(cmd, sizeof(cmd), "LED:%d", (int)mode);

    /* Fire-and-forget — don't wait for OK. The LED command is
     * non-critical and we don't want to block the UI thread
     * waiting for a response during a hot capture loop. */
    String discard;
    NRF52Hardware::send_command(cmd, discard, 50);

    Serial.printf("[nrf52_led] → %s (%d)\n",
                  nrf52_led_mode_name(mode), (int)mode);
}

void nrf52_led_oneshot(nrf52_led_mode_t flash, nrf52_led_mode_t revert,
                       uint32_t duration_ms)
{
    if (!NRF52Hardware::is_up()) return;

    s_current = flash;  /* will be reverted by Feather firmware */
    char cmd[48];
    snprintf(cmd, sizeof(cmd), "LED:%d,%d,%lu",
             (int)flash, (int)revert, (unsigned long)duration_ms);

    String discard;
    NRF52Hardware::send_command(cmd, discard, 50);

    Serial.printf("[nrf52_led] → oneshot %s → %s (%lums)\n",
                  nrf52_led_mode_name(flash),
                  nrf52_led_mode_name(revert),
                  (unsigned long)duration_ms);
}
