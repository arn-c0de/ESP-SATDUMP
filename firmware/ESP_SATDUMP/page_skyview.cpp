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
    int16_t rW = (W - 10) / 2;
    int16_t rH = (H - STATUS_BAR_H - 10) / 2;
    R_MAX = (rW < rH) ? rW : rH;
}

static void drawPolarGrid() {
    int16_t CX, CY, R_MAX;
    plotGeometry(CX, CY, R_MAX);

    for (uint8_t elev : {30, 60, 90}) {
        int16_t r = (int16_t)((1.0f - elev / 90.0f) * R_MAX);
        tft.drawCircle(CX, CY, r, COL_DIM);
    }
    tft.drawFastVLine(CX, CY - R_MAX, 2 * R_MAX, COL_DIM);
    tft.drawFastHLine(CX - R_MAX, CY, 2 * R_MAX, COL_DIM);

    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(CX - 3, CY - R_MAX - 10); tft.print('N');
    tft.setCursor(CX - 3, CY + R_MAX + 2);  tft.print('S');
    tft.setCursor(CX + R_MAX + 2, CY - 3);  tft.print('E');
    tft.setCursor(CX - R_MAX - 8, CY - 3);  tft.print('W');
}

// Previous satellite screen positions — used to erase old dots without fillRect
struct SatPos { int16_t x, y; uint8_t prn; };
static SatPos _prevSats[MAX_SATS];

static void erasePrevSatellites() {
    int16_t CX, CY, R_MAX;
    plotGeometry(CX, CY, R_MAX);
    for (uint8_t i = 0; i < MAX_SATS; i++) {
        if (!_prevSats[i].prn) continue;
        int16_t x = _prevSats[i].x;
        int16_t y = _prevSats[i].y;
        // Erase dot + ring + label area with BG color
        tft.fillCircle(x, y, 7, COL_BG);
        tft.fillRect(x + 7, y - 4, 22, 10, COL_BG);
        // Restore any grid lines that passed through this area
        if (abs(x - CX) < 8) tft.drawFastVLine(CX, CY - R_MAX, 2 * R_MAX, COL_DIM);
        if (abs(y - CY) < 8) tft.drawFastHLine(CX - R_MAX, CY, 2 * R_MAX, COL_DIM);
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

        uint16_t col = (s.snr >= SNR_GOOD) ? COL_GREEN :
                       (s.snr >= SNR_FAIR) ? COL_YELLOW :
                       (s.snr > 0)         ? COL_RED    : COL_DIM;

        tft.fillCircle(x, y, 5, col);
        if (s.used) tft.drawCircle(x, y, 6, COL_TEXT);

        char buf[6];
        snprintf(buf, sizeof(buf), "%c%02d", constellationChar(s.prn), s.prn);
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(x + 7, y - 3);
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
    // Erase old sat positions, redraw grid where needed, draw new positions
    erasePrevSatellites();
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawSatellites();
}
