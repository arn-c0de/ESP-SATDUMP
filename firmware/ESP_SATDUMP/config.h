#pragma once

// ─── Display ────────────────────────────────────────────────────────────────
#define TFT_WIDTH       480
#define TFT_HEIGHT      320
#define TFT_ROTATION    1       // landscape

// ─── GPS (Serial2 / UART2) ──────────────────────────────────────────────────
#define GPS_RX_PIN       2      // ESP RX ← GPS TX
#define GPS_TX_PIN      15      // ESP TX → GPS RX
#define GPS_BAUD        9600

// ─── Rotary Encoder (KY-040) ────────────────────────────────────────────────
#define ENC_CLK_PIN     33
#define ENC_DT_PIN      32
#define ENC_SW_PIN      25
#define ENC_LONG_MS     600     // long-press threshold (ms)

// ─── Status bar ─────────────────────────────────────────────────────────────
#define STATUS_BAR_H    20

// ─── Update interval ────────────────────────────────────────────────────────
#define UPDATE_MS       250

// ─── NMEA ring buffer ───────────────────────────────────────────────────────
#define NMEA_LINES      12
#define NMEA_LINE_LEN   80

// ─── Satellite tracking ─────────────────────────────────────────────────────
#define MAX_SATS        16

// ─── RGB565 dark theme ──────────────────────────────────────────────────────
#define COL_BG          0x0820u  // deep navy
#define COL_ACCENT      0x07FFu  // cyan
#define COL_TEXT        0xFFFFu  // white
#define COL_DIM         0x4208u  // dark grey
#define COL_GREEN       0x07E0u  // signal good
#define COL_YELLOW      0xFFE0u  // signal fair
#define COL_RED         0xF800u  // signal weak/no-fix
#define COL_STATUS_BG   0x000Fu  // very dark blue for status bar

// ─── SNR thresholds ─────────────────────────────────────────────────────────
#define SNR_GOOD        35
#define SNR_FAIR        20
