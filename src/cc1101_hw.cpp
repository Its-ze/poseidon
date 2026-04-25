/*
 * cc1101_hw.cpp — CC1101 init/teardown for Hydra RF Cap 424.
 */
#include "cc1101_hw.h"
#include "sd_helper.h"
#include <ELECHOUSE_CC1101_SRC_DRV.h>
#include <SD.h>

static bool s_up = false;

void cc1101_park_others(void)
{
    /* SD CS=12, nRF24 CS=6: hold HIGH so they ignore SPI traffic. */
    pinMode(12, OUTPUT); digitalWrite(12, HIGH);
    pinMode(6, OUTPUT);  digitalWrite(6, HIGH);
}

bool cc1101_begin(float freq_mhz)
{
    if (s_up) cc1101_end();
    cc1101_park_others();

    /* Reuse the SD's HSPI instance (pins 40/39/14 are shared). With
     * the bmorcelli fork of the ELECHOUSE lib we can pass that
     * instance in via setSPIinstance so the lib skips its own
     * SPI.begin() — which on Arduino-ESP32 3.x put pins into SPI
     * peripheral mode then immediately digitalWrite/Read them, logging
     * "IO X is not set as GPIO" errors and spinning forever in
     * Reset()'s MISO-wait loop. By handing the library the SD SPI
     * instance (already initialised) the pin-mode conflict disappears. */
    ELECHOUSE_cc1101.setSPIinstance(&sd_get_spi());
    pinMode(CC1101_CS, OUTPUT); digitalWrite(CC1101_CS, HIGH);
    ELECHOUSE_cc1101.setSpiPin(40, 39, 14, CC1101_CS);
    /* Do NOT call setGDO() — it sets GDO0 to OUTPUT which blocks the
     * CC1101's data signal. The official RCSwitch example skips it.
     * GDO0 must be INPUT so the CC1101 drives it and RCSwitch reads. */
    ELECHOUSE_cc1101.Init();
    delay(10);
    pinMode(CC1101_GDO0, INPUT);  /* CC1101 drives this pin, we read it */

    /* Verify the chip is actually there by reading version register.
     * getCC1101() returns false if SPI reads back 0x00 or 0xFF. */
    if (!ELECHOUSE_cc1101.getCC1101()) {
        Serial.println("[cc1101] chip not detected — wrong hat?");
        return false;
    }

    /* Tuned for car keys / garage remotes — Flipper's "AM650" preset
     * which covers the vast majority of 315/433 MHz OOK fobs. Prior
     * values (RxBW 256, DRate 50) were too narrow + too fast: the
     * demod filter ate the slow (~2-5 kbps) pulses that car remotes
     * emit, so GDO0 never transitioned even though RSSI tracked the
     * burst. RxBW 650 + DRate 3.794 matches Flipper's capture range. */
    ELECHOUSE_cc1101.setModulation(2);          /* ASK/OOK */
    ELECHOUSE_cc1101.setMHZ(freq_mhz);
    ELECHOUSE_cc1101.setRxBW(650);              /* AM650 — wide enough for car fobs */
    ELECHOUSE_cc1101.setClb(1, 13, 15);         /* VCO calibration (Bruce) */
    ELECHOUSE_cc1101.setClb(2, 16, 19);
    ELECHOUSE_cc1101.setDRate(3.794);           /* AM650 data rate */
    ELECHOUSE_cc1101.setPktFormat(3);           /* async serial on GDO0 */
    ELECHOUSE_cc1101.SetRx();

    /* In async serial mode (PKT_FORMAT=3), GDO0 is the raw data line
     * regardless of IOCFG0 setting. Ensure pin stays INPUT. */
    pinMode(CC1101_GDO0, INPUT);

    s_up = true;
    Serial.printf("[cc1101] up @ %.3f MHz\n", freq_mhz);
    return true;
}

void cc1101_end(void)
{
    if (!s_up) return;
    ELECHOUSE_cc1101.setSidle();
    ELECHOUSE_cc1101.goSleep();
    s_up = false;
}

bool cc1101_is_up(void)    { return s_up; }
void cc1101_set_freq(float mhz) { ELECHOUSE_cc1101.setMHZ(mhz); }
void cc1101_set_rx(void)   { ELECHOUSE_cc1101.SetRx(); }
void cc1101_set_tx(void)   { ELECHOUSE_cc1101.SetTx(); }
void cc1101_set_idle(void) { ELECHOUSE_cc1101.setSidle(); }

int cc1101_get_rssi(void)
{
    int raw = ELECHOUSE_cc1101.getRssi();
    return raw;
}
