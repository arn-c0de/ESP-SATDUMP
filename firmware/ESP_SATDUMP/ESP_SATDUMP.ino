/*
 * ESP-SATDUMP — GPS Satellite Tracker
 *
 * Hardware:
 *   - ESP32
 *   - LAFVIN 4.0" ST7796S TFT 480x320 (SPI)
 *   - NEO-6M GPS module (UART2: RX=GPIO2, TX=GPIO15)
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
    showSplash();
    encoderInit();
    gpsParserInit();
    pinMode(BTN_ROT_PIN, INPUT_PULLUP);

    pm.addPage(&pageSkyView);
    pm.addPage(&pageSignals);
    pm.addPage(&pageFixInfo);
    pm.addPage(&pageNMEA);
    pm.begin();
}

static void printStatus() {
    const char* fixStr = gpsData.fix_quality == 0 ? "NO FIX" :
                         gpsData.fix_quality == 1 ? "GPS FIX" : "DGPS FIX";
    Serial.printf("[STATUS] %s | sats=%d/%d | lat=%.6f lon=%.6f | alt=%.1fm | spd=%.1fkm/h | %02d:%02d:%02d %04d-%02d-%02d\n",
        fixStr,
        gpsData.sats_used, gpsData.sats_inview,
        gpsData.lat, gpsData.lon,
        gpsData.alt_m, gpsData.speed_kmh,
        gpsData.hour, gpsData.minute, gpsData.second,
        gpsData.year, gpsData.month, gpsData.day);
    for (uint8_t i = 0; i < MAX_SATS; i++) {
        if (gpsData.sats[i].prn) {
            Serial.printf("  PRN%02d el=%2d az=%3d snr=%2d %s\n",
                gpsData.sats[i].prn,
                gpsData.sats[i].elev,
                gpsData.sats[i].azim,
                gpsData.sats[i].snr,
                gpsData.sats[i].used ? "[FIX]" : "");
        }
    }
}

void loop() {
    // Rotation toggle button (GPIO26, active-low, debounced)
    static bool    lastBtnState = HIGH;
    static uint32_t btnPressedAt = 0;
    bool btnState = digitalRead(BTN_ROT_PIN);
    if (btnState == LOW && lastBtnState == HIGH) btnPressedAt = millis();
    if (btnState == HIGH && lastBtnState == LOW && (millis() - btnPressedAt) > 50) {
        displayToggleRotation();
        pm.forceRedraw();
    }
    lastBtnState = btnState;

    // Auto-status every 10 seconds
    static uint32_t lastStatus = 0;
    if (millis() - lastStatus >= 10000) {
        lastStatus = millis();
        printStatus();
    }

    // Serial commands:  s=status now  h=help
    if (Serial.available()) {
        char cmd = (char)Serial.read();
        if (cmd == 's') {
            printStatus();
        } else if (cmd == 'h') {
            Serial.println("[HELP] s=status  h=help");
        }
    }
    pm.loop();
}
