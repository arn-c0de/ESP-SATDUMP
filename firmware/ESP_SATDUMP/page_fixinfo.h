#pragma once
#include "page_manager.h"

class PageFixInfo : public Page {
public:
    const char* name() const override { return "FIX INFO"; }
    void        onEnter()             override;
    void        update()              override;
    void        onEncoder(EncEvent)   override {}
};
