#include "page_nmea.h"
#include "display_utils.h"
#include "gps_parser.h"

void PageNMEA::pushLine(const char* s) {
    strncpy(_ring[_head], s, NMEA_LINE_LEN - 1);
    _ring[_head][NMEA_LINE_LEN - 1] = '\0';
    _head = (_head + 1) % NMEA_LINES;
    if (_count < NMEA_LINES) _count++;
    _scroll = 0;
}

void PageNMEA::redraw() {
    int16_t W       = tft.width();
    int16_t H       = tft.height();
    int16_t LINE_H  = (H - STATUS_BAR_H - 10) / NMEA_LINES;
    int16_t MAX_COLS= W / 6;

    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);

    // Scroll indicator
    if (_count > 0) {
        int16_t barH = (H - STATUS_BAR_H - 10);
        int16_t scrollH = (NMEA_LINES * barH) / _count;
        if (scrollH > barH) scrollH = barH;
        int16_t scrollY = STATUS_BAR_H + 5 + ((_count - NMEA_LINES - _scroll) * (barH - scrollH)) / (_count - NMEA_LINES > 0 ? _count - NMEA_LINES : 1);
        tft.drawFastVLine(W - 2, STATUS_BAR_H + 5, barH, COL_DIM);
        tft.drawFastVLine(W - 2, scrollY, scrollH, COL_ACCENT);
    }

    if (_count == 0) {
        tft.setTextColor(COL_YELLOW, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(10, STATUS_BAR_H + 20);
        tft.print("Waiting for NMEA data...");
        return;
    }

    tft.setTextSize(1);

    for (int8_t row = NMEA_LINES - 1; row >= 0; row--) {
        int16_t lineIdx = ((int16_t)_head - 1 - row + (int16_t)_scroll + (int16_t)NMEA_LINES * 4) % NMEA_LINES;
        int16_t screenRow = NMEA_LINES - 1 - row;
        int16_t y = STATUS_BAR_H + 5 + screenRow * LINE_H;

        bool hasData = (_count == NMEA_LINES) || (lineIdx < (int16_t)_count);
        if (!hasData) continue;

        const char* line = _ring[lineIdx];
        if (!line[0]) continue;

        uint16_t col = COL_TEXT;
        if (strstr(line, "GGA")) col = COL_GREEN;
        else if (strstr(line, "RMC")) col = COL_ACCENT;
        else if (strstr(line, "GSV")) col = COL_YELLOW;

        char display[MAX_COLS + 1];
        strncpy(display, line, MAX_COLS);
        display[MAX_COLS] = '\0';
        for (int i = strlen(display) - 1; i >= 0 && (display[i] == '\r' || display[i] == '\n'); i--)
            display[i] = '\0';

        tft.setTextColor(col, COL_BG);
        tft.setCursor(4, y);
        tft.print(display);
    }
}

void PageNMEA::onEnter() {
    tft.fillScreen(COL_BG);
    redraw();
}

void PageNMEA::update() {
    if (gpsData.last_sentence[0]) {
        static char lastPushed[84] = {};
        if (strncmp(gpsData.last_sentence, lastPushed, 83) != 0) {
            strncpy(lastPushed, gpsData.last_sentence, 83);
            pushLine(gpsData.last_sentence);
        }
    }
    redraw();
}

void PageNMEA::onEncoder(EncEvent ev) {
    if (ev == EncEvent::CW) {
        if (_scroll < (int8_t)(_count - NMEA_LINES)) _scroll++;
        redraw();
    } else if (ev == EncEvent::CCW) {
        if (_scroll > 0) _scroll--;
        redraw();
    }
}
