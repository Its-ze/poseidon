/*
 * sd_helper.cpp — single point of truth for M5Cardputer SD access.
 *
 * Cardputer SD wiring (K126):
 *   SCK  GPIO 40
 *   MISO GPIO 39
 *   MOSI GPIO 14
 *   CS   GPIO 12
 * Uses the FSPI peripheral. Plain SPI.begin() with default pins
 * doesn't work — that's why SD.begin() silently fails everywhere.
 */
#include "sd_helper.h"
#include <SD.h>
#include <SPI.h>
#include <FS.h>
#include <new>
#include <esp_vfs_fat.h>
#include <diskio_impl.h>
#include <sdmmc_cmd.h>

#define SD_SCK   40
#define SD_MISO  39
#define SD_MOSI  14
#define SD_CS    12
#define SD_FREQ  20000000  /* 20 MHz — reliable on 5cm ribbon in the Cardputer */

/* Dedicated SPI bus for SD. The M5Cardputer display (M5GFX) claims
 * FSPI/SPI2 for the TFT — confirmed by checking M5GFX init paths.
 * That leaves HSPI/SPI3 free for us. */
static SPIClass sd_spi(HSPI);

static bool s_mounted = false;

bool sd_is_mounted(void) { return s_mounted; }

SPIClass &sd_get_spi(void) { return sd_spi; }

bool sd_remount(void)
{
    s_mounted = false;
    SD.end();
    sd_spi.end();
    delay(10);
    return sd_mount();
}

static bool try_mount(int hz, bool fmt_if_fail, const char *tag)
{
    SD.end();
    sd_spi.end();

    /* SD cards in SPI mode require pull-ups on MISO, CS, and MOSI.
     * The Cardputer board doesn't include them and Arduino's
     * SPI.begin() doesn't enable internal pull-ups by default.
     * Force them on before SPI takes the pins over. */
    pinMode(SD_MISO, INPUT_PULLUP);
    pinMode(SD_MOSI, INPUT_PULLUP);
    pinMode(SD_CS,   INPUT_PULLUP);
    pinMode(SD_SCK,  INPUT_PULLUP);
    /* Toggle CS high so card sees idle state. */
    pinMode(SD_CS, OUTPUT);
    digitalWrite(SD_CS, HIGH);
    delay(10);

    sd_spi.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    /* max_files=2 (was 5). Each handle reserves ~600 B of FATFS state +
     * a sector buffer slot. Our features only ever hold 1-2 files open
     * simultaneously (CSV log + transient read), so 5 was wasted heap.
     * Reclaiming ~2 KB matters when WiFi has already eaten the rest. */
    bool ok = SD.begin(SD_CS, sd_spi, hz, "/sd", 2, fmt_if_fail);
    Serial.printf("[sd] %-12s @ %d Hz fmt=%d -> %s\n", tag, hz, fmt_if_fail, ok ? "OK" : "FAIL");
    return ok;
}

bool sd_mount(void)
{
    if (s_mounted) return true;

    /* HSPI only. NEVER probe FSPI — that's M5GFX's bus for the TFT, and
     * rebinding sd_spi to FSPI (as the prior Tier 2/3 code did) calls
     * SPIClass::begin on a bus the display driver is actively using,
     * which trashes the display SPI state and causes subsequent
     * fillScreen/draw calls to block forever on a transaction
     * semaphore. We tolerate "no SD" gracefully — destructive format-
     * on-fail is reachable via the Tools -> SD Format menu, not boot. */
    if (try_mount(SD_FREQ,    false, "HSPI fast")) { s_mounted = true; return true; }
    if (try_mount(10000000,   false, "HSPI half")) { s_mounted = true; return true; }
    if (try_mount( 4000000,   false, "HSPI slow")) { s_mounted = true; return true; }

    return false;
}

File sdlog_open(const char *stem, const char *header_line,
                char *out_path, size_t out_path_sz)
{
    File empty;
    if (!stem || !*stem) return empty;
    if (!sd_mount()) return empty;
    SD.mkdir("/poseidon");
    char path[64];
    snprintf(path, sizeof(path), "/poseidon/%s-%lu.csv",
             stem, (unsigned long)(millis() / 1000));
    File f = SD.open(path, FILE_WRITE);
    if (!f) return empty;
    if (header_line && *header_line) {
        f.println(header_line);
    }
    if (out_path && out_path_sz) {
        strncpy(out_path, path, out_path_sz - 1);
        out_path[out_path_sz - 1] = '\0';
    }
    return f;
}

void sd_rotate_on_size(const char *path, size_t max_bytes)
{
    if (!path || !*path) return;
    if (!sd_mount()) return;
    if (!SD.exists(path)) return;
    File f = SD.open(path, FILE_READ);
    if (!f) return;
    size_t sz = f.size();
    f.close();
    if (sz < max_bytes) return;

    /* Build "<path>.1" — single-generation rotation. */
    char rolled[80];
    snprintf(rolled, sizeof(rolled), "%s.1", path);
    if (SD.exists(rolled)) SD.remove(rolled);
    SD.rename(path, rolled);
}

bool sd_format(void)
{
    /* Use try_mount with format-on-fail = true so we hit the same
     * pull-up + CS-toggle init sequence as the regular mount path.
     * Without those, post-format the bus is left in a half-configured
     * state and subsequent SD.open() calls from features (e.g.
     * wardrive's CSV write) fail intermittently. */
    s_mounted = false;
    if (!try_mount(SD_FREQ, true,  "format fast"))
        if (!try_mount(4000000, true,  "format slow")) return false;
    /* Nuke contents: walk root and delete everything. Arduino SD
     * doesn't expose FAT format directly, so a full clean is the
     * closest user-meaningful equivalent. */
    File root = SD.open("/");
    if (root) {
        File f;
        while ((f = root.openNextFile())) {
            String path = f.path();
            bool is_dir = f.isDirectory();
            f.close();
            if (is_dir) SD.rmdir(path.c_str());
            else        SD.remove(path.c_str());
        }
        root.close();
    }
    s_mounted = true;
    return true;
}
