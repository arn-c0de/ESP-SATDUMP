#include "gps_parser.h"

GpsData gpsData = {};

// ─── Internal NMEA line buffer ───────────────────────────────────────────────
static char   _buf[84];
static uint8_t _bufLen = 0;

// ─── Split a comma-delimited NMEA sentence into fields ─────────────────────
// Returns number of fields found.  fields[] points into the sentence buffer
// (which is modified by inserting '\0').
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

// ─── Convert NMEA lat/lon ddmm.mmmm → decimal degrees ──────────────────────
static float nmeaToDeg(const char* val, char hemi) {
    if (!val || !*val) return 0.0f;
    float raw = atof(val);
    int   deg = (int)(raw / 100);
    float min = raw - deg * 100.0f;
    float dec = deg + min / 60.0f;
    if (hemi == 'S' || hemi == 'W') dec = -dec;
    return dec;
}

// ─── Parse $GPGGA ────────────────────────────────────────────────────────────
static void parseGGA(char* fields[], uint8_t n) {
    // $GPGGA,hhmmss.ss,lat,N,lon,E,quality,sats,hdop,alt,M,...
    if (n < 10) return;
    if (*fields[6]) gpsData.fix_quality = (uint8_t)atoi(fields[6]);
    if (*fields[7]) gpsData.sats_used   = (uint8_t)atoi(fields[7]);
    if (*fields[8]) gpsData.hdop        = atof(fields[8]);
    if (*fields[9]) gpsData.alt_m       = atof(fields[9]);
    if (*fields[2] && *fields[3])
        gpsData.lat = nmeaToDeg(fields[2], fields[3][0]);
    if (*fields[4] && *fields[5])
        gpsData.lon = nmeaToDeg(fields[4], fields[5][0]);
    // Parse UTC time from field[1]: hhmmss.ss
    if (strlen(fields[1]) >= 6) {
        gpsData.hour   = (fields[1][0]-'0')*10 + (fields[1][1]-'0');
        gpsData.minute = (fields[1][2]-'0')*10 + (fields[1][3]-'0');
        gpsData.second = (fields[1][4]-'0')*10 + (fields[1][5]-'0');
    }
}

// ─── Parse $GPRMC ────────────────────────────────────────────────────────────
static void parseRMC(char* fields[], uint8_t n) {
    // $GPRMC,hhmmss,A,lat,N,lon,E,speed_kn,course,date,...
    if (n < 10) return;
    if (*fields[7]) gpsData.speed_kmh  = atof(fields[7]) * 1.852f;
    if (*fields[8]) gpsData.course_deg = atof(fields[8]);
    // date: ddmmyy
    if (strlen(fields[9]) >= 6) {
        gpsData.day   = (fields[9][0]-'0')*10 + (fields[9][1]-'0');
        gpsData.month = (fields[9][2]-'0')*10 + (fields[9][3]-'0');
        gpsData.year  = 2000 + (fields[9][4]-'0')*10 + (fields[9][5]-'0');
    }
}

// ─── Parse $GPGSV ────────────────────────────────────────────────────────────
// Sentence format: $GPGSV,totalMsg,msgNum,totalSats,[prn,el,az,snr]*4
static void parseGSV(char* fields[], uint8_t n) {
    if (n < 4) return;
    uint8_t msgNum  = (uint8_t)atoi(fields[2]);
    uint8_t total   = (uint8_t)atoi(fields[3]);
    gpsData.sats_inview = total;

    // On first message reset satellite table
    if (msgNum == 1) {
        memset(gpsData.sats, 0, sizeof(gpsData.sats));
    }

    // Each satellite block starts at field[4], 4 fields per sat
    for (uint8_t i = 0; i < 4 && (4 + i*4 + 3) < n; i++) {
        uint8_t base = 4 + i * 4;
        uint8_t prn  = (uint8_t)atoi(fields[base]);
        if (!prn) continue;

        // Find existing slot or free slot
        int8_t slot = -1;
        for (uint8_t s = 0; s < MAX_SATS; s++) {
            if (gpsData.sats[s].prn == prn) { slot = s; break; }
        }
        if (slot < 0) {
            for (uint8_t s = 0; s < MAX_SATS; s++) {
                if (!gpsData.sats[s].prn) { slot = s; break; }
            }
        }
        if (slot < 0) continue;  // table full

        gpsData.sats[slot].prn  = prn;
        gpsData.sats[slot].elev = (int16_t)atoi(fields[base+1]);
        gpsData.sats[slot].azim = (int16_t)atoi(fields[base+2]);
        gpsData.sats[slot].snr  = (uint8_t)atoi(fields[base+3]);
    }
}

// ─── Parse $GPGSA ────────────────────────────────────────────────────────────
// Marks which PRNs are used in the current fix
static void parseGSA(char* fields[], uint8_t n) {
    if (n < 3) return;
    // Clear 'used' flags
    for (uint8_t s = 0; s < MAX_SATS; s++) gpsData.sats[s].used = false;
    // Fields 3..14 are PRNs used
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

// ─── Validate NMEA checksum ──────────────────────────────────────────────────
static bool validateChecksum(const char* sentence) {
    // sentence starts with '$', ends with '*XX\r\n or *XX\0
    const char* star = strchr(sentence, '*');
    if (!star) return false;  // Fix: require '*' for checksum validation
    uint8_t calc = 0;
    for (const char* p = sentence + 1; p < star; p++) calc ^= (uint8_t)*p;
    uint8_t given = (uint8_t)strtol(star + 1, nullptr, 16);
    return calc == given;
}

// ─── Dispatch a complete NMEA sentence ──────────────────────────────────────
static void dispatchSentence(char* sentence) {
    if (!validateChecksum(sentence)) return;

    // Copy to gpsData.last_sentence (strip '$')
    strncpy(gpsData.last_sentence, sentence, sizeof(gpsData.last_sentence) - 1);
    gpsData.last_sentence[sizeof(gpsData.last_sentence)-1] = '\0';

    // Work on a mutable copy for field splitting
    char copy[84];
    strncpy(copy, sentence + 1, sizeof(copy) - 1);  // skip '$'
    copy[sizeof(copy)-1] = '\0';

    char* fields[25];
    uint8_t n = splitFields(copy, fields, 25);
    if (n < 1) return;

    if      (strcmp(fields[0], "GPGGA") == 0) parseGGA(fields, n);
    else if (strcmp(fields[0], "GPRMC") == 0) parseRMC(fields, n);
    else if (strcmp(fields[0], "GPGSV") == 0) parseGSV(fields, n);
    else if (strcmp(fields[0], "GPGSA") == 0) parseGSA(fields, n);
    // GNRMC, GNGGA etc. — same parsing, just different talker IDs
    else if (strcmp(fields[0], "GNGGA") == 0) parseGGA(fields, n);
    else if (strcmp(fields[0], "GNRMC") == 0) parseRMC(fields, n);
    else if (strcmp(fields[0], "GNGSV") == 0) parseGSV(fields, n);
    else if (strcmp(fields[0], "GNGSA") == 0) parseGSA(fields, n);
}

// ─── Public API ─────────────────────────────────────────────────────────────
void gpsParserInit() {
    Serial2.begin(GPS_BAUD, SERIAL_8N1, GPS_RX_PIN, GPS_TX_PIN);
    memset(&gpsData, 0, sizeof(gpsData));
}

void gpsParserUpdate() {
    static bool discarding = false;
    while (Serial2.available()) {
        char c = (char)Serial2.read();
        Serial.write(c);  // raw GPS echo → USB serial (for diagnostics)

        if (c == '$') {
            _bufLen = 0;
            discarding = false;
        }
        
        if (discarding) continue;

        if (_bufLen < sizeof(_buf) - 1) {
            _buf[_bufLen++] = c;
        } else {
            // Buffer overflow: discard this sentence
            _bufLen = 0;
            discarding = true;
            continue;
        }

        if (c == '\n') {
            _buf[_bufLen] = '\0';
            if (_bufLen > 0 && _buf[0] == '$') {
                dispatchSentence(_buf);
            }
            _bufLen = 0;
        }
    }
}
