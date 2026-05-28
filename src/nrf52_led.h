/*
 * nrf52_led — NeoPixel animation controller for the Adafruit Feather
 * nRF52840 Bluefruit companion.
 *
 * The Feather has a built-in NeoPixel on pin D8. POSEIDON sends LED
 * mode commands over UART; the Feather firmware drives its own pixel.
 * This file defines the animation modes and sends the commands.
 *
 * COLOR PALETTE — pure cyberpunk, no white:
 *   CYAN     #00FFFF   POSEIDON signature, the ocean
 *   MAGENTA  #FF00FF   accent, data flow
 *   E_BLUE   #0033FF   deep electric blue
 *   NEON_GRN #00FF44   Matrix phosphor
 *   AMBER    #FF7700   Tron legacy, warm CRT
 *   PURPLE   #8800FF   phantom, stealth
 *   BLOOD    #FF0022   danger, active attack
 *   GOLD     #FFB300   capture success, XP
 *   TEAL     #00AA88   subtle status
 */
#pragma once

#include <Arduino.h>

/* Each mode is a self-contained animation that the nRF52 firmware
 * runs autonomously on its NeoPixel until a new mode is received.
 * ESP32 sends "LED:<mode_id>" over UART. */
enum nrf52_led_mode_t {
    /* ---- Idle / Status ---- */
    NRF52_LED_OFF        = 0,   /* Kill the pixel                              */
    NRF52_LED_BOOT       = 1,   /* Startup: cyan ember slowly brightening       */
    NRF52_LED_CONNECTED  = 2,   /* Solid electric blue heartbeat (slow pulse)   */
    NRF52_LED_IDLE       = 3,   /* Deep breathing cyan, 3s cycle                */

    /* ---- Scanning ---- */
    NRF52_LED_BLE_SCAN   = 10,  /* Cyan→magenta→blue color morph, medium       */
    NRF52_LED_LONG_RANGE = 11,  /* Deep blue slow throb + cyan spike every 4s   */
    NRF52_LED_ZIGBEE     = 12,  /* Matrix green / amber alternating strobe      */

    /* ---- Sniffing / Passive ---- */
    NRF52_LED_BLE_SNIFF  = 20,  /* Rapid purple→magenta data-stream flicker     */
    NRF52_LED_PROMISC    = 21,  /* Teal→cyan→teal soft ripple                   */

    /* ---- Active Attacks ---- */
    NRF52_LED_DEAUTH     = 30,  /* Blood red rapid-fire strobe (4 Hz)           */
    NRF52_LED_MITM       = 31,  /* Red↔magenta aggressive fast alternate        */
    NRF52_LED_MOUSEJACK  = 32,  /* Amber↔cyan keystroke injection pulse         */
    NRF52_LED_FLOOD      = 33,  /* Purple rapid triple-blink, pause, repeat     */

    /* ---- Capture / Success ---- */
    NRF52_LED_CAPTURE    = 40,  /* Gold burst → slow fade to cyan               */
    NRF52_LED_PMKID      = 41,  /* Cyan flash → hold gold 2s → fade             */
    NRF52_LED_HANDSHAKE  = 42,  /* Magenta flash → hold gold 3s → fade          */
    NRF52_LED_LEVEL_UP   = 43,  /* Neon rainbow explosion: rapid C→M→G→A cycle  */

    /* ---- Dual-Radio Attack Modes ---- */
    NRF52_LED_SCOUT_LOCKED = 60, /* Amber crosshair pulse — target acquired      */
    NRF52_LED_SCOUT_STRIKE = 61, /* Red/cyan aggressive split — raw sniffing      */
    NRF52_LED_COMBO_DEAUTH = 62, /* Blood red rapid strobe — WiFi deauth phase   */
    NRF52_LED_COMBO_WAIT   = 63, /* Amber→teal slow morph — waiting for BLE      */
    NRF52_LED_COMBO_CAPTURE= 64, /* Gold supernova — provisioning captured        */

    /* ---- Errors / Warnings ---- */
    NRF52_LED_ERROR      = 50,  /* Deep red slow breathing                      */
    NRF52_LED_NO_TARGET  = 51,  /* Amber→off→amber slow blink                   */
};

/* Human-readable names for serial debug logging. */
const char *nrf52_led_mode_name(nrf52_led_mode_t mode);

/* Send a mode change to the Feather over UART. Non-blocking. */
void nrf52_led_set(nrf52_led_mode_t mode);

/* Convenience: fire a one-shot animation (e.g. CAPTURE) then auto-revert
 * to the given fallback mode after duration_ms. The revert is handled
 * by the nRF52 firmware — we just send "LED:<mode>,<revert>,<ms>". */
void nrf52_led_oneshot(nrf52_led_mode_t flash, nrf52_led_mode_t revert,
                       uint32_t duration_ms = 2000);
