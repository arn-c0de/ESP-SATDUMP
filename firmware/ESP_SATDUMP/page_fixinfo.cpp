#include "page_fixinfo.h"
#include "display_utils.h"
#include "gps_parser.h"

static const int16_t Y0 = STATUS_BAR_H + 8;
static const int16_t LX = 4;

static void printRow(int16_t x, int16_t y, uint8_t sz,
                     uint16_t labelCol, uint16_t valCol,
                     const char* label, const char* value) {
    tft.setTextSize(sz);
    tft.setTextColor(labelCol, COL_BG);
    tft.setCursor(x, y);
    tft.print(label);
    tft.setTextColor(valCol, COL_BG);
    tft.print(value);
}

static void drawContent() {
    // No full clear — text drawn with bg color overwrites in place (no flicker)

    char buf[32];
    int16_t y = Y0;

    // ── Fix badge ───────────────────────────────────────────────────────────
    const char* badge;
    uint16_t badgeCol;
    switch (gpsData.fix_quality) {
        case 2:  badge = "[DGPS FIX]"; badgeCol = COL_GREEN;  break;
        case 1:  badge = "[ GPS FIX]"; badgeCol = COL_GREEN;  break;
        default: badge = "[ NO  FIX]"; badgeCol = COL_RED;    break;
    }
    tft.setTextSize(2);
    tft.setTextColor(badgeCol, COL_BG);
    tft.setCursor(LX, y);
    tft.print(badge);
    y += 22;

    // ── Lat / Lon (large) ───────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%+12.6f", gpsData.lat);
    printRow(LX, y, 2, COL_DIM, COL_TEXT, "LAT: ", buf);
    y += 22;

    snprintf(buf, sizeof(buf), "%+12.6f", gpsData.lon);
    printRow(LX, y, 2, COL_DIM, COL_TEXT, "LON: ", buf);
    y += 26;

    // ── Medium fields ───────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.1f m", gpsData.alt_m);
    printRow(LX, y, 1, COL_DIM, COL_TEXT, "ALT:    ", buf);
    y += 14;

    snprintf(buf, sizeof(buf), "%.1f km/h", gpsData.speed_kmh);
    printRow(LX, y, 1, COL_DIM, COL_TEXT, "SPEED:  ", buf);
    y += 14;

    snprintf(buf, sizeof(buf), "%.1f deg", gpsData.course_deg);
    printRow(LX, y, 1, COL_DIM, COL_TEXT, "COURSE: ", buf);
    y += 14;

    snprintf(buf, sizeof(buf), "%.2f", gpsData.hdop);
    printRow(LX, y, 1, COL_DIM, COL_TEXT, "HDOP:   ", buf);
    y += 18;

    // ── Small info ──────────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d UTC",
             gpsData.hour, gpsData.minute, gpsData.second);
    printRow(LX, y, 1, COL_DIM, COL_ACCENT, "TIME:  ", buf);
    y += 14;

    snprintf(buf, sizeof(buf), "%04d-%02d-%02d",
             gpsData.year, gpsData.month, gpsData.day);
    printRow(LX, y, 1, COL_DIM, COL_ACCENT, "DATE:  ", buf);
    y += 14;

    snprintf(buf, sizeof(buf), "%d / %d",
             gpsData.sats_used, gpsData.sats_inview);
    printRow(LX, y, 1, COL_DIM, COL_TEXT, "SATS:  ", buf);
}

void PageFixInfo::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawContent();
}

void PageFixInfo::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawContent();
}

void PageFixInfo::onEncoder(EncEvent ev) {
    if (ev == EncEvent::CLICK) {
        _showDetail = !_showDetail;
        // Future: toggle detail view; for now just redraw
        update();
    }
}
