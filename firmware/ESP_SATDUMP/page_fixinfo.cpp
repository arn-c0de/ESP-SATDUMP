#include "page_fixinfo.h"
#include "display_utils.h"
#include "gps_parser.h"

static const int16_t Y0 = STATUS_BAR_H + 8;
static const int16_t LX = 10;

static void drawContent() {
    int16_t W = tft.width();
    int16_t rowW = W - LX * 2;
    char buf[32];
    int16_t y = Y0;

    const char* badge;
    uint16_t badgeCol;
    switch (gpsData.fix_quality) {
        case 2:  badge = " DGPS FIX "; badgeCol = COL_GREEN;  break;
        case 1:  badge = "  GPS FIX "; badgeCol = COL_GREEN;  break;
        default: badge = "  NO FIX  "; badgeCol = COL_RED;    break;
    }
    drawBadge(LX, y, badge, badgeCol, COL_BG);
    y += 35;

    snprintf(buf, sizeof(buf), "%+10.6f", gpsData.lat);
    drawLargeRow(LX, y, rowW, "LAT", buf, COL_TEXT);
    y += 20;
    snprintf(buf, sizeof(buf), "%+10.6f", gpsData.lon);
    drawLargeRow(LX, y, rowW, "LON", buf, COL_TEXT);
    y += 18;
    drawSeparator(LX, y, rowW);
    y += 12;

    snprintf(buf, sizeof(buf), "%.1f m", gpsData.alt_m);
    drawLargeRow(LX, y, rowW, "ALT", buf, COL_YELLOW);
    y += 20;
    snprintf(buf, sizeof(buf), "%.1f km/h", gpsData.speed_kmh);
    drawLargeRow(LX, y, rowW, "SPD", buf, COL_YELLOW);
    y += 18;
    drawSeparator(LX, y, rowW);
    y += 12;

    snprintf(buf, sizeof(buf), "%.2f", gpsData.hdop);
    drawValueRow(LX, y, rowW, "HDOP", buf, COL_TEXT);
    y += 14;
    snprintf(buf, sizeof(buf), "%d / %d", gpsData.sats_used, gpsData.sats_inview);
    drawValueRow(LX, y, rowW, "SATELLITES", buf, COL_TEXT);
    y += 12;
    drawSeparator(LX, y, rowW);
    y += 12;

    snprintf(buf, sizeof(buf), "%02d:%02d:%02d UTC", gpsData.hour, gpsData.minute, gpsData.second);
    drawValueRow(LX, y, rowW, "TIME", buf, COL_ACCENT);
    y += 14;
    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", gpsData.year, gpsData.month, gpsData.day);
    drawValueRow(LX, y, rowW, "DATE", buf, COL_ACCENT);
}

void PageFixInfo::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    drawContent();
}

void PageFixInfo::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    drawContent();
}
