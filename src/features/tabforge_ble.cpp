/*
 * TabForge Bluetooth integration.
 *
 * BT Keyboard exposes the Cardputer keyboard as a conventional BLE HID
 * keyboard. TabForge Direct connects to the private Tab5 GATT service and
 * sends authenticated keyboard events plus semantic remote-display frames.
 */
#include "app.h"
#include "../theme.h"
#include "ui.h"
#include "input.h"
#include "radio.h"
#include <NimBLEDevice.h>
#include <NimBLEHIDDevice.h>
#include <HIDTypes.h>
#include <HIDKeyboardTypes.h>
#include <esp_heap_caps.h>
#include <esp_wifi.h>

static const char *TF_SERVICE_UUID = "7d2ea001-2e7b-4c77-9b1c-4c9b8fdf5000";
static const char *TF_RX_UUID      = "7d2ea001-2e7b-4c77-9b1c-4c9b8fdf5001";
static const char *TF_TX_UUID      = "7d2ea001-2e7b-4c77-9b1c-4c9b8fdf5002";

static const uint8_t TF_HID_REPORT_MAP[] = {
    USAGE_PAGE(1),       0x01,
    USAGE(1),            0x06,
    COLLECTION(1),       0x01,
    REPORT_ID(1),        0x01,
    USAGE_PAGE(1),       0x07,
    USAGE_MINIMUM(1),    0xE0,
    USAGE_MAXIMUM(1),    0xE7,
    LOGICAL_MINIMUM(1),  0x00,
    LOGICAL_MAXIMUM(1),  0x01,
    REPORT_SIZE(1),      0x01,
    REPORT_COUNT(1),     0x08,
    HIDINPUT(1),         0x02,
    REPORT_COUNT(1),     0x01,
    REPORT_SIZE(1),      0x08,
    HIDINPUT(1),         0x01,
    REPORT_COUNT(1),     0x06,
    REPORT_SIZE(1),      0x08,
    LOGICAL_MINIMUM(1),  0x00,
    LOGICAL_MAXIMUM(1),  0x65,
    USAGE_PAGE(1),       0x07,
    USAGE_MINIMUM(1),    0x00,
    USAGE_MAXIMUM(1),    0x65,
    HIDINPUT(1),         0x00,
    END_COLLECTION(0),
};

static NimBLEHIDDevice *s_keyboard_hid;
static NimBLECharacteristic *s_keyboard_input;
static volatile bool s_keyboard_connected;

class tf_keyboard_server_cb : public NimBLEServerCallbacks {
    void onConnect(NimBLEServer *, NimBLEConnInfo &) override
    {
        s_keyboard_connected = true;
    }

    void onDisconnect(NimBLEServer *, NimBLEConnInfo &, int) override
    {
        s_keyboard_connected = false;
        NimBLEDevice::startAdvertising();
    }
};

static tf_keyboard_server_cb s_keyboard_server_cb;

static void release_wifi_for_ble(void)
{
    radio_switch(RADIO_NONE);
    esp_wifi_stop();
    esp_wifi_deinit();
    delay(100);
}

static bool side_button_pressed(void)
{
    return M5Cardputer.BtnA.wasPressed();
}

static void draw_keyboard_page(void)
{
    auto &d = M5Cardputer.Display;
    ui_force_clear_body();
    d.setTextColor(T_ACCENT, T_BG);
    d.setCursor(4, BODY_Y + 2);
    d.print("BT KEYBOARD");
    d.drawFastHLine(4, BODY_Y + 12, 112, T_ACCENT);
    d.setTextColor(T_FG, T_BG);
    d.setCursor(4, BODY_Y + 24);
    d.print("name: Cardputer Keyboard");
    d.setTextColor(T_DIM, T_BG);
    d.setCursor(4, BODY_Y + 38);
    d.print("Pair in the host Bluetooth menu.");
    d.setCursor(4, BODY_Y + 50);
    d.print("All keys + modifiers are forwarded.");
    ui_draw_footer("side button = disconnect / back");
}

static void setup_keyboard_hid(void)
{
    NimBLEDevice::init("Cardputer Keyboard");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);

    NimBLEServer *server = NimBLEDevice::createServer();
    server->setCallbacks(&s_keyboard_server_cb);
    s_keyboard_hid = new NimBLEHIDDevice(server);
    s_keyboard_input = s_keyboard_hid->getInputReport(1);
    s_keyboard_hid->setManufacturer("TabForge");
    s_keyboard_hid->setPnp(0x02, 0x303A, 0x1001, 0x0100);
    s_keyboard_hid->setHidInfo(0x00, 0x01);
    s_keyboard_hid->setReportMap((uint8_t *)TF_HID_REPORT_MAP, sizeof(TF_HID_REPORT_MAP));
    s_keyboard_hid->startServices();

    NimBLEAdvertising *advertising = NimBLEDevice::getAdvertising();
    advertising->setAppearance(HID_KEYBOARD);
    advertising->addServiceUUID(s_keyboard_hid->getHidService()->getUUID());
    advertising->start();
}

static void send_native_keyboard_report(void)
{
    if (!s_keyboard_connected || !s_keyboard_input) return;

    Keyboard_Class::KeysState &state = M5Cardputer.Keyboard.keysState();
    uint8_t report[8] = {state.modifiers, 0, 0, 0, 0, 0, 0, 0};
    size_t count = state.hid_keys.size();
    if (count > 6) count = 6;
    for (size_t i = 0; i < count; ++i) report[2 + i] = state.hid_keys[i];
    s_keyboard_input->setValue(report, sizeof(report));
    s_keyboard_input->notify();
}

void feat_ble_keyboard(void)
{
    release_wifi_for_ble();
    setup_keyboard_hid();
    draw_keyboard_page();

    bool last_connected = false;
    uint32_t last_draw = 0;
    while (true) {
        M5Cardputer.update();
        if (side_button_pressed()) break;

        if (M5Cardputer.Keyboard.isChange()) {
            send_native_keyboard_report();
        }

        if (last_connected != s_keyboard_connected || millis() - last_draw > 500) {
            last_connected = s_keyboard_connected;
            last_draw = millis();
            auto &d = M5Cardputer.Display;
            d.fillRect(0, BODY_Y + 68, SCR_W, 18, T_BG);
            d.setTextColor(s_keyboard_connected ? T_GOOD : T_WARN, T_BG);
            d.setCursor(4, BODY_Y + 68);
            d.print(s_keyboard_connected ? "CONNECTED - typing live" : "ADVERTISING - waiting for host");
            ui_draw_status("bt-keyboard", s_keyboard_connected ? "paired" : "ready");
        }
        delay(8);
    }

    if (s_keyboard_input && s_keyboard_connected) {
        uint8_t released[8] = {0};
        s_keyboard_input->setValue(released, sizeof(released));
        s_keyboard_input->notify();
    }
    NimBLEDevice::getAdvertising()->stop();
    s_keyboard_connected = false;
    if (s_keyboard_hid) {
        delete s_keyboard_hid;
        s_keyboard_hid = nullptr;
    }
    s_keyboard_input = nullptr;
    radio_switch(RADIO_BLE);
    radio_switch(RADIO_NONE);
}

static NimBLEClient *s_direct_client;
static NimBLERemoteCharacteristic *s_direct_rx;
static NimBLERemoteCharacteristic *s_direct_tx;
static volatile bool s_direct_disconnected;

class tf_direct_client_cb : public NimBLEClientCallbacks {
    void onDisconnect(NimBLEClient *, int) override
    {
        s_direct_disconnected = true;
    }
};

static tf_direct_client_cb s_direct_client_cb;

static bool direct_write(const char *json)
{
    if (!s_direct_rx || !s_direct_client || !s_direct_client->isConnected()) return false;
    return s_direct_rx->writeValue((uint8_t *)json, strlen(json), true);
}

static void json_escape_char(char c, char out[3])
{
    if (c == '"' || c == '\\') {
        out[0] = '\\';
        out[1] = c;
        out[2] = '\0';
    } else {
        out[0] = c;
        out[1] = '\0';
    }
}

static void direct_send_key(const char *key, const char *text, uint8_t modifiers, uint8_t hid)
{
    char escaped[3] = "";
    if (text && text[0]) json_escape_char(text[0], escaped);
    char json[192];
    snprintf(json, sizeof(json),
             "{\"tabforge\":\"cardputer.keyboard\",\"device\":\"cardputer\","
             "\"key\":\"%.12s\",\"text\":\"%.2s\",\"modifiers\":%u,\"hid\":%u}",
             key ? key : "", escaped, (unsigned)modifiers, (unsigned)hid);
    direct_write(json);
}

static void direct_send_frame(uint32_t key_count, const char *status)
{
    char json[256];
    snprintf(json, sizeof(json),
             "{\"tabforge\":\"cardputer.remote.frame\",\"device\":\"cardputer\","
             "\"title\":\"POSEIDON Bluetooth Link\","
             "\"body\":\"Cardputer keyboard active. %lu key events sent.\","
             "\"status\":\"%.72s\"}",
             (unsigned long)key_count, status ? status : "Connected to TabForge Tab5.");
    direct_write(json);
}

class tf_direct_scan_cb : public NimBLEScanCallbacks {
public:
    bool found = false;
    NimBLEAddress address;

    void onResult(const NimBLEAdvertisedDevice *device) override
    {
        if (found || !device->isConnectable()) return;
        if (device->haveServiceUUID() && device->isAdvertisingService(NimBLEUUID(TF_SERVICE_UUID))) {
            address = device->getAddress();
            found = true;
            NimBLEDevice::getScan()->stop();
        }
    }
};

static bool direct_connect(void)
{
    tf_direct_scan_cb scan_cb;
    NimBLEScan *scan = NimBLEDevice::getScan();
    scan->setScanCallbacks(&scan_cb, false);
    scan->setActiveScan(false);
    scan->setInterval(80);
    scan->setWindow(50);
    /* Cardputer-Adv has no usable PSRAM. Keep advertisements callback-only
     * so security/AES still has a contiguous internal-heap block. */
    scan->setMaxResults(0);
    scan->getResults(12000, false);
    bool found = scan_cb.found;
    NimBLEAddress address = scan_cb.address;
    scan->clearResults();
    scan->setScanCallbacks(nullptr);
    if (!found) return false;

    Serial.printf("[tab-direct] pre-connect heap=%u largest=%u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));

    s_direct_client = NimBLEDevice::createClient();
    s_direct_client->setClientCallbacks(&s_direct_client_cb, false);
    s_direct_client->setConnectTimeout(8000);
    if (!s_direct_client->connect(address)) return false;
    Serial.printf("[tab-direct] pre-security heap=%u largest=%u\n",
                  (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                  (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL));
    if (!s_direct_client->secureConnection()) return false;

    NimBLERemoteService *service = s_direct_client->getService(TF_SERVICE_UUID);
    if (!service) return false;
    s_direct_rx = service->getCharacteristic(TF_RX_UUID);
    s_direct_tx = service->getCharacteristic(TF_TX_UUID);
    return s_direct_rx && (s_direct_rx->canWrite() || s_direct_rx->canWriteNoResponse());
}

static bool direct_pair(const char *code, char *ack, size_t ack_size)
{
    char json[192];
    snprintf(json, sizeof(json),
             "{\"tabforge\":\"cardputer.remote.pair\",\"device\":\"cardputer\",\"code\":\"%.7s\"}",
             code);
    if (!direct_write(json)) return false;
    uint32_t deadline = millis() + 2500;
    while (s_direct_tx && s_direct_tx->canRead() && (int32_t)(deadline - millis()) > 0) {
        delay(100);
        std::string response = s_direct_tx->readValue();
        strlcpy(ack, response.c_str(), ack_size);
        if (response.find("\"state\":\"paired\"") != std::string::npos) return true;
        if (response.find("\"state\":\"pair_failed\"") != std::string::npos) return false;
    }
    return false;
}

static void draw_direct_page(const char *state, bool good)
{
    auto &d = M5Cardputer.Display;
    ui_force_clear_body();
    d.setTextColor(T_ACCENT, T_BG);
    d.setCursor(4, BODY_Y + 2);
    d.print("TABFORGE DIRECT");
    d.drawFastHLine(4, BODY_Y + 12, 132, T_ACCENT);
    d.setTextColor(good ? T_GOOD : T_WARN, T_BG);
    d.setCursor(4, BODY_Y + 24);
    d.print(state);
    d.setTextColor(T_FG, T_BG);
    d.setCursor(4, BODY_Y + 40);
    d.print("Keys control the active Tab text field.");
    d.setTextColor(T_DIM, T_BG);
    d.setCursor(4, BODY_Y + 54);
    d.print("Tab5 renders link status + app frames.");
    ui_draw_footer("side button = end direct session");
}

void feat_tabforge_direct(void)
{
    char pair_code[8];
    uint32_t key_count = 0;
    uint32_t last_frame = 0;
    if (!input_line("Tab5 four-digit code:", pair_code, sizeof(pair_code))) return;

    release_wifi_for_ble();
    NimBLEDevice::init("POSEIDON Direct");
    NimBLEDevice::setPower(ESP_PWR_LVL_P9);
    NimBLEDevice::setSecurityAuth(true, false, true);
    NimBLEDevice::setSecurityIOCap(BLE_HS_IO_NO_INPUT_OUTPUT);
    s_direct_disconnected = false;
    s_direct_rx = nullptr;
    s_direct_tx = nullptr;

    draw_direct_page("Scanning for TabForge Tab5...", false);
    if (!direct_connect()) {
        draw_direct_page("Tab5 not found. Open Card Display.", false);
        delay(1800);
        goto cleanup;
    }

    {
        char ack[224] = "";
        draw_direct_page("Connected. Authorizing code...", false);
        if (!direct_pair(pair_code, ack, sizeof(ack))) {
            draw_direct_page("Pair code rejected by Tab5.", false);
            delay(1800);
            goto cleanup;
        }
    }

    draw_direct_page("PAIRED - direct keyboard active", true);
    direct_send_frame(0, "Authenticated Bluetooth direct session.");
    last_frame = millis();

    while (!s_direct_disconnected && s_direct_client && s_direct_client->isConnected()) {
        M5Cardputer.update();
        if (side_button_pressed()) break;

        if (M5Cardputer.Keyboard.isChange() && M5Cardputer.Keyboard.isPressed()) {
            Keyboard_Class::KeysState &state = M5Cardputer.Keyboard.keysState();
            uint8_t hid = state.hid_keys.empty() ? 0 : state.hid_keys[0];
            if (state.fn && !state.word.empty()) {
                switch (state.word[0]) {
                case '`': direct_send_key("escape", "", state.modifiers, hid); break;
                case ';': direct_send_key("up", "", state.modifiers, hid); break;
                case '.': direct_send_key("down", "", state.modifiers, hid); break;
                case ',': direct_send_key("left", "", state.modifiers, hid); break;
                case '/': direct_send_key("right", "", state.modifiers, hid); break;
                default: break;
                }
            } else if (state.del) {
                direct_send_key("backspace", "", state.modifiers, hid);
            } else if (state.enter) {
                direct_send_key("enter", "", state.modifiers, hid);
            } else if (state.tab) {
                direct_send_key("tab", "", state.modifiers, hid);
            } else if (!state.word.empty() && !state.ctrl && !state.alt) {
                for (char c : state.word) {
                    char text[2] = {c, '\0'};
                    direct_send_key("", text, state.modifiers, hid);
                }
            } else if (hid != 0) {
                direct_send_key("hid", "", state.modifiers, hid);
            }
            key_count++;
        }

        if (millis() - last_frame > 1500) {
            last_frame = millis();
            direct_send_frame(key_count, "Direct keyboard and remote display linked.");
            ui_draw_status("tab-direct", "paired");
        }
        delay(8);
    }

    direct_write("{\"tabforge\":\"cardputer.remote.end\",\"device\":\"cardputer\"}");

cleanup:
    if (s_direct_client) {
        if (s_direct_client->isConnected()) s_direct_client->disconnect();
        NimBLEDevice::deleteClient(s_direct_client);
        s_direct_client = nullptr;
    }
    s_direct_rx = nullptr;
    s_direct_tx = nullptr;
    if (NimBLEDevice::isInitialized()) NimBLEDevice::deinit(true);
    radio_switch(RADIO_NONE);
}
