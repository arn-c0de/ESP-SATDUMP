# Architecture

## Module Overview

```
ESP_SATDUMP.ino        Entry point: setup() + loop()
  └─ PageManager       Owns page list, drives update loop, routes EncEvents
       ├─ PageSkyView  Polar satellite plot
       ├─ PageSignals  Horizontal SNR bar chart
       ├─ PageFixInfo  Position / time / speed readout
       └─ PageNMEA     Scrolling raw NMEA stream

config.h               All pins, colours, constants
display_utils          TFT_eSPI singleton + status bar + helpers
encoder                KY-040 interrupt driver → EncEvent queue
gps_parser             NMEA sentence parser → GpsData struct
```

## Data Flow

```
Serial2 (UART2 @ 9600)
    │
    └── gpsParserUpdate()   ← called from PageManager::loop() every iteration
              │
              ▼
         GpsData gpsData    (global singleton, updated in-place)
              │
    ┌─────────┼────────────┬──────────────┐
    ▼         ▼            ▼              ▼
 SkyView   Signals      FixInfo        NMEA
    │         │            │              │
    └─────────┴────────────┴──────────────┘
                     TFT_eSPI tft
```

## Update Cycle

`PageManager::loop()` runs in Arduino `loop()` at full speed:
1. Calls `gpsParserUpdate()` — drains Serial2 RX buffer into NMEA parser.
2. Checks encoder — routes CW/CCW to page switch; CLICK/LONG_PRESS to active page.
3. Every `UPDATE_MS` (250 ms) calls active page's `update()`.

## Page Switching

Encoder CW/CCW triggers `PageManager::switchTo()`, which calls `onEnter()` for a
full-screen redraw. Within a page, `update()` redraws only changed regions.

## NMEA Parser

Custom field-split parser — no external GPS library.
Sentences handled: `$GPGGA`, `$GPRMC`, `$GPGSV`, `$GPGSA`
(and `$GN*` multi-constellation variants).

Checksum validated for every sentence; sentences with bad checksums are silently
dropped.
