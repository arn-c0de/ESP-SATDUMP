#pragma once
#include <Arduino.h>
#include "config.h"

enum class EncEvent : uint8_t {
    NONE       = 0,
    CW         = 1,  // clockwise rotation
    CCW        = 2,  // counter-clockwise rotation
    CLICK      = 3,  // short press
    LONG_PRESS = 4,  // press ≥ ENC_LONG_MS
};

void encoderInit();
EncEvent encoderRead();  // returns NONE when nothing pending (non-blocking)
