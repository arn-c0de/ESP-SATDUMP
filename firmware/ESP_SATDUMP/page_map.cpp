#include "page_map.h"
#include "display_utils.h"
#include "gps_parser.h"
#include <SD.h>
#include <math.h>

static const uint16_t TILE_SIZE = 256;
static uint16_t rowBuffer[TILE_SIZE]; 

static double _persistLat = 0;
static double _persistLon = 0;
static uint8_t _lastDrawZoom = 0;

void PageMap::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    _zoomMode = false;
    _lastLat = 0;
    _lastLon = 0;
    _lastDrawZoom = 0;
    if (gpsData.fix_quality > 0 || _persistLat != 0) {
        drawMap();
    }
}

void PageMap::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    if (gpsData.fix_quality > 0) {
        _persistLat = gpsData.lat;
        _persistLon = gpsData.lon;
        if (fabs(gpsData.lat - _lastLat) > 0.0001 || fabs(gpsData.lon - _lastLon) > 0.0001) {
            drawMap();
            _lastLat = gpsData.lat;
            _lastLon = gpsData.lon;
        }
    }
}

bool PageMap::onLongPress() {
    _zoomMode = true;
    drawZoomBadge();
    return true;  // handled — don't launch launcher
}

void PageMap::onEncoder(EncEvent ev) {
    if (_zoomMode) {
        if (ev == EncEvent::CLICK) {
            _zoomMode = false;
            clearZoomBadge();
            return;
        }
        if (ev == EncEvent::CW && _zoom < 18) {
            _zoom++;
            drawMap();
            drawZoomBadge();
        } else if (ev == EncEvent::CCW && _zoom > 5) {
            _zoom--;
            drawMap();
            drawZoomBadge();
        }
    }
}

void PageMap::drawMap() {
    double lat, lon;
    if (gpsData.fix_quality > 0) { lat = gpsData.lat; lon = gpsData.lon; }
    else if (_persistLat != 0) { lat = _persistLat; lon = _persistLon; }
    else return;

    if (_lastDrawZoom != _zoom) {
        tft.fillRect(0, STATUS_BAR_H + 1, tft.width(), tft.height() - STATUS_BAR_H - 1, COL_BG);
        _lastDrawZoom = _zoom;
    }

    double lat_rad = lat * M_PI / 180.0;
    double n = pow(2.0, _zoom);
    double gx = (lon + 180.0) / 360.0 * n * 256.0;
    double gy = (1.0 - log(tan(lat_rad) + 1.0/cos(lat_rad)) / M_PI) / 2.0 * n * 256.0;

    int W = tft.width();
    int H = tft.height();
    int CX = W / 2;
    int CY = STATUS_BAR_H + (H - STATUS_BAR_H) / 2;

    int anchorX = (int)floor(CX - gx + 0.5);
    int anchorY = (int)floor(CY - gy + 0.5);

    int startTX = (int)floor((gx - CX) / 256.0);
    int endTX   = (int)floor((gx + CX) / 256.0);
    int startTY = (int)floor((gy - (CY - STATUS_BAR_H)) / 256.0);
    int endTY   = (int)floor((gy + (H - CY)) / 256.0);

    for (int ty = startTY; ty <= endTY; ty++) {
        for (int tx = startTX; tx <= endTX; tx++) {
            loadTile(tx, ty, anchorX + tx * 256, anchorY + ty * 256);
        }
    }
    
    uint16_t crossCol = (gpsData.fix_quality > 0) ? COL_RED : COL_DIM;
    tft.drawFastHLine(CX - 10, CY, 20, crossCol);
    tft.drawFastVLine(CX, CY - 10, 20, crossCol);
    tft.drawCircle(CX, CY, 5, crossCol);
}

void PageMap::drawZoomBadge() {
    // Bottom-left corner: dark pill showing "ZOOM  Z:15"
    char buf[16];
    snprintf(buf, sizeof(buf), "ZOOM  Z:%d", _zoom);
    int bx = 4, by = tft.height() - 20, bw = 88, bh = 16;
    tft.fillRect(bx, by, bw, bh, 0x2104);          // very dark grey
    tft.drawRect(bx, by, bw, bh, COL_ACCENT);
    tft.setTextColor(COL_ACCENT, 0x2104);
    tft.setTextSize(1);
    tft.setCursor(bx + 4, by + 4);
    tft.print(buf);
}

void PageMap::clearZoomBadge() {
    // Erase badge area with background colour
    tft.fillRect(4, tft.height() - 20, 88, 16, COL_BG);
}

void PageMap::loadTile(int tx, int ty, int x, int y) {
    char path[64];
    snprintf(path, sizeof(path), "/Tiles/%d/%d/%d.bin", _zoom, tx, ty);
    
    File file = SD.open(path);
    int vY1 = STATUS_BAR_H + 1;
    int vY2 = tft.height() - 1;

    int drawX = x, drawW = 256, drawY = y, drawH = 256;
    int offX = 0, offY = 0;

    if (drawX < 0) { offX = -drawX; drawW -= offX; drawX = 0; }
    if (drawX + drawW > tft.width()) { drawW = tft.width() - drawX; }
    if (drawY < vY1) { offY = vY1 - drawY; drawH -= offY; drawY = vY1; }
    if (drawY + drawH > tft.height()) { drawH = tft.height() - drawY; }

    if (drawW <= 0 || drawH <= 0) { if(file) file.close(); return; }

    if (!file) {
        tft.fillRect(drawX, drawY, drawW, drawH, COL_BG);
        return;
    }

    // Read SD row → release SPI → write TFT row (never overlap SD + TFT on same bus)
    for (int r = 0; r < drawH; r++) {
        file.seek(((offY + r) * 256 + offX) * 2);
        file.read((uint8_t*)rowBuffer, drawW * 2);   // SD read  (SPI free for TFT)
        tft.startWrite();
        tft.setAddrWindow(drawX, drawY + r, drawW, 1);
        tft.pushColors(rowBuffer, drawW, true);
        tft.endWrite();                               // release SPI before next SD read
    }
    file.close();
}
