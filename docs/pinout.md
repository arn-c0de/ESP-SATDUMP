# Full Hardware Wiring

## TFT Display — LAFVIN 4.0" ST7796S (SPI)

| TFT Pin | ESP32 GPIO | Note            |
|---------|-----------|-----------------|
| MISO    | GPIO 19   | SPI MISO        |
| MOSI    | GPIO 23   | SPI MOSI        |
| SCK     | GPIO 18   | SPI clock       |
| CS      | GPIO 5    | Chip select     |
| DC/RS   | GPIO 16   | Data/Command    |
| RESET   | GPIO 17   | Reset           |
| LED/BL  | GPIO 4    | Backlight (HIGH=on) |
| VCC     | 3.3V      |                 |
| GND     | GND       |                 |

SPI frequency: 27 MHz

## GPS — u-blox NEO-6M (UART2)

| GPS Pin | ESP32 GPIO | Note             |
|---------|-----------|------------------|
| TX      | GPIO 26   | GPS TX → ESP RX  |
| RX      | GPIO 27   | GPS RX ← ESP TX  |
| VCC     | 3.3V      |                  |
| GND     | GND       |                  |

Baud: 9600, SERIAL_8N1

## Rotary Encoder — KY-040

| Encoder Pin | ESP32 GPIO | Note            |
|-------------|-----------|-----------------|
| CLK         | GPIO 33   | Clock (ISR)     |
| DT          | GPIO 32   | Direction (ISR) |
| SW          | GPIO 25   | Button (active-low) |
| +           | 3.3V      |                 |
| GND         | GND       |                 |

## SPI Bus Sharing

TFT and SD card (if fitted) share MOSI/MISO/SCK.
Each device must use a unique CS pin.
