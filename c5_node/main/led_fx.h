#pragma once

typedef enum {
    LED_MODE_OFF,
    LED_MODE_BOOT,    /* power-on confirmation: green swell + 2 blinks, auto-returns to IDLE */
    LED_MODE_IDLE,    /* slow magenta breathe */
    LED_MODE_SCAN,    /* cyan pulse */
    LED_MODE_ATTACK,  /* magenta/cyan strobe */
    LED_MODE_PING,    /* one cyan flash, auto-returns to IDLE */
    LED_MODE_ZB_RX,   /* one white blip per Zigbee frame, returns to SCAN */
    LED_MODE_DONE,    /* operation finished OK: 2 green blinks, auto-returns to IDLE */
    LED_MODE_ERROR,   /* operation failed: 3 red blinks, auto-returns to IDLE */
} led_mode_t;

void led_fx_init(void);
void led_fx_set(led_mode_t m);
