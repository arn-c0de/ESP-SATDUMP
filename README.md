# ESP-SATDUMP

GPS satellite tracker firmware for ESP32 with a 4" TFT display.

## Hardware

| Component | Part |
|-----------|------|
| MCU       | ESP32 |
| Display   | LAFVIN 4.0" ST7796S TFT 480×320 (SPI) |
| GPS       | u-blox NEO-6M (UART2) |
| Encoder   | KY-040 rotary encoder |

Full wiring: [`docs/pinout.md`](docs/pinout.md)

## Pages

| # | Name | Description |
|---|------|-------------|
| 1 | Sky View  | Polar satellite plot (azimuth / elevation) |
| 2 | Signals   | SNR bar chart per satellite |
| 3 | Fix Info  | Lat / lon / alt / speed / time |
| 4 | NMEA Raw  | Scrolling raw NMEA sentence stream |

Navigate with the rotary encoder: **CW = next page**, **CCW = previous page**.

## Flashing

```bash
# Auto-detect port and flash
./flash.sh

# Specify port explicitly
./flash.sh /dev/ttyUSB0
```

Requires [arduino-cli](https://arduino.github.io/arduino-cli/) and the
`esp32:esp32` board package.

### TFT_eSPI setup

Copy `firmware/ESP_SATDUMP/User_Setup.h` into your TFT_eSPI library folder
(replacing the existing one), or configure `User_Setup_Select.h` to point
at this file. The file sets the ST7796S driver and the correct GPIO pins.

### Libraries needed

```bash
arduino-cli lib install "TFT_eSPI"
```

## Project Structure

```
firmware/ESP_SATDUMP/   Arduino sketch + all source files
flash.sh                Compile & upload script
monitorv2.sh            Serial monitor
docs/                   Architecture, pinout, GPS, page descriptions
```

## Serial Output

After boot the sketch prints `[ESP-SATDUMP] boot` at 115200 baud and then
forwards GPS debug info. Use `monitorv2.sh` or any serial terminal at 115200.
