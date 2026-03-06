#pragma once
#include <TFT_eSPI.h>
#include "config.h"

extern TFT_eSPI tft;

void displayInit();

// Toggle between portrait (320×480) and landscape (480×320) and clear screen
void displayToggleRotation();
bool displayIsPortrait();

void drawStatusBar(const char* pageName, uint8_t satsInView, uint8_t fixQuality);
void clearContent();
uint16_t snrColor(uint8_t snr);
