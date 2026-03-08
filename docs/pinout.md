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
| TX      | GPIO 2    | GPS TX → ESP RX  |
| RX      | GPIO 15   | GPS RX ← ESP TX  |
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

## SD Card — AHH-1.0 Handheld (Dedicated Bus)

| Component | Signal | Old GPIO | **Final GPIO (AHH-1.0)** | Reason |
| :--- | :--- | :--- | :--- | :--- |
| **SD Card** | **MISO** | 19 | **12** | Moved to 12. |
| **SD Card** | **MOSI** | 23 | **14** | Moved to 14. |
| **SD Card** | **SCK**  | 18 | **22** | Dedicated SPI bus (HSPI). |
| **SD Card** | **CS**   | 13 | **13** | (Unchanged). |

## SPI Bus Configuration (AHH-1.0)

For the **AHH-1.0 Handheld**, the TFT and SD card use **separate SPI buses**.
- **TFT Display:** Remains on the default VSPI bus (GPIO 18, 19, 23).
- **SD Card:** Uses the HSPI peripheral (GPIO 12, 14, 22).
This separation prevents common interference issues where the SD card module fails to release the MISO line correctly.


## Pushbuttons (AHH-1.0)

- Pushbutton 1: GPIO26
- Pushbutton 2: GPIO27
