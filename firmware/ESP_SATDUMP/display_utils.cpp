#include "display_utils.h"

TFT_eSPI tft = TFT_eSPI();

static bool _portrait = (DEFAULT_PORTRAIT != 0);

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
        default: fixStr = "---";  fixColor = COL_RED;   break;
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
