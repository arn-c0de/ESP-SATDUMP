#pragma once

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                        USER SETTINGS                                    ║
// ║  Alles hier anpassen, dann flashen.                                     ║
// ╚══════════════════════════════════════════════════════════════════════════╝

// ── Standard-Ausrichtung (kann per Knopf an BTN_ROT_PIN umgeschaltet werden)
//    1 = Hochkant / Portrait  (320×480)
//    0 = Querformat / Landscape (480×320)
#define DEFAULT_PORTRAIT    1

// ── Serielle NMEA-Ausgabe (GPS-Sätze über USB-Serial ausgeben)
//    1 = an  (gut zum Testen)
//    0 = aus (sauberere Serial-Ausgabe im Betrieb)
#define SERIAL_LOG_GPS      1

// ── Welche NMEA-Sätze im Log ausblenden? (1 = ausblenden, 0 = zeigen)
#define LOG_HIDE_VTG    1   // GPVTG — Kurs/Geschwindigkeit (nicht genutzt)
#define LOG_HIDE_GLL    1   // GPGLL — Pos. (doppelt zu GGA)
#define LOG_HIDE_GGA    0   // GPGGA — Fix-Daten
#define LOG_HIDE_RMC    0   // GPRMC — Empfohlenes Minimum
#define LOG_HIDE_GSV    0   // GPGSV — Satelliten-Details
#define LOG_HIDE_GSA    0   // GPGSA — Aktive Satelliten

// ── Screen-Update-Intervall in Millisekunden
#define UPDATE_MS       250

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                        HARDWARE PINS                                    ║
// ╚══════════════════════════════════════════════════════════════════════════╝

// ── Display (TFT_eSPI — Pinbelegung in User_Setup.h)
#define TFT_WIDTH       480     // physische Breite  (Landscape-Referenz)
#define TFT_HEIGHT      320     // physische Höhe    (Landscape-Referenz)
#define ROT_PORTRAIT    0       // TFT-Rotation hochkant (320×480)
#define ROT_LANDSCAPE   1       // TFT-Rotation quer     (480×320)
#define TFT_ROTATION    (DEFAULT_PORTRAIT ? ROT_PORTRAIT : ROT_LANDSCAPE)
#define BTN_ROT_PIN    26       // Taster: Ausrichtung umschalten

// ── GPS (UART2)
#define GPS_RX_PIN       2      // ESP RX ← GPS TX
#define GPS_TX_PIN      15      // ESP TX → GPS RX
#define GPS_BAUD        9600

// ── Drehencoder KY-040
#define ENC_CLK_PIN     33
#define ENC_DT_PIN      32
#define ENC_SW_PIN      25
#define ENC_LONG_MS     600     // Long-Press-Schwelle (ms)

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                        SYSTEM CONSTANTS                                 ║
// ╚══════════════════════════════════════════════════════════════════════════╝

#define STATUS_BAR_H    20
#define NMEA_LINES      12
#define NMEA_LINE_LEN   80
#define MAX_SATS        16

// ── Farbschema RGB565 (dunkles Navy-Theme)
#define COL_BG          0x0820u
#define COL_ACCENT      0x07FFu
#define COL_TEXT        0xFFFFu
#define COL_DIM         0x4208u
#define COL_GREEN       0x07E0u
#define COL_YELLOW      0xFFE0u
#define COL_RED         0xF800u
#define COL_STATUS_BG   0x000Fu

// ── Version
#define SATDUMP_VERSION "B-1.0"

// ── SNR-Schwellwerte (dBHz)
#define SNR_GOOD        35
#define SNR_FAIR        20
