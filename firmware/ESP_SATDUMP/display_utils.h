#pragma once
#include <TFT_eSPI.h>
#include "config.h"

extern TFT_eSPI tft;

void displayInit();

// Toggle between portrait (320×480) and landscape (480×320) and clear screen
void displayToggleRotation();
bool displayIsPortrait();

void showSplash();
void drawStatusBar(const char* pageName, uint8_t satsInView, uint8_t fixQuality);
void clearContent();

// ── UI Helpers ──────────────────────────────────────────────────────────────
int16_t centeredX(const char* s, uint8_t sz);
void drawBadge(int16_t x, int16_t y, const char* text, uint16_t color, uint16_t textColor = COL_TEXT);
void drawValueRow(int16_t x, int16_t y, int16_t w, const char* label, const char* value, uint16_t valCol = COL_TEXT);
void drawLargeRow(int16_t x, int16_t y, int16_t w, const char* label, const char* value, uint16_t valCol = COL_TEXT);
void drawSeparator(int16_t x, int16_t y, int16_t w);
