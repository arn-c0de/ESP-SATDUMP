# Hardware Pinout (ESP32 + TFT + Encoder + SD)

This document summarizes a reference wiring setup for the Gladiator WiFi Tool.

Important:
- TFT modules vary by driver and pinout. Always configure TFT_eSPI for your specific display.
- Treat the tables below as a starting point. Confirm your wiring against `ArduinoIDE/User_Setup.h` and your hardware.

## Rotary Encoder (KY-040)

| Encoder Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| DT | GPIO 32 | Rotation direction |
| CLK | GPIO 33 | Clock signal |
| SW | GPIO 25 | Button/switch |
| + | 3.3V | Power |
| GND | GND | Ground |

## SD Card Module (SPI)

| SD Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| CS | GPIO 13 | Chip select |
| MOSI | GPIO 23 | SPI MOSI |
| MISO | GPIO 19 | SPI MISO |
| SCK | GPIO 18 | SPI clock |
| VCC | 3.3V | Power |
| GND | GND | Ground |

## TFT Display (SPI, TFT_eSPI)

TFT wiring is display-dependent. The most common SPI signals are:

| TFT Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| VCC | 3.3V | Power |
| GND | GND | Ground |
| MOSI / SDI | GPIO 23 | SPI MOSI |
| SCK | GPIO 18 | SPI clock |
| CS | (configurable) | Chip select |
| DC | (configurable) | Data/command |
| RST | (configurable) | Reset |
| BL / LED | GPIO 4 | Backlight (example) |
| MISO / SDO | GPIO 19 | SPI MISO (optional) |

## SPI Bus Sharing Notes

- The TFT and SD card share MOSI/MISO/SCK on the SPI bus.
- The TFT and SD card must use different chip select (CS) pins.

## TFT_eSPI Configuration Reference

A reference `User_Setup.h` is included in this repository:
- `ArduinoIDE/User_Setup.h`

Typical pin macros in TFT_eSPI:

```cpp
#define TFT_CS   15
#define TFT_DC    2
#define TFT_RST   4
#define TFT_BL    4
```

Adjust these values to match your wiring and display module.

## Checklist

- All grounds connected (shared GND).
- Use 3.3V for TFT and SD card power (unless your specific module explicitly supports 5V).
- MOSI/MISO/SCK wired consistently across TFT and SD card.
- TFT CS and SD CS are different pins.
- TFT_eSPI configured for the correct driver (ILI9341/ST7789/etc.) and pins.
- SD card formatted as FAT32.

