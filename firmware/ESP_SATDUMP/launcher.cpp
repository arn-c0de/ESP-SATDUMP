#include "launcher.h"
#include "config.h"
#include "display_utils.h"
#include "encoder.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Update.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

// ── SD card SPI pins (VSPI — separate from TFT HSPI on GPIO 18/19/23) ────────
// Wire your SD module to these pins:
//   SD_MOSI → GPIO13,  SD_MISO → GPIO21,  SD_SCK → GPIO22,  SD_CS → GPIO27
#define SD_MOSI_PIN  13
#define SD_MISO_PIN  21
#define SD_SCK_PIN   22
#define SD_CS_PIN    27
#define FW_DIR       "/firmware"

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

    // Header bar
    tft.fillRect(0, 0, W, 30, COL_STATUS_BG);
    tft.setTextColor(COL_ACCENT, COL_STATUS_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX(title, 2), 7);
    tft.print(title);

    // Footer hint
    tft.setTextColor(COL_DIM, COL_BG);
    tft.setTextSize(1);
    const char* hint = "ROT=navigate  CLICK=confirm";
    tft.setCursor(centeredX(hint, 1), tft.height() - 12);
    tft.print(hint);

    // Menu items
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

// Blocks until encoder CLICK; returns selected index.
static int _waitForSelection(const char** opts, int n, const char* title) {
    _drawMenu(0, opts, n, title);
    int sel = 0;
    while (true) {
        EncEvent ev = encoderRead();
        if (ev == EncEvent::CW) {
            sel = (sel + 1) % n;
            _drawMenu(sel, opts, n, title);
        } else if (ev == EncEvent::CCW) {
            sel = (sel - 1 + n) % n;
            _drawMenu(sel, opts, n, title);
        } else if (ev == EncEvent::CLICK) {
            return sel;
        }
        delay(2);
    }
}

static void _showMessage(const char* line1, const char* line2 = nullptr,
                          uint16_t color = COL_ACCENT) {
    int16_t H = tft.height();
    tft.fillScreen(COL_BG);
    tft.setTextColor(color, COL_BG);
    tft.setTextSize(2);
    int16_t y = line2 ? H / 2 - 18 : H / 2 - 8;
    tft.setCursor(centeredX(line1, 2), y);
    tft.print(line1);
    if (line2) {
        tft.setTextColor(COL_DIM, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(centeredX(line2, 1), y + 26);
        tft.print(line2);
    }
}

// ── SD firmware listing ───────────────────────────────────────────────────────

static int _listFirmware(FwEntry* entries) {
    File dir = SD.open(FW_DIR);
    if (!dir || !dir.isDirectory()) return 0;

    int count = 0;
    while (count < MAX_FW) {
        File f = dir.openNextFile();
        if (!f) break;
        const char* nm = f.name();
        if (!f.isDirectory() && strstr(nm, ".bin")) {
            const char* base = strrchr(nm, '/');
            base = base ? base + 1 : nm;
            strlcpy(entries[count].name, base, sizeof(entries[count].name));
            snprintf(entries[count].path, sizeof(entries[count].path),
                     "%s/%s", FW_DIR, base);
            entries[count].size = f.size();
            count++;
        }
        f.close();
    }
    dir.close();
    return count;
}

// ── OTA flash via Arduino Update ─────────────────────────────────────────────

static bool _flashFile(const char* path, size_t sz) {
    File f = SD.open(path);
    if (!f) return false;

    if (!Update.begin(sz)) {
        f.close();
        return false;
    }

    int16_t W = tft.width(), H = tft.height();
    int16_t bx = 20, by = H / 2 + 10, bw = W - 40, bh = 22;
    tft.fillScreen(COL_BG);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX("FLASHING...", 2), H / 2 - 24);
    tft.print("FLASHING...");
    tft.drawRect(bx, by, bw, bh, COL_DIM);

    static uint8_t buf[4096];  // static: BSS, not stack
    size_t written = 0;
    while (f.available()) {
        size_t n = f.readBytes((char*)buf, sizeof(buf));
        if (!n) break;
        if (Update.write(buf, n) != n) { f.close(); return false; }
        written += n;

        int16_t fill = (int16_t)((long)bw * (long)written / (long)sz);
        tft.fillRect(bx, by, fill, bh, COL_ACCENT);

        char pct[8];
        snprintf(pct, sizeof(pct), "%d%%", (int)(100UL * written / sz));
        int16_t px = centeredX(pct, 1);
        tft.fillRect(px - 2, by + bh + 6, (int16_t)(strlen(pct) * 6 + 4), 10, COL_BG);
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(px, by + bh + 6);
        tft.print(pct);
    }
    f.close();

    if (!Update.end(true)) return false;
    return Update.isFinished();
}

// ── Main entry point ──────────────────────────────────────────────────────────

void launcherCheckAndRun() {
    // NVS must be initialized; Arduino ESP32 does this before setup(), but
    // calling init again is safe (returns ESP_ERR_NVS_NO_FREE_PAGES at worst).
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    bool forceFlag = _readBootFlag();
    bool btnHeld   = (digitalRead(BTN_ROT_PIN) == LOW);
    if (!forceFlag && !btnHeld) return;

    _clearBootFlag();

    // ── Main OS selection menu ────────────────────────────────────────────────
    const char* mainOpts[] = { "Boot ESP-SATDUMP", "Install from SD" };
    int choice = _waitForSelection(mainOpts, 2, "OS LAUNCHER");
    if (choice == 0) return; // boot normally, fall through to rest of setup()

    // ── Install flow ──────────────────────────────────────────────────────────
    _showMessage("Mounting SD...");

    SPIClass vspi(VSPI);
    vspi.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);

    if (!SD.begin(SD_CS_PIN, vspi, 4000000)) {
        _showMessage("SD Mount Failed!", "Check wiring & card", COL_RED);
        delay(3000);
        return;
    }

    static FwEntry entries[MAX_FW];  // static: BSS, not stack (3136 bytes)
    int count = _listFirmware(entries);

    if (count == 0) {
        SD.end();
        vspi.end();
        _showMessage("No .bin found!", "Put files in /firmware", COL_RED);
        delay(3000);
        return;
    }

    // ── Firmware selection menu ───────────────────────────────────────────────
    static const char* names[MAX_FW];  // static: BSS, not stack
    for (int i = 0; i < count; i++) names[i] = entries[i].name;

    int fwSel = _waitForSelection(names, count, "SELECT FIRMWARE");
    bool ok   = _flashFile(entries[fwSel].path, entries[fwSel].size);

    SD.end();
    vspi.end();

    if (ok) {
        _showMessage("Done! Rebooting...", nullptr, COL_GREEN);
        delay(1500);
        esp_restart();
    } else {
        _showMessage("Flash Failed!", Update.errorString(), COL_RED);
        delay(4000);
    }
}
