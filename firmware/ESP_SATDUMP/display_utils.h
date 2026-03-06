#pragma once
#include <TFT_eSPI.h>
#include "config.h"

extern TFT_eSPI tft;

void displayInit();
// Draw the 20-px status bar at the top.
// pageName  : short label drawn left
// satsInView: satellite count drawn centre
// fixQuality: 0=NO FIX, 1=GPS, 2=DGPS
void drawStatusBar(const char* pageName, uint8_t satsInView, uint8_t fixQuality);

// Convenience: fill background below status bar
void clearContent();

// Map SNR value to a RGB565 colour
uint16_t snrColor(uint8_t snr);
