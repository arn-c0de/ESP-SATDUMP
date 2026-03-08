#include "launcher.h"
#include "config.h"
#include "display_utils.h"
#include "encoder.h"

#include <Arduino.h>
#include <SPI.h>
#include <SD.h>
#include <Update.h>
#include <ctype.h>
#include <string.h>

#include "nvs_flash.h"
#include "nvs.h"
#include "esp_system.h"

#define SD_MISO_PIN  12
#define SD_MOSI_PIN  14
#define SD_SCK_PIN   22
#define SD_CS_PIN    13

static SPIClass sdSPI(HSPI);
static bool sdMounted = false;

#define NVS_NS       "launcher"
#define NVS_KEY      "boot_flag"
#define MAX_FW       32

struct FwEntry {
    char   name[64];
    char   path[128];
    size_t size;
};

static const char* FW_SCAN_DIRS[] = { "/firmware", "/firmwares", "/" };

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

static void _drawMenu(int sel, const char** opts, int n, const char* title, int scrollOffset) {
    int16_t W = tft.width();
    int16_t H = tft.height();
    tft.fillScreen(COL_BG);

    tft.fillRect(0, 0, W, 34, COL_STATUS_BG);
    tft.setTextColor(COL_ACCENT, COL_STATUS_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX(title, 2), 8);
    tft.print(title);

    tft.fillRect(0, H - 24, W, 24, COL_STATUS_BG);
    tft.setTextColor(COL_TEXT, COL_STATUS_BG);
    tft.setTextSize(1);
    const char* hint = "ROT: Navigieren | CLICK: Bestaetigen";
    tft.setCursor(centeredX(hint, 1), H - 16);
    tft.print(hint);

    int16_t y = 42;
    int16_t rowH = 34;
    int maxVisible = (H - 42 - 28) / rowH;
    if (maxVisible < 1) maxVisible = 1;

    for (int i = scrollOffset; i < n && i < scrollOffset + maxVisible; i++) {
        bool active = (i == sel);
        uint16_t bg = active ? COL_ACCENT : COL_BG;
        uint16_t fg = active ? COL_BG : COL_TEXT;

        tft.fillRect(10, y, W - 20, rowH - 2, bg);
        tft.setTextColor(fg, bg);
        tft.setTextSize(2);
        tft.setCursor(18, y + 7);
        tft.print(active ? "> " : "  ");

        char label[28];
        label[0] = '\0';
        strlcpy(label, opts[i], sizeof(label));
        if (strlen(opts[i]) >= sizeof(label)) {
            size_t l = strlen(label);
            if (l > 3) {
                label[l - 1] = '\0';
                label[l - 2] = '.';
                label[l - 3] = '.';
                label[l - 4] = '.';
            }
        }
        tft.print(label);
        y += rowH;
    }
}

static int _waitForSelection(const char** opts, int n, const char* title) {
    int sel = 0;
    int scrollOffset = 0;

    while (true) {
        int16_t H = tft.height();
        int rowH = 34;
        int maxVisible = (H - 42 - 28) / rowH;
        if (maxVisible < 1) maxVisible = 1;

        if (sel < scrollOffset) scrollOffset = sel;
        if (sel >= scrollOffset + maxVisible) scrollOffset = sel - maxVisible + 1;

        _drawMenu(sel, opts, n, title, scrollOffset);

        while (true) {
            EncEvent ev = encoderRead();
            if (ev == EncEvent::CW) {
                sel = (sel + 1) % n;
                break;
            }
            if (ev == EncEvent::CCW) {
                sel = (sel - 1 + n) % n;
                break;
            }
            if (ev == EncEvent::CLICK) return sel;
            delay(5);
        }
    }
}

static void _showMessage(const char* line1, const char* line2 = nullptr, uint16_t color = COL_ACCENT) {
    int16_t H = tft.height();
    tft.fillScreen(COL_BG);
    tft.setTextColor(color, COL_BG);
    tft.setTextSize(2);

    int16_t y = line2 ? (H / 2 - 20) : (H / 2 - 10);
    tft.setCursor(centeredX(line1, 2), y);
    tft.print(line1);

    if (line2) {
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(centeredX(line2, 1), y + 28);
        tft.print(line2);
    }
}

static bool _mountSd() {
    if (sdMounted) return true;

    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);
    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, -1);

    const uint32_t freqs[] = { 4000000, 8000000, 1000000 };
    for (uint8_t i = 0; i < (sizeof(freqs) / sizeof(freqs[0])); i++) {
        if (SD.begin(SD_CS_PIN, sdSPI, freqs[i])) {
            sdMounted = true;
            return true;
        }
    }

    SD.end();
    sdSPI.end();
    return false;
}

static void _unmountSd() {
    if (!sdMounted) return;
    SD.end();
    sdSPI.end();
    sdMounted = false;
}

static bool _endsWithBin(const char* s) {
    if (!s) return false;
    size_t n = strlen(s);
    if (n < 4) return false;
    const char* ext = s + n - 4;
    return (strcasecmp(ext, ".bin") == 0);
}

static bool _isDuplicateName(const FwEntry* entries, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcasecmp(entries[i].name, name) == 0) return true;
    }
    return false;
}

static int _scanDir(const char* dirPath, FwEntry* entries, int count) {
    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        return count;
    }

    while (count < MAX_FW) {
        File f = dir.openNextFile();
        if (!f) break;

        const char* nm = f.name();
        const char* base = strrchr(nm, '/');
        base = base ? (base + 1) : nm;

        if (!f.isDirectory() && _endsWithBin(base) && !_isDuplicateName(entries, count, base)) {
            strlcpy(entries[count].name, base, sizeof(entries[count].name));
            if (strcmp(dirPath, "/") == 0) {
                snprintf(entries[count].path, sizeof(entries[count].path), "/%s", base);
            } else {
                snprintf(entries[count].path, sizeof(entries[count].path), "%s/%s", dirPath, base);
            }
            entries[count].size = f.size();
            count++;
        }

        f.close();
    }

    dir.close();
    return count;
}

static int _listFirmware(FwEntry* entries) {
    int count = 0;
    for (uint8_t i = 0; i < (sizeof(FW_SCAN_DIRS) / sizeof(FW_SCAN_DIRS[0])) && count < MAX_FW; i++) {
        count = _scanDir(FW_SCAN_DIRS[i], entries, count);
    }
    return count;
}

static bool _flashFile(const char* path, size_t sz) {
    static uint8_t buf[4096];

    File f = SD.open(path, FILE_READ);
    if (!f || f.isDirectory()) {
        if (f) f.close();
        return false;
    }

    if (sz == 0) sz = f.size();
    if (sz == 0) {
        f.close();
        return false;
    }

    if (!Update.begin(sz, U_FLASH)) {
        f.close();
        return false;
    }

    int16_t W = tft.width();
    int16_t H = tft.height();
    int16_t bx = 16;
    int16_t by = H / 2 + 8;
    int16_t bw = W - 32;
    int16_t bh = 18;

    tft.fillScreen(COL_BG);
    tft.fillRect(0, 0, W, 34, COL_STATUS_BG);
    tft.setTextColor(COL_ACCENT, COL_STATUS_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX("FLASHING", 2), 8);
    tft.print("FLASHING");

    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(16, by - 16);
    tft.print(path);

    tft.drawRect(bx - 1, by - 1, bw + 2, bh + 2, COL_TEXT);

    size_t written = 0;
    int lastPct = -1;

    while (f.available()) {
        size_t n = f.read(buf, sizeof(buf));
        if (n == 0) break;

        if (Update.write(buf, n) != n) {
            Update.abort();
            f.close();
            return false;
        }

        written += n;
        int pct = (int)((100UL * written) / sz);
        if (pct != lastPct) {
            int16_t fill = (int16_t)((long)bw * (long)written / (long)sz);
            tft.fillRect(bx, by, bw, bh, COL_STATUS_BG);
            tft.fillRect(bx, by, fill, bh, COL_ACCENT);

            char pctTxt[8];
            snprintf(pctTxt, sizeof(pctTxt), "%d%%", pct);
            tft.fillRect(0, by + bh + 8, W, 12, COL_BG);
            tft.setTextColor(COL_TEXT, COL_BG);
            tft.setTextSize(1);
            tft.setCursor(centeredX(pctTxt, 1), by + bh + 8);
            tft.print(pctTxt);
            lastPct = pct;
        }
    }

    f.close();

    if (written != sz) {
        Update.abort();
        return false;
    }

    if (!Update.end(true)) return false;
    return Update.isFinished();
}

void launcherCheckAndRun() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        nvs_flash_erase();
        nvs_flash_init();
    }

    bool forceFlag = _readBootFlag();
    bool btnHeld = (digitalRead(BTN_ROT_PIN) == LOW);
    if (!forceFlag && !btnHeld) return;

    _clearBootFlag();

    while (true) {
        const char* mainOpts[] = { "Boot ESP-SATDUMP", "SD Card Browser" };
        int mainSel = _waitForSelection(mainOpts, 2, "OS LAUNCHER");

        if (mainSel == 0) {
            _unmountSd();
            return;
        }

        if (!_mountSd()) {
            _showMessage("SD Fail!", "Check card/wiring", COL_RED);
            delay(1500);
            continue;
        }

        static FwEntry entries[MAX_FW];
        int count = _listFirmware(entries);
        if (count <= 0) {
            _showMessage("No firmware found", "Check /firmware, /firmwares, /", COL_RED);
            delay(2000);
            _unmountSd();
            continue;
        }

        static const char* names[MAX_FW];
        for (int i = 0; i < count; i++) names[i] = entries[i].name;

        int fwSel = _waitForSelection(names, count, "SD CARD BROWSER");
        bool ok = _flashFile(entries[fwSel].path, entries[fwSel].size);

        _unmountSd();

        if (ok) {
            _showMessage("Done! Rebooting...");
            delay(1000);
            esp_restart();
        } else {
            _showMessage("Flash failed", "See serial for details", COL_RED);
            delay(2000);
        }
    }
}
