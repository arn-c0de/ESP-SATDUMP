#include "page_signals.h"
#include "display_utils.h"
#include "gps_parser.h"

static void drawBars() {
    int16_t W           = tft.width();
    int16_t H           = tft.height();
    int16_t LEFT_MARGIN = 32;
    int16_t RIGHT_MARGIN= 30;
    int16_t BAR_MAX_W   = W - LEFT_MARGIN - RIGHT_MARGIN - 4;
    int16_t ROW_H       = (H - STATUS_BAR_H - 4) / MAX_SATS;
    int16_t BAR_H       = ROW_H - 3;
    int16_t y = STATUS_BAR_H + 4;

    // Collect active satellites sorted by PRN
    uint8_t order[MAX_SATS];
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_SATS; i++) {
        if (gpsData.sats[i].prn) order[count++] = i;
    }
    // Simple insertion sort by PRN
    for (uint8_t i = 1; i < count; i++) {
        uint8_t key = order[i];
        int8_t j = i - 1;
        while (j >= 0 && gpsData.sats[order[j]].prn > gpsData.sats[key].prn) {
            order[j+1] = order[j]; j--;
        }
        order[j+1] = key;
    }

    for (uint8_t r = 0; r < MAX_SATS; r++) {
        int16_t ry = y + r * ROW_H;
        if (r >= count) {
            // Clear unused row
            tft.fillRect(0, ry, W, ROW_H, COL_BG);
            continue;
        }
        const SatInfo& s = gpsData.sats[order[r]];

        // PRN label
        char prnBuf[4];
        snprintf(prnBuf, sizeof(prnBuf), "%2d", s.prn);
        uint16_t lblCol = s.used ? COL_ACCENT : COL_TEXT;
        tft.setTextColor(lblCol, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(2, ry + (ROW_H - 8) / 2);
        tft.print(prnBuf);

        // SNR bar
        int16_t barW = (s.snr * BAR_MAX_W) / 50;
        if (barW > BAR_MAX_W) barW = BAR_MAX_W;
        uint16_t col = snrColor(s.snr);
        if (barW > 0) {
            tft.fillRect(LEFT_MARGIN, ry + 2, barW, BAR_H, col);
        }
        // Empty portion
        tft.fillRect(LEFT_MARGIN + barW, ry + 2, BAR_MAX_W - barW, BAR_H, COL_DIM);

        // SNR value
        char snrBuf[8];
        snprintf(snrBuf, sizeof(snrBuf), "%2ddB", s.snr);
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setCursor(LEFT_MARGIN + BAR_MAX_W + 4, ry + (ROW_H - 8) / 2);
        tft.print(snrBuf);
    }

    // NO FIX overlay
    if (!gpsData.fix_quality) {
        tft.setTextColor(COL_RED, COL_BG);
        tft.setTextSize(2);
        tft.setCursor((W - 12*12) / 2, H / 2 - 8);
        tft.print("NO FIX");
    }
}

void PageSignals::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawBars();
}

void PageSignals::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    drawBars();
}
