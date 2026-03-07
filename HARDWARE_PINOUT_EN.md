# Hardware Pinout (ESP32 + TFT + Encoder + GPS + SD)

This document summarizes the reference wiring for **ESP-SATDUMP**.

## GPS Module (UART2)

| GPS Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| TX | GPIO 2 | ESP RX ← GPS TX |
| RX | GPIO 15 | ESP TX → GPS RX |
| VCC | 3.3V / 5V | Power (check module) |
| GND | GND | Ground |

## SD Card Module (SPI - Shared Bus)

| SD Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| CS | GPIO 13 | Chip Select |
| MOSI | GPIO 23 | SPI MOSI |
| MISO | GPIO 19 | SPI MISO |
| SCK | GPIO 18 | SPI Clock |
| VCC | 3.3V | Power |
| GND | GND | Ground |

## Rotary Encoder (KY-040)

| Encoder Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| CLK | GPIO 33 | Clock signal |
| DT | GPIO 32 | Rotation direction |
| SW | GPIO 25 | Button/switch |
| + | 3.3V | Power |
| GND | GND | Ground |

## TFT Display (4.0" ST7796S SPI - Shared Bus)

| TFT Pin | ESP32 GPIO | Description |
| --- | --- | --- |
| VCC | 3.3V | Power |
| GND | GND | Ground |
| SCLK | GPIO 18 | SPI Clock |
| MOSI | GPIO 23 | SPI MOSI |
| MISO | GPIO 19 | SPI MISO |
| CS | GPIO 5 | Chip Select |
| DC | GPIO 16 | Data/Command |
| RST | GPIO 17 | Reset |
| BL | GPIO 4 | Backlight Control |

## Button: Orientation Toggle

| Component | ESP32 GPIO | Description |
| --- | --- | --- |
| Button | GPIO 26 | Toggle Portrait/Landscape |

## Checklist

- All grounds connected (shared GND).
- Use 3.3V for TFT, SD, and Encoder power.
- **SPI Sharing:** TFT and SD card share MOSI, MISO, and SCLK, but use different CS pins (GPIO 5 and GPIO 13).
- **TFT_eSPI** must be configured using the `User_Setup.h` provided in the firmware folder.
- Ensure GPS TX is connected to ESP RX (GPIO 2) and GPS RX to ESP TX (GPIO 15).
