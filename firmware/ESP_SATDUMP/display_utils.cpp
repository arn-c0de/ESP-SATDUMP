#include "display_utils.h"

TFT_eSPI tft = TFT_eSPI();

static bool _portrait = (DEFAULT_PORTRAIT != 0);

static inline int16_t centeredX(const char* s, uint8_t sz) {
    return (tft.width() - (int16_t)(strlen(s) * 6 * sz)) / 2;
}

void showSplash() {
    int16_t W = tft.width();
    int16_t H = tft.height();
    tft.fillScreen(COL_BG);

    // ── Layout: calculate total block height to center vertically
    // Title (size 4 = 32px) + gap 6 + version (size 2 = 16px) + gap 20 + art 7×16px
    const int16_t TITLE_H   = 32;
    const int16_t VER_H     = 16;
    const int16_t ART_LINE_H= 16;
    const int16_t ART_LINES = 8;
    const int16_t TOTAL_H   = TITLE_H + 6 + VER_H + 20 + ART_LINES * ART_LINE_H;
    int16_t y = (H - TOTAL_H) / 2;

    // ── "SATDUMP" title
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextSize(4);
    tft.setCursor(centeredX("SATDUMP", 4), y);
    tft.print("SATDUMP");
    y += TITLE_H + 6;

    // ── Version
    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX(SATDUMP_VERSION, 2), y);
    tft.print(SATDUMP_VERSION);
    y += VER_H + 20;

    // ── ASCII satellite (size 2)
    const char* art[ART_LINES] = {
        "      _______      ",
        "  ___[_______]___  ",
        " |   |   |   |   | ",
        " |---|  (O)  |---| ",
        " |___|_______|___| ",
        "     \\_______/     ",
        "        | |        ",
        "        'V'        ",
    };
    tft.setTextColor(COL_TEXT, COL_BG);
    tft.setTextSize(2);
    for (uint8_t i = 0; i < ART_LINES; i++) {
        tft.setCursor(centeredX(art[i], 2), y);
        tft.print(art[i]);
        y += ART_LINE_H;
    }

    delay(2500);
    tft.fillScreen(COL_BG);
}

void displayInit() {
    tft.init();
    tft.setRotation(TFT_ROTATION);
    tft.fillScreen(COL_BG);
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
}

void displayToggleRotation() {
    _portrait = !_portrait;
    tft.setRotation(_portrait ? ROT_PORTRAIT : ROT_LANDSCAPE);
    tft.fillScreen(COL_BG);
}

bool displayIsPortrait() { return _portrait; }

void drawStatusBar(const char* pageName, uint8_t satsInView, uint8_t fixQuality) {
    int16_t W = tft.width();
    tft.fillRect(0, 0, W, STATUS_BAR_H, COL_STATUS_BG);

    tft.setTextColor(COL_ACCENT, COL_STATUS_BG);
    tft.setTextSize(1);

    tft.setCursor(4, 6);
    tft.print(pageName);

    char satBuf[16];
    snprintf(satBuf, sizeof(satBuf), "SAT:%d", satsInView);
    int16_t tw = (int16_t)(strlen(satBuf) * 6);
    tft.setCursor((W - tw) / 2, 6);
    tft.print(satBuf);

    const char* fixStr;
    uint16_t fixColor;
    switch (fixQuality) {
        case 2:  fixStr = "DGPS"; fixColor = COL_GREEN; break;
        case 1:  fixStr = "FIX";  fixColor = COL_GREEN; break;
        default: fixStr = "NO FIX"; fixColor = COL_RED;  break;
    }
    tft.setTextColor(fixColor, COL_STATUS_BG);
    tft.setCursor(W - (int16_t)(strlen(fixStr) * 6) - 4, 6);
    tft.print(fixStr);

    tft.drawFastHLine(0, STATUS_BAR_H, W, COL_DIM);
}

void clearContent() {
    tft.fillRect(0, STATUS_BAR_H + 1, tft.width(), tft.height() - STATUS_BAR_H - 1, COL_BG);
}

uint16_t snrColor(uint8_t snr) {
    if (snr >= SNR_GOOD) return COL_GREEN;
    if (snr >= SNR_FAIR) return COL_YELLOW;
    if (snr > 0)         return COL_RED;
    return COL_DIM;
}
