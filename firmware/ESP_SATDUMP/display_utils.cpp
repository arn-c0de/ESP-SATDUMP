#include "display_utils.h"

TFT_eSPI tft = TFT_eSPI();

static bool _portrait = (DEFAULT_PORTRAIT != 0);

int16_t centeredX(const char* s, uint8_t sz) {
    return (tft.width() - (int16_t)(strlen(s) * 6 * sz)) / 2;
}

void showSplash() {
    int16_t W = tft.width();
    int16_t H = tft.height();
    tft.fillScreen(COL_BG);

    const int16_t TITLE_H   = 32;
    const int16_t VER_H     = 16;
    const int16_t ART_LINE_H= 16;
    const int16_t ART_LINES = 8;
    const int16_t TOTAL_H   = TITLE_H + 6 + VER_H + 20 + ART_LINES * ART_LINE_H;
    int16_t y = (H - TOTAL_H) / 2;

    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextSize(4);
    tft.setCursor(centeredX("SATDUMP", 4), y);
    tft.print("SATDUMP");
    y += TITLE_H + 6;

    tft.setTextColor(COL_YELLOW, COL_BG);
    tft.setTextSize(2);
    tft.setCursor(centeredX(SATDUMP_VERSION, 2), y);
    tft.print(SATDUMP_VERSION);
    y += VER_H + 20;

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

    // OS Launcher hint
    tft.setTextColor(COL_DIM, COL_BG);
    tft.setTextSize(1);
    const char* hint = "Hold GPIO26 for OS Launcher";
    tft.setCursor(centeredX(hint, 1), H - 14);
    tft.print(hint);

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

void drawBadge(int16_t x, int16_t y, const char* text, uint16_t color, uint16_t textColor) {
    int16_t tw = (int16_t)(strlen(text) * 6 * 2);
    tft.fillRoundRect(x, y, tw + 8, 20, 4, color);
    tft.setTextColor(textColor, color);
    tft.setTextSize(2);
    tft.setCursor(x + 4, y + 2);
    tft.print(text);
}

void drawValueRow(int16_t x, int16_t y, int16_t w, const char* label, const char* value, uint16_t valCol) {
    tft.setTextSize(1);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setCursor(x, y);
    tft.print(label);

    tft.setTextColor(valCol, COL_BG);
    int16_t valW = (int16_t)(strlen(value) * 6 * 1);
    tft.setCursor(x + w - valW, y);
    tft.print(value);
}

void drawLargeRow(int16_t x, int16_t y, int16_t w, const char* label, const char* value, uint16_t valCol) {
    tft.setTextSize(1);
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setCursor(x, y);
    tft.print(label);

    tft.setTextSize(2);
    tft.setTextColor(valCol, COL_BG);
    int16_t valW = (int16_t)(strlen(value) * 6 * 2);
    tft.setCursor(x + w - valW, y - 4);
    tft.print(value);
}

void drawSeparator(int16_t x, int16_t y, int16_t w) {
    tft.drawFastHLine(x, y, w, COL_DIM);
}
