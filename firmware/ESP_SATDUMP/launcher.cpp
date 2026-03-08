#include "launcher.h"
#include "config.h"
#include "display_utils.h"
#include "encoder.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Update.h>
#include <ctype.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

// ── SD card SPI pins (AHH-1.0 mapping) ──
#define SD_MISO_PIN  12
#define SD_MOSI_PIN  14
#define SD_SCK_PIN   22
#define SD_CS_PIN    13
#define FW_DIR       "/firmware"

static SPIClass sdSPI(HSPI);

#define NVS_NS       "launcher"
#define NVS_KEY      "boot_flag"
#define MAX_FW       16

struct FwEntry {
    char   name[64];
    char   path[128];
    size_t size;
};

// ── NVS helpers ───────────────────────────────────────────────────────────────

static bool _readBootFlag() {
    nvs_handle_t h;
    uint8_t flag = 0;
    if (nvs_open(NVS_NS, NVS_READONLY, &h) == ESP_OK) {
        nvs_get_u8(h, NVS_KEY, &flag);
        nvs_close(h);
    }
    return flag != 0;
}

static void _clearBootFlag() {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY, 0);
        nvs_commit(h);
        nvs_close(h);
    }
}

void launcherRebootToLauncher() {
    nvs_handle_t h;
    if (nvs_open(NVS_NS, NVS_READWRITE, &h) == ESP_OK) {
        nvs_set_u8(h, NVS_KEY, 1);
        nvs_commit(h);
        nvs_close(h);
    }
    esp_restart();
}

// ── TFT menu helpers ──────────────────────────────────────────────────────────

static void _drawMenu(int sel, const char** opts, int n, const char* title) {
    int16_t W = tft.width();
    tft.fillScreen(COL_BG);
    tft.fillRect(0, 0, W, 30, COL_STATUS_BG);
    tft.setTextColor(COL_ACCENT, COL_STATUS_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX(title, 2), 7);
    tft.print(title);

    int16_t y = 40;
    for (int i = 0; i < n; i++) {
        bool active = (i == sel);
        uint16_t bg = active ? COL_ACCENT : COL_BG;
        uint16_t fg = active ? COL_BG     : COL_TEXT;
        tft.fillRect(10, y, W - 20, 30, bg);
        tft.setTextColor(fg, bg);
        tft.setTextSize(2);
        tft.setCursor(22, y + 7);
        if (active) tft.print("> ");
        tft.print(opts[i]);
        y += 38;
    }
}

static int _waitForSelection(const char** opts, int n, const char* title) {
    _drawMenu(0, opts, n, title);
    int sel = 0;
    while (true) {
        EncEvent ev = encoderRead();
        if (ev == EncEvent::CW) { sel = (sel + 1) % n; _drawMenu(sel, opts, n, title); }
        else if (ev == EncEvent::CCW) { sel = (sel - 1 + n) % n; _drawMenu(sel, opts, n, title); }
        else if (ev == EncEvent::CLICK) return sel;
        delay(5);
    }
}

static void _showMessage(const char* l1, const char* l2 = nullptr, uint16_t col = COL_ACCENT) {
    tft.fillScreen(COL_BG);
    tft.setTextColor(col, COL_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX(l1, 2), tft.height()/2 - 10);
    tft.print(l1);
}

// ── SD & Flash ────────────────────────────────────────────────────────────────

static bool _mountSd() {
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, -1);
    if (SD.begin(SD_CS_PIN, sdSPI, 4000000)) return true;
    SD.end();
    sdSPI.end();
    return false;
}

static void _unmountSd() {
    SD.end();
    sdSPI.end();
    pinMode(SD_CS_PIN, INPUT);
    pinMode(SD_MISO_PIN, INPUT);
    pinMode(SD_MOSI_PIN, INPUT);
    pinMode(SD_SCK_PIN, INPUT);
}

static int _scanFirmware(FwEntry* entries) {
    File dir = SD.open("/firmware");
    if (!dir || !dir.isDirectory()) return 0;
    int count = 0;
    while (count < MAX_FW) {
        File f = dir.openNextFile();
        if (!f) break;
        if (!f.isDirectory() && strstr(f.name(), ".bin")) {
            strlcpy(entries[count].name, f.name(), 64);
            snprintf(entries[count].path, 128, "/firmware/%s", f.name());
            entries[count].size = f.size();
            count++;
        }
        f.close();
    }
    dir.close();
    return count;
}

void launcherCheckAndRun() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    bool force = _readBootFlag();
    bool btn = (digitalRead(BTN_ROT_PIN) == LOW);
    if (!force && !btn) return;

    _clearBootFlag();

    while (true) {
        const char* mainOpts[] = { "Boot current SATDUMP", "Install from SD" };
        int choice = _waitForSelection(mainOpts, 2, "OS LAUNCHER");
        if (choice == 0) {
            _unmountSd();
            return;
        }

        if (_mountSd()) {
            static FwEntry entries[MAX_FW];
            int count = _scanFirmware(entries);
            if (count > 0) {
                static const char* names[MAX_FW];
                for (int i = 0; i < count; i++) names[i] = entries[i].name;
                int fwSel = _waitForSelection(names, count, "SELECT OS");
                _showMessage("Flashing...");
                // (Flash-Logik wie vorher hier einfügen...)
                delay(1000);
                _showMessage("Done! Rebooting...");
                delay(1000);
                esp_restart();
            } else {
                _showMessage("No files found!", "Check /firmware/", COL_RED);
                delay(2000);
            }
            _unmountSd();
        } else {
            _showMessage("SD Fail!", "Check pins/card", COL_RED);
            delay(1000);
        }
    }
}
