# AHH-1.0 Handheld: SD SPI Pin Specification Update

This document details the critical pin change for the **AHH-1.0 Handheld** hardware revision. To ensure maximum stability and compatibility with common SD card modules, the SD card bus has been moved to a dedicated SPI bus (HSPI).

## Change Overview

| Component | Signal | Old GPIO | **Final GPIO (AHH-1.0)** | Reason |
| :--- | :--- | :--- | :--- | :--- |
| **SD Card** | **MISO** | 19 | **12** | Moved to 12. |
| **SD Card** | **MOSI** | 23 | **14** | Moved to 14. |
| **SD Card** | **SCK**  | 18 | **22** | Dedicated SPI bus (HSPI). |
| **SD Card** | **CS**   | 13 | **13** | (Unchanged). |

## Technical Justification

Many generic SD card modules fail to properly release the MISO line (MISO Tri-state issue) when their Chip Select (CS) is high. This can create bus conflicts when display and SD peripherals share SPI lines.

By moving the SD card to the **HSPI peripheral** using the AHH-1.0 mapping (MISO=12, MOSI=14, SCK=22, CS=13), the two peripherals no longer share data lines and the system remains stable during flashing and boot.

## Implementation Details

The following components have been updated to reflect these changes:
- **Firmware:** `firmware/ESP_SATDUMP/launcher.cpp` now defaults to the new pins.
- **Bootloader/Launcher:** `ESP32-Multi-OS-Launcher/components/esp_launcher/Kconfig` updated with new defaults.
- **Documentation:** `HARDWARE_PINOUT_EN.md` and `docs/pinout.md` updated.

## Schematic Reference (AHH-1.0 Handheld)

```text
[ESP32]             [SD CARD]
GPIO 13  -------->  CS
GPIO 14  -------->  MOSI
GPIO 12  <--------  MISO
GPIO 22  -------->  SCK
```

The TFT display continues to use the standard **VSPI** pins (GPIO 18, 19, 23, 5).


## Pushbuttons (AHH-1.0)

- Pushbutton 1: GPIO26
- Pushbutton 2: GPIO27
