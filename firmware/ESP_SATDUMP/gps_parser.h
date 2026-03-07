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
    uint8_t  fix_quality;   // 0=no fix, 1=GPS, 2=DGPS
    uint8_t  sats_used;     
    uint8_t  sats_inview;   
    float    lat;           
    float    lon;           
    float    alt_m;         
    float    hdop;
    float    speed_kmh;
    float    course_deg;
    uint8_t  hour, minute, second;
    uint8_t  day, month;
    uint16_t year;

    SatInfo  sats[MAX_SATS];
    char     last_sentence[84];
};

extern GpsData gpsData;

// ─── Parser API ─────────────────────────────────────────────────────────────
void gpsParserInit();
void gpsParserUpdate();

// ─── Sat Utils ──────────────────────────────────────────────────────────────
char     satConstellation(uint8_t prn);
uint16_t satColor(uint8_t prn);
uint16_t satSnrColor(uint8_t snr);
