#include "page_skyview.h"
#include "display_utils.h"
#include "gps_parser.h"
#include <math.h>

static char constellationChar(uint8_t prn) {
    if (prn >= 1   && prn <= 32)  return 'G';
    if (prn >= 33  && prn <= 64)  return 'S';
    if (prn >= 65  && prn <= 96)  return 'R';
    if (prn >= 193 && prn <= 202) return 'J';
    if (prn >= 201 && prn <= 235) return 'C';
    if (prn >= 301 && prn <= 336) return 'E';
    return '?';
}

static void plotGeometry(int16_t& CX, int16_t& CY, int16_t& R_MAX) {
    int16_t W = tft.width();
    int16_t H = tft.height();
    CX    = W / 2;
    CY    = STATUS_BAR_H + (H - STATUS_BAR_H) / 2;
    int16_t rW = (W - 30) / 2;
    int16_t rH = (H - STATUS_BAR_H - 30) / 2;
    R_MAX = (rW < rH) ? rW : rH;
}

static void drawPolarGrid() {
    int16_t CX, CY, R_MAX;
    plotGeometry(CX, CY, R_MAX);

    // Crosshairs
    tft.drawFastVLine(CX, CY - R_MAX, 2 * R_MAX, COL_DIM);
    tft.drawFastHLine(CX - R_MAX, CY, 2 * R_MAX, COL_DIM);

    // Rings
    for (uint8_t elev : {30, 60}) {
        int16_t r = (int16_t)((1.0f - elev / 90.0f) * R_MAX);
        tft.drawCircle(CX, CY, r, COL_DIM);
    }
    tft.drawCircle(CX, CY, R_MAX, COL_ACCENT); // Outer ring

    // Labels
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(CX - 3, CY - R_MAX - 12); tft.print('N');
    tft.setCursor(CX - 3, CY + R_MAX + 4);  tft.print('S');
    tft.setCursor(CX + R_MAX + 6, CY - 3);  tft.print('E');
    tft.setCursor(CX - R_MAX - 12, CY - 3); tft.print('W');
}

struct SatPos { int16_t x, y; uint8_t prn; };
static SatPos _prevSats[MAX_SATS];

static void erasePrevSatellites() {
    int16_t CX, CY, R_MAX;
    plotGeometry(CX, CY, R_MAX);
    for (uint8_t i = 0; i < MAX_SATS; i++) {
        if (!_prevSats[i].prn) continue;
        int16_t x = _prevSats[i].x;
        int16_t y = _prevSats[i].y;
        tft.fillCircle(x, y, 8, COL_BG);
        // Restore grid
        if (abs(x - CX) < 9) tft.drawFastVLine(CX, CY - R_MAX, 2 * R_MAX, COL_DIM);
        if (abs(y - CY) < 9) tft.drawFastHLine(CX - R_MAX, CY, 2 * R_MAX, COL_DIM);
        // Restore rings (simplified: just redraw circles near erase area)
        for (uint8_t elev : {30, 60}) {
            int16_t r = (int16_t)((1.0f - elev / 90.0f) * R_MAX);
            tft.drawCircle(CX, CY, r, COL_DIM);
        }
        tft.drawCircle(CX, CY, R_MAX, COL_ACCENT);
    }
}

static void drawSatellites() {
    int16_t CX, CY, R_MAX;
    plotGeometry(CX, CY, R_MAX);

    for (uint8_t i = 0; i < MAX_SATS; i++) {
        const SatInfo& s = gpsData.sats[i];
        if (!s.prn) { _prevSats[i] = {0, 0, 0}; continue; }

        float r   = (1.0f - s.elev / 90.0f) * R_MAX;
        float rad = (s.azim - 90) * M_PI / 180.0f;
        int16_t x = CX + (int16_t)(r * cosf(rad));
        int16_t y = CY + (int16_t)(r * sinf(rad));

        _prevSats[i] = {x, y, s.prn};

        uint16_t col = snrColor(s.snr);
        tft.fillCircle(x, y, 6, col);
        if (s.used) {
            tft.drawCircle(x, y, 7, COL_TEXT);
            tft.drawCircle(x, y, 8, COL_TEXT);
        }

        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(1);
        char buf[4];
        snprintf(buf, sizeof(buf), "%02d", s.prn);
        int16_t tx = (x > CX) ? x + 10 : x - 22;
        tft.setCursor(tx, y - 3);
        tft.print(buf);
    }
}

void PageSkyView::onEnter() {
    memset(_prevSats, 0, sizeof(_prevSats));
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawPolarGrid();
    drawSatellites();
}

void PageSkyView::update() {
    erasePrevSatellites();
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawSatellites();
}
