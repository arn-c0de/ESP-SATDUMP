#include "page_map.h"
#include "display_utils.h"
#include "gps_parser.h"
#include <SD.h>
#include <math.h>

static const uint16_t TILE_SIZE = 256;
static uint16_t lineBuffer[TILE_SIZE];

void PageMap::onEnter() {
    tft.fillScreen(COL_BG);
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    _lastLat = 0; // Force redraw
    _lastLon = 0;
    drawMap();
}

void PageMap::update() {
    drawStatusBar(name(), gpsData.sats_inview, gpsData.fix_quality);
    
    // Only redraw if position changed significantly or no fix
    if (gpsData.fix_quality > 0) {
        if (fabs(gpsData.lat - _lastLat) > 0.0001 || fabs(gpsData.lon - _lastLon) > 0.0001) {
            drawMap();
            _lastLat = gpsData.lat;
            _lastLon = gpsData.lon;
        }
    } else {
        // No fix, maybe show "Waiting for Fix" message
        tft.setTextColor(COL_TEXT, COL_BG);
        tft.setTextSize(2);
        const char* msg = "WAITING FOR FIX...";
        tft.setCursor(centeredX(msg, 2), tft.height() / 2);
        tft.print(msg);
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
    if (gpsData.fix_quality == 0) return;

    double lat_rad = gpsData.lat * M_PI / 180.0;
    double n = pow(2.0, _zoom);
    
    // Global pixel coordinates
    double gx = (gpsData.lon + 180.0) / 360.0 * n * TILE_SIZE;
    double gy = (1.0 - log(tan(lat_rad) + 1.0/cos(lat_rad)) / M_PI) / 2.0 * n * TILE_SIZE;
    
    int tileX = (int)(gx / TILE_SIZE);
    int tileY = (int)(gy / TILE_SIZE);
    
    int centerX = tft.width() / 2;
    int centerY = (tft.height() + STATUS_BAR_H) / 2;
    
    // Relative position of center on screen in global pixel coordinates
    // We want (gx, gy) to be at (centerX, centerY)
    
    // Determine range of tiles to draw
    int startTX = (int)((gx - centerX) / TILE_SIZE);
    int endTX = (int)((gx + centerX) / TILE_SIZE);
    int startTY = (int)((gy - (centerY - STATUS_BAR_H)) / TILE_SIZE);
    int endTY = (int)((gy + (tft.height() - centerY)) / TILE_SIZE);
    
    for (int ty = startTY; ty <= endTY; ty++) {
        for (int tx = startTX; tx <= endTX; tx++) {
            int screenX = (int)(centerX - (gx - tx * TILE_SIZE));
            int screenY = (int)(centerY - (gy - ty * TILE_SIZE));
            loadTile(tx, ty, screenX, screenY);
        }
    }
    
    // Draw crosshair at center
    tft.drawFastHLine(centerX - 10, centerY, 20, COL_RED);
    tft.drawFastVLine(centerX, centerY - 10, 20, COL_RED);
    tft.drawCircle(centerX, centerY, 5, COL_RED);
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
