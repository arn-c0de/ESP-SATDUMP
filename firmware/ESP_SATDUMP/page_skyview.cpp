#include "page_skyview.h"
#include "display_utils.h"
#include "gps_parser.h"
#include <math.h>

// Compute polar plot geometry based on current screen orientation
static void plotGeometry(int16_t& CX, int16_t& CY, int16_t& R_MAX) {
    int16_t W = tft.width();
    int16_t H = tft.height();
    CX    = W / 2;
    CY    = STATUS_BAR_H + (H - STATUS_BAR_H) / 2;
    // Largest circle that fits both width and the content height
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

static void drawSatellites() {
    int16_t CX, CY, R_MAX;
    plotGeometry(CX, CY, R_MAX);

    for (uint8_t i = 0; i < MAX_SATS; i++) {
        const SatInfo& s = gpsData.sats[i];
        if (!s.prn) continue;

        float r   = (1.0f - s.elev / 90.0f) * R_MAX;
        float rad = (s.azim - 90) * M_PI / 180.0f;
        int16_t x = CX + (int16_t)(r * cosf(rad));
        int16_t y = CY + (int16_t)(r * sinf(rad));

        uint16_t col = (s.snr >= SNR_GOOD) ? COL_GREEN :
                       (s.snr >= SNR_FAIR) ? COL_YELLOW :
                       (s.snr > 0)         ? COL_RED    : COL_DIM;

        tft.fillCircle(x, y, 5, col);
        if (s.used) tft.drawCircle(x, y, 6, COL_TEXT);

        // PRN label
        char buf[4];
        snprintf(buf, sizeof(buf), "%d", s.prn);
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(x + 7, y - 3);
        tft.print(buf);
    }
}

void PageSkyView::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawPolarGrid();
    drawSatellites();
}

void PageSkyView::update() {
    // Redraw content area only
    tft.fillRect(0, STATUS_BAR_H + 1, TFT_WIDTH, TFT_HEIGHT - STATUS_BAR_H - 1, COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawPolarGrid();
    drawSatellites();
}
