#include "encoder.h"

// ─── Volatile state touched in ISR ──────────────────────────────────────────
static volatile int8_t   _encDelta    = 0;   // accumulated rotation
static volatile uint8_t  _lastCLK     = HIGH;

// ─── Button state ────────────────────────────────────────────────────────────
static uint32_t _btnPressTime  = 0;
static bool     _btnPressed    = false;
static bool     _longFired     = false;

// ─── ISR: called on any change of CLK pin ────────────────────────────────────
static void IRAM_ATTR encoderISR() {
    uint8_t clk = (uint8_t)digitalRead(ENC_CLK_PIN);
    uint8_t dt  = (uint8_t)digitalRead(ENC_DT_PIN);
    if (clk != _lastCLK) {
        _lastCLK = clk;
        if (clk == LOW) {
            _encDelta += (dt != clk) ? +1 : -1;
        }
    }
}

void encoderInit() {
    pinMode(ENC_CLK_PIN, INPUT_PULLUP);
    pinMode(ENC_DT_PIN,  INPUT_PULLUP);
    pinMode(ENC_SW_PIN,  INPUT_PULLUP);
    _lastCLK = (uint8_t)digitalRead(ENC_CLK_PIN);
    attachInterrupt(digitalPinToInterrupt(ENC_CLK_PIN), encoderISR, CHANGE);
    attachInterrupt(digitalPinToInterrupt(ENC_DT_PIN),  encoderISR, CHANGE);
}

EncEvent encoderRead() {
    // ── Rotation ──────────────────────────────────────────────────────────
    noInterrupts();
    int8_t delta = _encDelta;
    _encDelta = 0;
    interrupts();

    if (delta > 0) return EncEvent::CW;
    if (delta < 0) return EncEvent::CCW;

    // ── Button ────────────────────────────────────────────────────────────
    bool sw = (digitalRead(ENC_SW_PIN) == LOW);  // active-low

    if (sw && !_btnPressed) {
        _btnPressed   = true;
        _longFired    = false;
        _btnPressTime = millis();
    }

    if (_btnPressed) {
        uint32_t held = millis() - _btnPressTime;

        if (!sw) {
            // Released
            _btnPressed = false;
            if (!_longFired && held < (uint32_t)ENC_LONG_MS) {
                return EncEvent::CLICK;
            }
        } else if (!_longFired && held >= (uint32_t)ENC_LONG_MS) {
            _longFired = true;
            return EncEvent::LONG_PRESS;
        }
    }

    return EncEvent::NONE;
}
