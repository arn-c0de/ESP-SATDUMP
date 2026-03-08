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

// ── SD card SPI pins (same physical wires as TFT, but uses HSPI peripheral) ──
// SD uses dedicated HSPI bus (MISO=12, MOSI=14, SCK=22, CS=13).
// TFT_eSPI claims the VSPI peripheral via spi_bus_initialize(), so SD must use
// the HSPI peripheral to avoid ESP-IDF bus conflicts.  The GPIO matrix routes
// HSPI (Alternative SPI bus) for SD card to avoid TFT bus interference.
#define SD_MISO_PIN  12
#define SD_MOSI_PIN  14
#define SD_SCK_PIN   22
#define SD_CS_PIN    13
#define FW_DIR       "/firmware"

// Dedicated HSPI instance for SD — separate from TFT_eSPI's VSPI.
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

static bool _endsWithBinIgnoreCase(const char* s) {
    if (!s) return false;
    size_t n = strlen(s);
    if (n < 4) return false;
    const char* ext = s + n - 4;
    return (tolower((unsigned char)ext[0]) == '.' &&
            tolower((unsigned char)ext[1]) == 'b' &&
            tolower((unsigned char)ext[2]) == 'i' &&
            tolower((unsigned char)ext[3]) == 'n');
}

// Check if a path is already in the list (FAT32 is case-insensitive).
static bool _isDuplicate(const FwEntry* entries, int count, const char* path) {
    for (int i = 0; i < count; i++) {
        if (strcasecmp(entries[i].path, path) == 0) return true;
    }
    return false;
}

static int _scanFirmwareDir(const char* dirPath, FwEntry* entries, int count) {
    File dir = SD.open(dirPath);
    if (!dir || !dir.isDirectory()) {
        if (dir) dir.close();
        Serial.printf("[LAUNCHER] Skip dir %s (not found)\n", dirPath);
        return count;
    }

    Serial.printf("[LAUNCHER] Scanning %s ...\n", dirPath);
    while (count < MAX_FW) {
        File f = dir.openNextFile();
        if (!f) break;
        const char* nm = f.name();
        if (!f.isDirectory() && _endsWithBinIgnoreCase(nm)) {
            const char* base = strrchr(nm, '/');
            base = base ? base + 1 : nm;
            char path[128];
            if (strcmp(dirPath, "/") == 0) {
                snprintf(path, sizeof(path), "/%s", base);
            } else {
                snprintf(path, sizeof(path), "%s/%s", dirPath, base);
            }
            if (_isDuplicate(entries, count, path)) {
                f.close();
                continue;
            }
            strlcpy(entries[count].name, base, sizeof(entries[count].name));
            strlcpy(entries[count].path, path, sizeof(entries[count].path));
            entries[count].size = f.size();
            Serial.printf("[LAUNCHER] Found FW: %s (%u bytes)\n",
                          entries[count].path, (unsigned)entries[count].size);
            count++;
        }
        f.close();
    }
    dir.close();
    return count;
}

static int _listFirmware(FwEntry* entries) {
    int count = 0;
    // FAT32 is case-insensitive, so only scan each unique real directory once.
    // Try canonical names; skip if already opened under a different case variant.
    const char* dirs[] = { "/firmware", "/firmwares", "/" };
    for (uint8_t i = 0; i < sizeof(dirs) / sizeof(dirs[0]) && count < MAX_FW; i++) {
        count = _scanFirmwareDir(dirs[i], entries, count);
    }
    Serial.printf("[LAUNCHER] Firmware entries=%d\n", count);
    return count;
}

static bool _mountSd() {
    // SD uses its own HSPI bus — completely independent from TFT (VSPI).
    // No need to park or restore TFT.
    pinMode(SD_CS_PIN, OUTPUT);
    digitalWrite(SD_CS_PIN, HIGH);

    sdSPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, -1);
    delay(10);

    // SD power-on: ≥74 clocks with CS HIGH (SD spec requirement).
    sdSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    for (uint8_t i = 0; i < 10; i++) sdSPI.transfer(0xFF);
    sdSPI.endTransaction();
    delay(10);

    // Diagnostic: read MISO idle level.
    sdSPI.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE0));
    const uint8_t misoIdle = sdSPI.transfer(0xFF);
    sdSPI.endTransaction();
    Serial.printf("[LAUNCHER] MISO idle=0x%02X (HSPI)\n", misoIdle);

    // Try SD.begin at increasing frequencies.
    const uint32_t freqs[] = {400000, 1000000, 4000000};
    for (uint8_t i = 0; i < sizeof(freqs) / sizeof(freqs[0]); i++) {
        Serial.printf("[LAUNCHER] SD.begin(CS=%d, freq=%lu, HSPI)...\n",
                      SD_CS_PIN, (unsigned long)freqs[i]);
        if (SD.begin(SD_CS_PIN, sdSPI, freqs[i])) {
            Serial.println("[LAUNCHER] SD.begin = OK");
            return true;
        }
        Serial.println("[LAUNCHER] SD.begin = FAIL");
        SD.end();
        delay(50);
    }

    sdSPI.end();
    return false;
}

static void _unmountSd() {
    // Just release HSPI — TFT on VSPI is unaffected (separate bus).
    Serial.println("[LAUNCHER] Unmounting SD...");
    digitalWrite(SD_CS_PIN, HIGH);
    sdSPI.end();
    Serial.println("[LAUNCHER] SD unmounted");
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

    while (true) {
        // ── Main OS selection menu ────────────────────────────────────────────
        const char* mainOpts[] = { "Boot ESP-SATDUMP", "Install from SD" };
        int choice = _waitForSelection(mainOpts, 2, "OS LAUNCHER");
        if (choice == 0) return; // user explicitly wants normal app

        // ── Install flow ──────────────────────────────────────────────────────
        _showMessage("Mounting SD...");

        bool sdOk = _mountSd();
        if (!sdOk) {
            const char* failOpts[] = { "Retry SD", "Boot ESP-SATDUMP" };
            _showMessage("SD Mount Failed!", "Choose action", COL_RED);
            int failChoice = _waitForSelection(failOpts, 2, "SD ERROR");
            if (failChoice == 1) return;
            continue;
        }
        Serial.printf("[LAUNCHER] SD type=%d size=%lluMB\n",
                      (int)SD.cardType(), SD.cardSize() / (1024ULL * 1024ULL));

        static FwEntry entries[MAX_FW];  // static: BSS, not stack (3136 bytes)
        int count = _listFirmware(entries);

        if (count == 0) {
            SD.end();
            _unmountSd();
            const char* emptyOpts[] = { "Rescan SD", "Boot ESP-SATDUMP" };
            _showMessage("No .bin found!", "Choose action", COL_RED);
            int emptyChoice = _waitForSelection(emptyOpts, 2, "NO FIRMWARE");
            if (emptyChoice == 1) return;
            continue;
        }

        // Unmount SD while user browses menu (remount after selection).
        SD.end();
        _unmountSd();
        Serial.printf("[LAUNCHER] Drawing FW menu (%d entries)\n", count);

        // ── Firmware selection menu ───────────────────────────────────────────
        static const char* names[MAX_FW];  // static: BSS, not stack
        for (int i = 0; i < count; i++) names[i] = entries[i].name;

        int fwSel = _waitForSelection(names, count, "SELECT FIRMWARE");
        _showMessage("Preparing flash...");
        if (!_mountSd()) {
            const char* remountFailOpts[] = { "Back to Launcher", "Boot ESP-SATDUMP" };
            _showMessage("SD remount failed!", "Choose action", COL_RED);
            int rm = _waitForSelection(remountFailOpts, 2, "SD ERROR");
            if (rm == 1) return;
            continue;
        }

        bool ok = _flashFile(entries[fwSel].path, entries[fwSel].size);

        SD.end();
        _unmountSd();

        if (ok) {
            _showMessage("Done! Rebooting...", nullptr, COL_GREEN);
            delay(1500);
            esp_restart();
        } else {
            const char* flashFailOpts[] = { "Back to Launcher", "Boot ESP-SATDUMP" };
            _showMessage("Flash Failed!", Update.errorString(), COL_RED);
            int ff = _waitForSelection(flashFailOpts, 2, "FLASH ERROR");
            if (ff == 1) return;
        }
    }
}
