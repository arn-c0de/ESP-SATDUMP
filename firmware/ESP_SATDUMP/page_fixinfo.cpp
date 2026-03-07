#include "page_fixinfo.h"
#include "display_utils.h"
#include "gps_parser.h"

static const int16_t Y0 = STATUS_BAR_H + 8;
static const int16_t LX = 10;
static const int16_t PAD = 4;

static void drawContent() {
    int16_t W = tft.width();
    int16_t rowW = W - LX * 2;
    char buf[32];
    int16_t y = Y0;

    // ── Fix badge ───────────────────────────────────────────────────────────
    const char* badge;
    uint16_t badgeCol;
    switch (gpsData.fix_quality) {
        case 2:  badge = " DGPS FIX "; badgeCol = COL_GREEN;  break;
        case 1:  badge = "  GPS FIX "; badgeCol = COL_GREEN;  break;
        default: badge = "  NO FIX  "; badgeCol = COL_RED;    break;
    }
    drawBadge(LX, y, badge, badgeCol, COL_BG);
    y += 30;

    // ── Position Group ──────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%+12.6f", gpsData.lat);
    drawValueRow(LX, y, rowW, "LATITUDE", buf, COL_TEXT);
    y += 12;
    snprintf(buf, sizeof(buf), "%+12.6f", gpsData.lon);
    drawValueRow(LX, y, rowW, "LONGITUDE", buf, COL_TEXT);
    y += 10;
    drawSeparator(LX, y, rowW);
    y += 10;

    // ── Motion Group ────────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.1f m", gpsData.alt_m);
    drawValueRow(LX, y, rowW, "ALTITUDE", buf, COL_ACCENT);
    y += 12;

    snprintf(buf, sizeof(buf), "%.1f km/h", gpsData.speed_kmh);
    drawValueRow(LX, y, rowW, "SPEED", buf, COL_ACCENT);
    y += 12;

    snprintf(buf, sizeof(buf), "%.1f deg", gpsData.course_deg);
    drawValueRow(LX, y, rowW, "COURSE", buf, COL_ACCENT);
    y += 10;
    drawSeparator(LX, y, rowW);
    y += 10;

    // ── Precision & Stats ───────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%.2f", gpsData.hdop);
    drawValueRow(LX, y, rowW, "HDOP", buf, COL_TEXT);
    y += 12;

    snprintf(buf, sizeof(buf), "%d / %d", gpsData.sats_used, gpsData.sats_inview);
    drawValueRow(LX, y, rowW, "SATELLITES", buf, COL_TEXT);
    y += 10;
    drawSeparator(LX, y, rowW);
    y += 10;

    // ── DateTime Group ──────────────────────────────────────────────────────
    snprintf(buf, sizeof(buf), "%02d:%02d:%02d UTC", gpsData.hour, gpsData.minute, gpsData.second);
    drawValueRow(LX, y, rowW, "TIME", buf, COL_YELLOW);
    y += 12;

    snprintf(buf, sizeof(buf), "%04d-%02d-%02d", gpsData.year, gpsData.month, gpsData.day);
    drawValueRow(LX, y, rowW, "DATE", buf, COL_YELLOW);
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
    // Page switching is handled by manager
}
