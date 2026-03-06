#pragma once
#include "page_manager.h"

class PageSkyView : public Page {
public:
    const char* name() const override { return "SKY VIEW"; }
    void        onEnter()             override;
    void        update()              override;
    void        onEncoder(EncEvent)   override {}
};
