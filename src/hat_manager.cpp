#include "hat_manager.h"
#include <SPI.h>
#include <Wire.h>

HatType HatManager::s_hat = HatType::UNKNOWN;

void HatManager::pre_init() {
    /* Cardputer-Adv hat compatibility: the CAP-LoRa1262 wires SX1262
     * NSS=G5, BUSY=G6, DIO1=G4, RST=G3. At power-on the SX1262 drives
     * BUSY HIGH until its ready, which breaks M5GFX's board autodetect.
     * Holding RST LOW keeps every SX1262 output in high-Z. */
    pinMode(3, OUTPUT);
    digitalWrite(3, LOW);
    delay(5);
}

void HatManager::post_init() {
    /* Release pin 5 — LoRa hat uses it as NSS (needs HIGH to deselect),
     * Hydra hat uses it as CC1101 GDO0 (input). */
    pinMode(5, INPUT_PULLUP);
    
    /* Default parking state for other CS lines to prevent SPI contention */
    pinMode(13, INPUT_PULLUP); // Hydra CC1101 CS
    pinMode(6, INPUT_PULLUP);  // Hydra NRF24 CS
}

HatType HatManager::detect() {
    if (s_hat != HatType::UNKNOWN) {
        return s_hat;
    }
    
    // Future Implementation: Actively probe SPI/I2C buses to detect the hat.
    // e.g. Reading CC1101 PARTNUM or SX1262 status registers.
    s_hat = HatType::NONE; 
    return s_hat;
}

HatType HatManager::current_hat() {
    return s_hat;
}

void HatManager::park_for_lora() {
    // LoRa uses SPI with CS on G5. Make sure Hydra CS lines are pulled up if present.
    pinMode(13, INPUT_PULLUP);
    pinMode(6, INPUT_PULLUP);
    // Park SD CS
    pinMode(12, OUTPUT); digitalWrite(12, HIGH);
}

void HatManager::park_for_cc1101() {
    // Hydra CC1101 uses CS on 13. Park LoRa NSS and NRF CS.
    pinMode(5, INPUT_PULLUP); // LoRa NSS
    pinMode(6, INPUT_PULLUP); // NRF CS
    // Park SD CS
    pinMode(12, OUTPUT); digitalWrite(12, HIGH);
}

void HatManager::park_for_nrf24() {
    // Hydra NRF uses CS on 6. Park LoRa NSS and CC1101 CS.
    pinMode(5, INPUT_PULLUP);
    pinMode(13, INPUT_PULLUP);
    // Park SD CS
    pinMode(12, OUTPUT); digitalWrite(12, HIGH);
}
