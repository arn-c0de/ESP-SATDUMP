#pragma once
#include "page_manager.h"

class PageMap : public Page {
public:
    const char* name() const override { return "MAP"; }
    void        onEnter()             override;
    void        update()              override;
    void        onEncoder(EncEvent ev) override;

private:
    uint8_t _zoom = 15;
    double _lastLat = 0;
    double _lastLon = 0;
    
    void drawMap();
    void loadTile(int x, int y, int screenX, int screenY);
};
