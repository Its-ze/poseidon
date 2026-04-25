/*
 * POSEIDON main — boot + splash + menu.
 */
#include "app.h"
#include "ui.h"
#include "input.h"
#include "menu.h"
#include "radio.h"
#include "sd_helper.h"
#include "gps.h"
#include "sfx.h"
#include "theme.h"
#include "c5_cmd.h"
#include "version.h"
#include "utility/Keyboard/KeyboardReader/TCA8418.h"

/* Strong override: tell Arduino-ESP32 core that BT is in use. Without
 * this the core calls esp_bt_controller_mem_release(ESP_BT_MODE_BTDM)
 * at boot (before app_main), releasing ~36KB of BT memory. After that
 * any esp_bt_controller_enable() will fail silently — NimBLE init
 * looks successful but the controller stays stuck at status 2 (INITED)
 * and BLE scans return zero devices. The weak default implementation
 * in esp32-hal-bt.c returns _btLibraryInUse which is only set to true
 * if a BT library includes esp32-hal-bt-mem.h — NimBLE-Arduino 2.5.0
 * does NOT include it, so on our stack we must opt in manually. */
extern "C" bool btInUse(void) { return true; }

/* Background GPS poller — keeps NMEA parsed continuously so that by
 * the time the user opens Wardrive or GPS fix page the last fix is
 * already fresh. Low priority, tiny stack. */
static void gps_task(void *_)
{
    while (1) {
        gps_poll();
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void setup()
{
    /* Cardputer-Adv hat compatibility: the CAP-LoRa1262 wires SX1262
     * NSS=G5, BUSY=G6, DIO1=G4, RST=G3. At power-on the SX1262 drives
     * BUSY HIGH until its ready, which breaks M5GFX's board autodetect
     * (it sets G5/G6 to input_pulldown and reads them to distinguish
     * Adv vs K126 vs VAMeter). Holding RST LOW keeps every SX1262
     * output in high-Z, so the Advs native pull-ups on G5/G6 decide
     * the probe and the Adv branch is taken correctly. We also park
     * G5 HIGH afterwards so the chip stays NSS-deselected on the
     * shared SPI bus (same bus as SD). The feature module will drive
     * G3 HIGH when LoRa is actually opened. */
    pinMode(3, OUTPUT);
    digitalWrite(3, LOW);
    delay(5);

    auto cfg = M5.config();
    M5Cardputer.begin(cfg, true);
    /* Safety belt: force the I2C (TCA8418) keyboard reader even if
     * autodetect picked K126, so the driver never grabs G3-G7. */
    M5Cardputer.Keyboard.begin(std::make_unique<TCA8418KeyboardReader>());
    /* Release pin 5 — LoRa hat uses it as NSS (needs HIGH to deselect),
     * Hydra hat uses it as CC1101 GDO0 (input). Since we don't know
     * which hat is attached, set INPUT_PULLUP which safely deselects
     * LoRa NSS (pulled high) and won't fight CC1101 GDO0 (input). */
    pinMode(5, INPUT_PULLUP);
    M5Cardputer.Display.setRotation(1);  /* landscape, keyboard at the bottom */
    Serial.begin(115200);
    delay(100);
    Serial.printf("\n[POSEIDON] %s (%s) boot\n",
                  poseidon_version(), poseidon_build_date());
    /* Confirm board detection — Adv uses TCA8418 I2C keyboard (G3-G7 free
     * for hats); original K126 uses GPIO matrix keyboard (G3-G7 claimed). */
    auto board = M5.getBoard();
    Serial.printf("[POSEIDON] board=%d (%s)\n", (int)board,
                  board == m5::board_t::board_M5CardputerADV ? "Cardputer-Adv" :
                  board == m5::board_t::board_M5Cardputer    ? "Cardputer K126" :
                                                               "UNKNOWN");

    /* Mount SD on boot if a card is present. Non-fatal if absent. */
    if (sd_mount()) Serial.println("[POSEIDON] sd mounted");
    else            Serial.println("[POSEIDON] sd absent");

    /* Load persisted sound settings + set speaker volume. */
    sfx_init();
    /* Load persisted theme before UI draws anything — otherwise the
     * splash + first menu render in POSEIDON's default instead of the
     * user's last pick. */
    theme_init();

    Serial.printf("[POSEIDON] boot heap free=%u KB\n",
                  (unsigned)(ESP.getFreeHeap() / 1024));

    /* Bring up the GNSS UART + background NMEA poller so GPS-consuming
     * features (Wardrive, R4 GPS fix) always have a recent snapshot. */
    gps_begin();
    xTaskCreate(gps_task, "gps", 3072, nullptr, 2, nullptr);

    ui_init();

    /* Deferred boot: c5_begin() used to fire here so the satellite
     * badge would light up before any feature was opened. But starting
     * WiFi at boot claims the coex slot before BLE gets a chance to
     * init — esp_bt_controller_enable then fails on first BLE scan
     * (status stuck at 2/INITED). Bruce doesn't run WiFi at boot
     * either. c5_begin is idempotent and called from every WiFi
     * feature entry, so moving it out of boot costs us only the
     * pre-feature satellite badge. */
#ifdef POSEIDON_AUTO_DEAUTH_TEST
    {
        extern void poseidon_autotest_show_last_crash(void);
        poseidon_autotest_show_last_crash();
    }
#endif

#ifdef POSEIDON_AUTO_DEAUTH_TEST
    /* No-UI deauth self-test: 3 stages (basic, burst, triton-hop).
     * Marks progress to NVS so a reboot trace is visible on next boot
     * without serial. Skip splash so we get feedback fast. */
    {
        extern void poseidon_deauth_autotest(void);
        poseidon_deauth_autotest();
    }
#endif

    ui_splash();  /* animates, then waits for a key press internally */
}

void loop()
{
    menu_run();
    /* menu_run only returns on a quit — rare. Fall through to a
     * quiescent poll loop so the device doesn't deadlock. */
    ui_clear_body();
    ui_toast("menu exited", COL_WARN, 800);
    delay(200);
}
