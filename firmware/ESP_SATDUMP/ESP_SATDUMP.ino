/*
 * ESP-SATDUMP — GPS Satellite Tracker
 *
 * Hardware:
 *   - ESP32
 *   - LAFVIN 4.0" ST7796S TFT 480x320 (SPI)
 *   - NEO-6M GPS module (UART2: RX=GPIO26, TX=GPIO27)
 *   - KY-040 rotary encoder (CLK=GPIO33, DT=GPIO32, SW=GPIO25)
 *
 * Pages (cycle with encoder rotation):
 *   1. Sky View   — polar satellite plot
 *   2. Signals    — SNR bar chart
 *   3. Fix Info   — position / time / speed
 *   4. NMEA Raw   — scrolling sentence stream
 */

#include "config.h"
#include "display_utils.h"
#include "encoder.h"
#include "gps_parser.h"
#include "page_manager.h"
#include "page_skyview.h"
#include "page_signals.h"
#include "page_fixinfo.h"
#include "page_nmea.h"

static PageSkyView  pageSkyView;
static PageSignals  pageSignals;
static PageFixInfo  pageFixInfo;
static PageNMEA     pageNMEA;
static PageManager  pm;

void setup() {
    Serial.begin(115200);
    Serial.println("[ESP-SATDUMP] boot");

    displayInit();
    encoderInit();
    gpsParserInit();

    pm.addPage(&pageSkyView);
    pm.addPage(&pageSignals);
    pm.addPage(&pageFixInfo);
    pm.addPage(&pageNMEA);
    pm.begin();
}

void loop() {
    pm.loop();
}
