#include "page_skyview.h"
#include "display_utils.h"
#include "gps_parser.h"
#include <math.h>

// Polar plot geometry
// Centre of plot: leave top 20px for status bar + some margin
static const int16_t CX     = 160;              // centre X
static const int16_t CY     = (STATUS_BAR_H + (TFT_HEIGHT - STATUS_BAR_H) / 2);
static const int16_t R_MAX  = (TFT_HEIGHT - STATUS_BAR_H - 10) / 2;  // ~140 px

static void drawPolarGrid() {
    // Three concentric rings at 30°, 60°, 90° elevation
    // r = (1 - elev/90) * R_MAX → elev 90 → r=0 (center), elev 0 → r=R_MAX
    for (uint8_t elev : {30, 60, 90}) {
        int16_t r = (int16_t)((1.0f - elev / 90.0f) * R_MAX);
        tft.drawCircle(CX, CY, r, COL_DIM);
    }
    // Cardinal spokes
    tft.drawFastVLine(CX, CY - R_MAX, 2 * R_MAX, COL_DIM);
    tft.drawFastHLine(CX - R_MAX, CY, 2 * R_MAX, COL_DIM);

    // Cardinal labels
    tft.setTextColor(COL_ACCENT, COL_BG);
    tft.setTextSize(1);
    tft.setCursor(CX - 3, CY - R_MAX - 10); tft.print('N');
    tft.setCursor(CX - 3, CY + R_MAX + 2);  tft.print('S');
    tft.setCursor(CX + R_MAX + 2, CY - 3);  tft.print('E');
    tft.setCursor(CX - R_MAX - 8, CY - 3);  tft.print('W');
}

static void drawSatellites() {
    for (uint8_t i = 0; i < MAX_SATS; i++) {
        const SatInfo& s = gpsData.sats[i];
        if (!s.prn) continue;

        float r   = (1.0f - s.elev / 90.0f) * R_MAX;
        float rad = (s.azim - 90) * M_PI / 180.0f;  // 0°=N → -90° in math coords
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
