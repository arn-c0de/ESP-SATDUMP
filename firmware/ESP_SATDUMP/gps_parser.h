#pragma once
#include <Arduino.h>
#include "config.h"

// ─── Per-satellite data ──────────────────────────────────────────────────────
struct SatInfo {
    uint8_t  prn;       // 0 = empty slot
    int16_t  elev;      // degrees (0-90)
    int16_t  azim;      // degrees (0-359)
    uint8_t  snr;       // dBHz (0 = no signal)
    bool     used;      // included in fix (from GPGSA)
};

// ─── Top-level GPS data struct ───────────────────────────────────────────────
struct GpsData {
    // Fix quality: 0=no fix, 1=GPS, 2=DGPS
    uint8_t  fix_quality;
    uint8_t  sats_used;     // from GGA
    uint8_t  sats_inview;   // from GSV
    float    lat;           // decimal degrees, negative = S
    float    lon;           // decimal degrees, negative = W
    float    alt_m;         // metres above sea level
    float    hdop;
    float    speed_kmh;
    float    course_deg;
    // UTC time from RMC
    uint8_t  hour, minute, second;
    uint8_t  day, month;
    uint16_t year;

    SatInfo  sats[MAX_SATS];

    // Last raw NMEA sentence (null-terminated, up to 82 chars)
    char     last_sentence[84];
};

extern GpsData gpsData;

// ─── Parser API ─────────────────────────────────────────────────────────────
void gpsParserInit();
void gpsParserUpdate();  // call frequently from loop()
