#pragma once

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                        USER SETTINGS                                    ║
// ║  Modify settings here, then compile and flash.                          ║
// ╚══════════════════════════════════════════════════════════════════════════╝

// ── Default Orientation (can be toggled via button on BTN_ROT_PIN)
//    1 = Portrait   (320×480)
//    0 = Landscape  (480×320)
#define DEFAULT_PORTRAIT    1

// ── Serial NMEA Output (Forward GPS sentences over USB-Serial)
//    1 = Enable  (Good for testing)
//    0 = Disable (Cleaner serial output during normal operation)
#define SERIAL_LOG_GPS      1

// ── Which NMEA sentences to hide in the log? (1 = hide, 0 = show)
#define LOG_HIDE_VTG    1   // GPVTG — Course/Speed (not used)
#define LOG_HIDE_GLL    1   // GPGLL — Position (duplicate of GGA)
#define LOG_HIDE_GGA    0   // GPGGA — Fix data
#define LOG_HIDE_RMC    0   // GPRMC — Recommended Minimum
#define LOG_HIDE_GSV    0   // GPGSV — Satellite details
#define LOG_HIDE_GSA    0   // GPGSA — Active satellites

// ── Screen update interval in milliseconds
#define UPDATE_MS       250

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                        HARDWARE PINS                                    ║
// ╚══════════════════════════════════════════════════════════════════════════╝

// ── Display (TFT_eSPI — Pin layout in User_Setup.h)
#define TFT_WIDTH       480     // Physical width  (Landscape reference)
#define TFT_HEIGHT      320     // Physical height (Landscape reference)
#define ROT_PORTRAIT    0       // TFT rotation portrait (320×480)
#define ROT_LANDSCAPE   1       // TFT rotation landscape (480×320)
#define TFT_ROTATION    (DEFAULT_PORTRAIT ? ROT_PORTRAIT : ROT_LANDSCAPE)
#define BTN_ROT_PIN    26       // Button: Toggle orientation

// ── GPS (UART2)
#define GPS_RX_PIN       15      // ESP RX ← GPS TX
#define GPS_TX_PIN      2      // ESP TX → GPS RX
#define GPS_BAUD        9600

// ── Rotary Encoder KY-040
#define ENC_CLK_PIN     33
#define ENC_DT_PIN      32
#define ENC_SW_PIN      25
#define ENC_LONG_MS     600     // Long-press threshold (ms)

// ╔══════════════════════════════════════════════════════════════════════════╗
// ║                        SYSTEM CONSTANTS                                 ║
// ╚══════════════════════════════════════════════════════════════════════════╝

#define STATUS_BAR_H    20
#define NMEA_LINES      12
#define NMEA_LINE_LEN   80
#define MAX_SATS        16

// ── Color Scheme RGB565 (Dark Navy Theme)
#define COL_BG          0x0820u
#define COL_ACCENT      0x07FFu
#define COL_TEXT        0xFFFFu
#define COL_DIM         0x4208u
#define COL_GREEN       0x07E0u
#define COL_YELLOW      0xFFE0u
#define COL_RED         0xF800u
#define COL_STATUS_BG   0x000Fu

// ── Version
#define SATDUMP_VERSION "B-1.1"

// ── SNR thresholds (dBHz)
#define SNR_GOOD        35
#define SNR_FAIR        20
