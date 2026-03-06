#include "page_nmea.h"
#include "display_utils.h"
#include "gps_parser.h"

void PageNMEA::pushLine(const char* s) {
    strncpy(_ring[_head], s, NMEA_LINE_LEN - 1);
    _ring[_head][NMEA_LINE_LEN - 1] = '\0';
    _head = (_head + 1) % NMEA_LINES;
    if (_count < NMEA_LINES) _count++;
    _scroll = 0;  // auto-scroll to bottom on new data
}

void PageNMEA::redraw() {
    int16_t W       = tft.width();
    int16_t H       = tft.height();
    int16_t LINE_H  = (H - STATUS_BAR_H - 2) / NMEA_LINES;
    int16_t MAX_COLS= W / 6;

    tft.fillRect(0, STATUS_BAR_H + 1, W, H - STATUS_BAR_H - 1, COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);

    if (_count == 0) {
        tft.setTextColor(COL_DIM, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(4, STATUS_BAR_H + 20);
        tft.print("Waiting for NMEA...");
        return;
    }

    tft.setTextSize(1);

    // We show NMEA_LINES rows; bottom row = most recent (if _scroll==0)
    // Line index in ring:  (_head - 1 - row + _scroll + NMEA_LINES*2) % NMEA_LINES
    for (int8_t row = NMEA_LINES - 1; row >= 0; row--) {
        int16_t lineIdx = ((int16_t)_head - 1 - row + (int16_t)_scroll + (int16_t)NMEA_LINES * 4)
                          % NMEA_LINES;
        int16_t screenRow = NMEA_LINES - 1 - row;
        int16_t y = STATUS_BAR_H + 2 + screenRow * LINE_H;

        // Determine if this row has valid data
        bool hasData = (_count == NMEA_LINES) || (lineIdx < (int16_t)_count);
        if (!hasData) continue;

        const char* line = _ring[lineIdx];
        if (!line[0]) continue;

        // Colour-code by sentence type
        uint16_t col = COL_DIM;
        if (strncmp(line, "$GPGGA", 6) == 0 || strncmp(line, "$GNGGA", 6) == 0)
            col = COL_GREEN;
        else if (strncmp(line, "$GPRMC", 6) == 0 || strncmp(line, "$GNRMC", 6) == 0)
            col = COL_ACCENT;
        else if (strncmp(line, "$GPGSV", 6) == 0 || strncmp(line, "$GNGSV", 6) == 0)
            col = COL_YELLOW;
        else if (strncmp(line, "$GPGSA", 6) == 0 || strncmp(line, "$GNGSA", 6) == 0)
            col = COL_TEXT;

        // Truncate to fit screen
        char display[MAX_COLS + 1];
        strncpy(display, line, MAX_COLS);
        display[MAX_COLS] = '\0';
        // Strip trailing \r\n
        for (int i = strlen(display) - 1; i >= 0 && (display[i] == '\r' || display[i] == '\n'); i--)
            display[i] = '\0';

        tft.setTextColor(col, COL_BG);
        tft.setCursor(0, y);
        tft.print(display);
    }
}

void PageNMEA::onEnter() {
    tft.fillScreen(COL_BG);
    redraw();
}

void PageNMEA::update() {
    // Push latest sentence if it changed
    if (gpsData.last_sentence[0]) {
        // Only push if it's different from what we last pushed
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
        // Scroll up (older)
        if (_scroll < (int8_t)(_count - 1)) _scroll++;
        redraw();
    } else if (ev == EncEvent::CCW) {
        // Scroll down (newer)
        if (_scroll > 0) _scroll--;
        redraw();
    }
}
