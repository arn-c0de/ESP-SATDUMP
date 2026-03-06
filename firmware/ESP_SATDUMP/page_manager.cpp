#include "page_manager.h"
#include "gps_parser.h"

void PageManager::addPage(Page* p) {
    if (_count < MAX_PAGES) _pages[_count++] = p;
}

void PageManager::begin() {
    if (_count == 0) return;
    switchTo(0);
}

void PageManager::switchTo(uint8_t idx) {
    _current = idx % _count;
    _pages[_current]->onEnter();
    _lastUpdate = millis();
}

void PageManager::loop() {
    if (_count == 0) return;

    // Handle encoder
    EncEvent ev = encoderRead();
    if (ev == EncEvent::CW) {
        switchTo((_current + 1) % _count);
        return;
    }
    if (ev == EncEvent::CCW) {
        switchTo((_current + _count - 1) % _count);
        return;
    }
    if (ev != EncEvent::NONE) {
        // Pass clicks / long presses to active page
        _pages[_current]->onEncoder(ev);
    }

    // Periodic update
    uint32_t now = millis();
    if (now - _lastUpdate >= UPDATE_MS) {
        _lastUpdate = now;
        gpsParserUpdate();
        _pages[_current]->update();
    }
    // Always keep GPS fed even between screen updates
    gpsParserUpdate();
}
