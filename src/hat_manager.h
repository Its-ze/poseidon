#pragma once

#include <Arduino.h>

enum class HatType {
    UNKNOWN = 0,
    NONE,
    CAP_LORA1262,
    HYDRA_RF_424,
    ESP32_C5_NODE,
    FEATHER_NRF52       /* Adafruit Feather nRF52840 Bluefruit (BLE 5.0) */
};

class HatManager {
public:
    /* Called before M5Cardputer.begin() to prevent SX1262 from fighting the autodetect pins */
    static void pre_init();

    /* Called after M5Cardputer.begin() to park SPI CS lines safely */
    static void post_init();

    /* Attempt to intelligently identify the attached hat by probing I2C/SPI */
    static HatType detect();

    /* Get the currently detected hat */
    static HatType current_hat();

    /* Helper functions to park SPI buses for specific radios */
    static void park_for_lora();
    static void park_for_cc1101();
    static void park_for_nrf24();

private:
    static HatType s_hat;
};
