#include "gps_parser.h"

GpsData gpsData = {};

static char   _buf[84];
static uint8_t _bufLen = 0;

// ─── Sat Utils Implementation ───────────────────────────────────────────────
char satConstellation(uint8_t prn) {
    if (prn >= 1   && prn <= 32)  return 'G'; // GPS
    if (prn >= 33  && prn <= 64)  return 'S'; // SBAS
    if (prn >= 65  && prn <= 96)  return 'R'; // GLONASS
    if (prn >= 193 && prn <= 202) return 'J'; // QZSS
    if (prn >= 201 && prn <= 235) return 'C'; // BeiDou
    if (prn >= 301 && prn <= 336) return 'E'; // Galileo
    return '?';
}

uint16_t satColor(uint8_t prn) {
    if (prn >= 1   && prn <= 32)  return COL_ACCENT; // GPS: Cyan
    if (prn >= 65  && prn <= 96)  return COL_YELLOW; // GLONASS: Yellow
    if (prn >= 301 && prn <= 336) return COL_GREEN;  // Galileo: Green
    return COL_TEXT;
}

uint16_t satSnrColor(uint8_t snr) {
    if (snr >= SNR_GOOD) return COL_GREEN;
    if (snr >= SNR_FAIR) return COL_YELLOW;
    if (snr > 0)         return COL_RED;
    return COL_DIM;
}

// ─── NMEA Parser Logic ──────────────────────────────────────────────────────
static uint8_t splitFields(char* sentence, char* fields[], uint8_t maxFields) {
    uint8_t n = 0;
    fields[n++] = sentence;
    for (char* p = sentence; *p && n < maxFields; p++) {
        if (*p == ',' || *p == '*') {
            *p = '\0';
            fields[n++] = p + 1;
        }
    }
    return n;
}

static float nmeaToDeg(const char* val, char hemi) {
    if (!val || !*val) return 0.0f;
    float raw = atof(val);
    int   deg = (int)(raw / 100);
    float min = raw - deg * 100.0f;
    float dec = deg + min / 60.0f;
    if (hemi == 'S' || hemi == 'W') dec = -dec;
    return dec;
}

static void parseGGA(char* fields[], uint8_t n) {
    if (n < 10) return;
    if (*fields[6]) gpsData.fix_quality = (uint8_t)atoi(fields[6]);
    if (*fields[7]) gpsData.sats_used   = (uint8_t)atoi(fields[7]);
    if (*fields[8]) gpsData.hdop        = atof(fields[8]);
    if (*fields[9]) gpsData.alt_m       = atof(fields[9]);
    if (*fields[2] && *fields[3]) gpsData.lat = nmeaToDeg(fields[2], fields[3][0]);
    if (*fields[4] && *fields[5]) gpsData.lon = nmeaToDeg(fields[4], fields[5][0]);
    if (strlen(fields[1]) >= 6) {
        gpsData.hour   = (fields[1][0]-'0')*10 + (fields[1][1]-'0');
        gpsData.minute = (fields[1][2]-'0')*10 + (fields[1][3]-'0');
        gpsData.second = (fields[1][4]-'0')*10 + (fields[1][5]-'0');
    }
}

static void parseRMC(char* fields[], uint8_t n) {
    if (n < 10) return;
    if (*fields[7]) gpsData.speed_kmh  = atof(fields[7]) * 1.852f;
    if (*fields[8]) gpsData.course_deg = atof(fields[8]);
    if (strlen(fields[9]) >= 6) {
        gpsData.day   = (fields[9][0]-'0')*10 + (fields[9][1]-'0');
        gpsData.month = (fields[9][2]-'0')*10 + (fields[9][3]-'0');
        gpsData.year  = 2000 + (fields[9][4]-'0')*10 + (fields[9][5]-'0');
    }
}

static void parseGSV(char* fields[], uint8_t n) {
    if (n < 4) return;
    uint8_t msgNum  = (uint8_t)atoi(fields[2]);
    uint8_t total   = (uint8_t)atoi(fields[3]);
    gpsData.sats_inview = total;

    for (uint8_t i = 0; i < 4 && (4 + i*4 + 3) < n; i++) {
        uint8_t base = 4 + i * 4;
        uint8_t prn  = (uint8_t)atoi(fields[base]);
        if (!prn) continue;

        int8_t slot = -1;
        for (uint8_t s = 0; s < MAX_SATS; s++) {
            if (gpsData.sats[s].prn == prn) { slot = s; break; }
        }
        if (slot < 0) {
            for (uint8_t s = 0; s < MAX_SATS; s++) {
                if (!gpsData.sats[s].prn) { slot = s; break; }
            }
        }
        if (slot < 0) continue;

        gpsData.sats[slot].prn  = prn;
        gpsData.sats[slot].elev = (int16_t)atoi(fields[base+1]);
        gpsData.sats[slot].azim = (int16_t)atoi(fields[base+2]);
        gpsData.sats[slot].snr  = (uint8_t)atoi(fields[base+3]);
    }
}

static void parseGSA(char* fields[], uint8_t n) {
    if (n < 3) return;
    for (uint8_t f = 3; f < 15 && f < n; f++) {
        if (!fields[f][0]) continue;
        uint8_t prn = (uint8_t)atoi(fields[f]);
        if (!prn) continue;
        for (uint8_t s = 0; s < MAX_SATS; s++) {
            if (gpsData.sats[s].prn == prn) {
                gpsData.sats[s].used = true;
                break;
            }
        }
    }
}

static bool validateChecksum(const char* sentence) {
    const char* star = strchr(sentence, '*');
    if (!star) return false;
    uint8_t calc = 0;
    for (const char* p = sentence + 1; p < star; p++) calc ^= (uint8_t)*p;
    uint8_t given = (uint8_t)strtol(star + 1, nullptr, 16);
    return calc == given;
}

static void dispatchSentence(char* sentence) {
#if SERIAL_LOG_GPS
    Serial.println(sentence);
#endif

    if (!validateChecksum(sentence)) return;

    strncpy(gpsData.last_sentence, sentence, sizeof(gpsData.last_sentence) - 1);
    gpsData.last_sentence[sizeof(gpsData.last_sentence)-1] = '\0';

    char copy[84];
    strncpy(copy, sentence + 1, sizeof(copy) - 1);
    copy[sizeof(copy)-1] = '\0';

    char* fields[25];
    uint8_t n = splitFields(copy, fields, 25);
    if (n < 1) return;

    const char* type = fields[0];
    size_t tLen = strlen(type);
    if (tLen < 3) return;
    const char* suffix = type + (tLen - 3);

    if      (strcmp(suffix, "GGA") == 0) parseGGA(fields, n);
    else if (strcmp(suffix, "RMC") == 0) parseRMC(fields, n);
    else if (strcmp(suffix, "GSV") == 0) parseGSV(fields, n);
    else if (strcmp(suffix, "GSA") == 0) parseGSA(fields, n);
}

void gpsParserInit() {
    Serial.printf("[GPS] Init UART2 (RX=%d, TX=%d, Baud=%d)\n", 
                  GPS_RX_PIN, GPS_TX_PIN, GPS_BAUD);
    
    Serial2.end();
    delay(100);
    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    Serial2.setRxBufferSize(1024);
    
    memset(&gpsData, 0, sizeof(gpsData));
    Serial.println("[GPS] Waiting for data...");
}

void gpsParserUpdate() {
    static int rawDumpCount = 0;
    static bool firstData = true;

    while (Serial2.available()) {
        uint8_t c = (uint8_t)Serial2.read();
        
        if (firstData) {
            Serial.println("[GPS] FIRST BYTES RECEIVED!");
            firstData = false;
        }

        if (rawDumpCount < 100) {
            Serial.printf("[RAW %02d] 0x%02X '%c'\n", rawDumpCount, c, (c >= 32 && c < 127) ? (char)c : '.');
            rawDumpCount++;
            if (rawDumpCount == 100) Serial.println("[GPS] Raw dump complete.");
        }

        if (c == '$') _bufLen = 0;
        if (_bufLen < sizeof(_buf) - 1) _buf[_bufLen++] = (char)c;
        if (c == '\n') {
            _buf[_bufLen] = '\0';
            if (_bufLen > 0 && _buf[0] == '$') dispatchSentence(_buf);
            _bufLen = 0;
        }
    }
}
