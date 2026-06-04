/*
 * nrf52_hw.cpp — UART bridge to Adafruit Feather nRF52840 Bluefruit.
 *
 * Uses UART2 on the top hat header (G3 TX, G4 RX) at 115200 baud.
 * (UART1 is owned by GPS — see comment at the nrfSerial decl.)
 * The Feather is just another "hat" — mutually exclusive with
 * LoRa-1262, Hydra RF Cap, and C5 node. No pin conflicts.
 */
#include "nrf52_hw.h"
#include "nrf52_led.h"

/* UART2 — dedicated to the nRF52 link. UART1 is owned by the GPS
 * driver (gps.cpp instantiates its own HardwareSerial(1) at boot,
 * and the gps_task polls it every 100 ms). Two HardwareSerial objects
 * pointing at the same peripheral fight over pins and RX buffer —
 * reconfiguring UART1 here stomped GPS, and gps_task would also eat
 * inbound bytes (e.g. PONG) before this driver could read them.
 * UART2 is independent hardware, so both can coexist. */
static HardwareSerial nrfSerial(2);
static bool s_up = false;

/* Top hat header on Cardputer-Adv. G3+G4 chosen — adjacent pins,
 * clean cable run. Conflicts with LoRa hat (G3=RST, G4=DIO1) so the
 * two are mutually exclusive at the physical hat level. */
#define NRF52_UART_RX   4  /* G4: Feather TX → ESP32 RX */
#define NRF52_UART_TX   3  /* G3: ESP32 TX → Feather RX */
#define NRF52_BAUD     115200

bool NRF52Hardware::begin()
{
    if (s_up) return true;

    nrfSerial.begin(NRF52_BAUD, SERIAL_8N1, NRF52_UART_RX, NRF52_UART_TX);

    /* Give the Feather a moment to boot if we just powered it. The
     * nRF52840 cold-boots in ~200ms with our custom firmware. */
    delay(250);

    /* Drain any stale data from a previous session or Feather reboot. */
    while (nrfSerial.available()) nrfSerial.read();

    /* Handshake: PING → PONG confirms the Feather is alive and
     * running our firmware (not stock Adafruit bootloader). */
    String resp;
    if (send_command("PING", resp) && resp.indexOf("PONG") >= 0) {
        s_up = true;
        Serial.println("[nrf52] Feather Bluefruit detected on UART2 (G3/G4)");

        /* Light it up — boot animation on the NeoPixel. */
        nrf52_led_set(NRF52_LED_BOOT);

        return true;
    }

    Serial.println("[nrf52] Feather not found — wrong hat or no firmware");
    nrfSerial.end();
    return false;
}

void NRF52Hardware::end()
{
    if (!s_up) return;

    /* Kill the NeoPixel before disconnecting. */
    nrf52_led_set(NRF52_LED_OFF);
    delay(30);

    nrfSerial.end();
    s_up = false;
    Serial.println("[nrf52] disconnected");
}

bool NRF52Hardware::is_up()
{
    return s_up;
}

bool NRF52Hardware::send_command(const char *cmd, String &response,
                                uint32_t timeout_ms)
{
    /* Allow PING even when not "up" — it's the detection probe. */
    if (!s_up && strcmp(cmd, "PING") != 0) return false;

    Serial.printf("[nrf52] TX> %s\n", cmd);
    nrfSerial.println(cmd);
    uint32_t start = millis();
    response = "";
    int bytes_in = 0;

    while (millis() - start < timeout_ms) {
        if (nrfSerial.available()) {
            String line = nrfSerial.readStringUntil('\n');
            bytes_in += line.length();
            line.trim();
            Serial.printf("[nrf52] RX< '%s'\n", line.c_str());
            if (line == "OK") return true;
            if (line.startsWith("ERR")) return false;
            if (line.length() > 0) {
                response += line + "\n";
            }
        }
        delay(2);
    }
    Serial.printf("[nrf52] TIMEOUT after %lu ms, bytes_in=%d\n",
                  (unsigned long)timeout_ms, bytes_in);
    return false;
}

bool NRF52Hardware::available()
{
    return s_up && nrfSerial.available() > 0;
}

String NRF52Hardware::read_line()
{
    if (!s_up) return "";
    String line = nrfSerial.readStringUntil('\n');
    line.trim();
    return line;
}
