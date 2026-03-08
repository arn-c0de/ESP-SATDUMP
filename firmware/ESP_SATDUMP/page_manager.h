#pragma once
#include <Arduino.h>
#include "encoder.h"

// ─── Abstract page base ──────────────────────────────────────────────────────
class Page {
public:
    virtual ~Page() = default;
    virtual const char* name()              const = 0;
    virtual void        onEnter()                 = 0;  // full redraw
    virtual void        update()                  = 0;  // called ~250ms
    virtual void        onEncoder(EncEvent ev)    = 0;

    // Override to true: PageManager forwards CW/CCW to page instead of switching pages
    virtual bool captureEncoder() const { return false; }
    // Override to true: PageManager won't launch launcher on long press
    virtual bool onLongPress()          { return false; }
};

// ─── Page manager ────────────────────────────────────────────────────────────
class PageManager {
public:
    void addPage(Page* p);
    void begin();           // call after all pages added
    void loop();            // call from Arduino loop()
    void forceRedraw();     // re-enter current page (after rotation change)

private:
    static const uint8_t MAX_PAGES = 8;
    Page*   _pages[MAX_PAGES];
    uint8_t _count       = 0;
    uint8_t _current     = 0;
    uint32_t _lastUpdate = 0;

    void switchTo(uint8_t idx);
};
