#pragma once
#include "page_manager.h"

class PageSignals : public Page {
public:
    const char* name() const override { return "SIGNALS"; }
    void        onEnter()             override;
    void        update()              override;
    void        onEncoder(EncEvent)   override {}
};
