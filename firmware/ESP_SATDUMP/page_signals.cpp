#include "page_signals.h"
#include "display_utils.h"
#include "gps_parser.h"

// ── Constellation from PRN ────────────────────────────────────────────────────
// Returns single-char prefix and full name
static char constellationChar(uint8_t prn) {
    if (prn >= 1   && prn <= 32)  return 'G';  // GPS
    if (prn >= 33  && prn <= 64)  return 'S';  // SBAS (WAAS/EGNOS)
    if (prn >= 65  && prn <= 96)  return 'R';  // GLONASS
    if (prn >= 193 && prn <= 202) return 'J';  // QZSS
    if (prn >= 201 && prn <= 235) return 'C';  // BeiDou
    if (prn >= 301 && prn <= 336) return 'E';  // Galileo
    return '?';
}

static const char* constellationName(uint8_t prn) {
    if (prn >= 1   && prn <= 32)  return "GPS";
    if (prn >= 33  && prn <= 64)  return "SBAS";
    if (prn >= 65  && prn <= 96)  return "GLO";
    if (prn >= 193 && prn <= 202) return "QZSS";
    if (prn >= 201 && prn <= 235) return "BDS";
    if (prn >= 301 && prn <= 336) return "GAL";
    return "???";
}

// ── Layout constants ──────────────────────────────────────────────────────────
// Left label: "G01" (3 chars) + space = 24px at size 1
static const int16_t LEFT_W   = 24;   // PRN+constellation label width
static const int16_t RIGHT_W  = 54;   // "42dB 52°" value width
static const int16_t BAR_PAD  = 2;

static void drawBars() {
    int16_t W       = tft.width();
    int16_t H       = tft.height();
    int16_t BAR_MAX_W = W - LEFT_W - RIGHT_W - BAR_PAD * 2;
    int16_t ROW_H   = (H - STATUS_BAR_H - 4) / MAX_SATS;
    int16_t BAR_H   = ROW_H - 4;
    int16_t y0      = STATUS_BAR_H + 4;

    // Sort visible satellites by constellation then PRN
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
        int16_t ty  = ry + (ROW_H - 8) / 2;  // text y (vertically centred)

        if (r >= count) {
            tft.fillRect(0, ry, W, ROW_H, COL_BG);
            continue;
        }

        const SatInfo& s = gpsData.sats[order[r]];
        uint16_t lblCol = s.used ? COL_ACCENT : COL_TEXT;

        // ── Left: "G01" ─────────────────────────────────────────────────────
        char prnBuf[5];
        snprintf(prnBuf, sizeof(prnBuf), "%c%02d", constellationChar(s.prn), s.prn);
        tft.setTextColor(lblCol, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(2, ty);
        tft.print(prnBuf);

        // ── SNR bar ──────────────────────────────────────────────────────────
        int16_t barW = (s.snr * BAR_MAX_W) / 50;
        if (barW > BAR_MAX_W) barW = BAR_MAX_W;
        uint16_t barCol = snrColor(s.snr);
        if (barW > 0)
            tft.fillRect(LEFT_W, ry + 2, barW, BAR_H, barCol);
        tft.fillRect(LEFT_W + barW, ry + 2, BAR_MAX_W - barW, BAR_H, COL_DIM);

        // ── Right: SNR + elevation ───────────────────────────────────────────
        char infoBuf[16];
        snprintf(infoBuf, sizeof(infoBuf), "%2ddB%3d\x07", s.snr, s.elev);
        // \x07 = small degree-like char; replace with a real degree symbol workaround
        snprintf(infoBuf, sizeof(infoBuf), "%2ddB %2d`", s.snr, s.elev);
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setCursor(LEFT_W + BAR_MAX_W + BAR_PAD * 2, ty);
        tft.print(infoBuf);
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
