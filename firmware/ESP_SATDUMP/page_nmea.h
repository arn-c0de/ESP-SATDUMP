#pragma once
#include "page_manager.h"

class PageNMEA : public Page {
public:
    const char* name() const override { return "NMEA RAW"; }
    void        onEnter()             override;
    void        update()              override;
    void        onEncoder(EncEvent)   override;

private:
    // Ring buffer: NMEA_LINES x NMEA_LINE_LEN
    char    _ring[NMEA_LINES][NMEA_LINE_LEN];
    uint8_t _head     = 0;   // next write position
    uint8_t _count    = 0;   // how many lines stored
    int8_t  _scroll   = 0;   // scroll offset (lines from bottom)

    void pushLine(const char* s);
    void redraw();
};
