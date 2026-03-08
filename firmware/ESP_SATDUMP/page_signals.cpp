#include "page_signals.h"
#include "display_utils.h"
#include "gps_parser.h"

static const int16_t LEFT_W   = 28;
static const int16_t RIGHT_W  = 50;
static const int16_t BAR_PAD  = 4;

static void drawSegmentedBar(int16_t x, int16_t y, int16_t w, int16_t h, uint8_t snr) {
    int16_t barW = (snr * w) / 50;
    if (barW > w) barW = w;
    uint16_t color = satSnrColor(snr);

    int16_t segments = 10;
    int16_t segW = w / segments;

    for (int i = 0; i < segments; i++) {
        int16_t sx = x + i * segW;
        uint16_t c = (sx - x + segW <= barW) ? color : COL_DIM;
        tft.fillRect(sx, y, segW - 1, h, c);
    }
}

static void drawBars() {
    int16_t W       = tft.width();
    int16_t H       = tft.height();
    int16_t BAR_MAX_W = W - LEFT_W - RIGHT_W - BAR_PAD * 2;
    int16_t ROW_H   = (H - STATUS_BAR_H - 4) / MAX_SATS;
    int16_t BAR_H   = ROW_H - 6;
    int16_t y0      = STATUS_BAR_H + 4;

    uint8_t order[MAX_SATS];
    uint8_t count = 0;
    for (uint8_t i = 0; i < MAX_SATS; i++) {
        if (gpsData.sats[i].prn) order[count++] = i;
    }
    for (uint8_t i = 1; i < count; i++) {
        uint8_t key = order[i];
        int8_t j = i - 1;
        while (j >= 0 && gpsData.sats[order[j]].prn > gpsData.sats[key].prn) {
            order[j+1] = order[j]; j--;
        }
        order[j+1] = key;
    }

    for (uint8_t r = 0; r < MAX_SATS; r++) {
        int16_t ry  = y0 + r * ROW_H;
        int16_t ty  = ry + (ROW_H - 8) / 2;

        if (r >= count) {
            tft.fillRect(0, ry, W, ROW_H, COL_BG);
            continue;
        }

        const SatInfo& s = gpsData.sats[order[r]];

        tft.setTextColor(satColor(s.prn), COL_BG);
        tft.setTextSize(1);
        tft.setCursor(4, ty);
        tft.printf("%c%02d", satConstellation(s.prn), s.prn);

        drawSegmentedBar(LEFT_W, ry + 3, BAR_MAX_W, BAR_H, s.snr);

        tft.setTextColor(s.used ? COL_GREEN : COL_DIM, COL_BG);
        tft.setCursor(LEFT_W + BAR_MAX_W + BAR_PAD, ty);
        tft.printf("%2d", s.snr);

        tft.setCursor(W - 14, ty);
        tft.print(s.used ? "*" : " ");
    }
}

void PageSignals::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    drawBars();
}

void PageSignals::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    drawBars();
}
