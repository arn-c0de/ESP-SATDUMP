#include "page_map.h"
#include "display_utils.h"
#include "gps_parser.h"
#include <SD.h>
#include <math.h>

static const uint16_t TILE_SIZE = 256;
static uint16_t lineBuffer[TILE_SIZE];

// Persistence across page switches
static double _persistLat = 0;
static double _persistLon = 0;

void PageMap::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    
    _lastLat = 0; // Force redraw logic
    _lastLon = 0;
    
    // Draw map immediately if we have a last known position
    if (gpsData.fix_quality > 0 || _persistLat != 0) {
        drawMap();
    } else {
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(2);
        const char* msg = "WAITING FOR FIX...";
        tft.setCursor(centeredX(msg, 2), tft.height() / 2);
        tft.print(msg);
    }
}

void PageMap::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.sats_used, gpsData.hdop, gpsData.fix_quality);
    
    if (gpsData.fix_quality > 0) {
        // Update persistent position
        _persistLat = gpsData.lat;
        _persistLon = gpsData.lon;

        // Redraw if moved significantly
        if (fabs(gpsData.lat - _lastLat) > 0.0001 || fabs(gpsData.lon - _lastLon) > 0.0001) {
            drawMap();
            _lastLat = gpsData.lat;
            _lastLon = gpsData.lon;
        }
    }
}

void PageMap::onEncoder(EncEvent ev) {
    if (ev == EncEvent::CW && _zoom < 18) {
        _zoom++;
        drawMap();
    } else if (ev == EncEvent::CCW && _zoom > 5) {
        _zoom--;
        drawMap();
    }
}
void PageMap::drawMap() {
    double lat, lon;

    if (gpsData.fix_quality > 0) {
        lat = gpsData.lat;
        lon = gpsData.lon;
    } else if (_persistLat != 0) {
        lat = _persistLat;
        lon = _persistLon;
    } else {
        return;
    }

    double lat_rad = lat * M_PI / 180.0;
    double n = pow(2.0, _zoom);

    // Global pixel coordinates (double)
    double gx = (lon + 180.0) / 360.0 * n * TILE_SIZE;
    double gy = (1.0 - log(tan(lat_rad) + 1.0/cos(lat_rad)) / M_PI) / 2.0 * n * TILE_SIZE;

    int centerX = tft.width() / 2;
    int centerY = STATUS_BAR_H + (tft.height() - STATUS_BAR_H) / 2;

    // We calculate an integer anchor for the (0,0) global pixel
    // This ensures all tiles are shifted by the same integer amount
    int anchorX = (int)round(centerX - gx);
    int anchorY = (int)round(centerY - gy);

    // Determine range of tiles to draw
    // Use floor to correctly handle negative coordinates
    int startTX = (int)floor((gx - centerX) / TILE_SIZE);
    int endTX   = (int)floor((gx + centerX) / TILE_SIZE);
    int startTY = (int)floor((gy - (centerY - STATUS_BAR_H)) / TILE_SIZE);
    int endTY   = (int)floor((gy + (tft.height() - centerY)) / TILE_SIZE);

    for (int ty = startTY; ty <= endTY; ty++) {
        for (int tx = startTX; tx <= endTX; tx++) {
            // Tiles are now placed exactly TILE_SIZE apart
            int screenX = anchorX + tx * TILE_SIZE;
            int screenY = anchorY + ty * TILE_SIZE;
            loadTile(tx, ty, screenX, screenY);
        }
    }

    // Draw crosshair at center
    uint16_t crossCol = (gpsData.fix_quality > 0) ? COL_RED : COL_DIM;
    tft.drawFastHLine(centerX - 10, centerY, 20, crossCol);
    tft.drawFastVLine(centerX, centerY - 10, 20, crossCol);
    tft.drawCircle(centerX, centerY, 5, crossCol);
}

void PageMap::loadTile(int x, int y, int screenX, int screenY) {
    char path[64];
    snprintf(path, sizeof(path), "/Tiles/%d/%d/%d.bin", _zoom, x, y);
    
    File file = SD.open(path);
    if (!file) {
        // Draw highly visible placeholder for missing tiles
        tft.fillRect(screenX, screenY, TILE_SIZE, TILE_SIZE, COL_BG);
        tft.drawRect(screenX, screenY, TILE_SIZE, TILE_SIZE, COL_YELLOW);
        tft.setTextColor(COL_YELLOW, COL_BG);
        tft.setTextSize(1);
        tft.setCursor(screenX + 10, screenY + 10);
        tft.printf("MISSING: %d/%d/%d", _zoom, x, y);
        return;
    }
    
    // Calculate clipping
    int drawX = screenX;
    int drawY = screenY;
    int drawW = TILE_SIZE;
    int drawH = TILE_SIZE;
    int offsetX = 0;
    int offsetY = 0;
    
    if (drawX < 0) { offsetX = -drawX; drawW += drawX; drawX = 0; }
    if (drawY < STATUS_BAR_H + 1) { offsetY = (STATUS_BAR_H + 1) - drawY; drawH -= offsetY; drawY = STATUS_BAR_H + 1; }
    if (drawX + drawW > tft.width()) { drawW = tft.width() - drawX; }
    if (drawY + drawH > tft.height()) { drawH = tft.height() - drawY; }
    
    if (drawW <= 0 || drawH <= 0) {
        file.close();
        return;
    }
    
    for (int row = 0; row < drawH; row++) {
        file.seek(((offsetY + row) * TILE_SIZE + offsetX) * 2);
        file.read((uint8_t*)lineBuffer, drawW * 2);
        tft.pushImage(drawX, drawY + row, drawW, 1, lineBuffer);
    }
    
    file.close();
}
