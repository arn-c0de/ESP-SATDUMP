# GPS Visual CLI Tool

Live GPS/NMEA terminal dashboard with Rich UI.

## Files

- `tools/run_gps_visual_test.sh`: launcher (venv + dependency checks)
- `tools/gps_visual_test.py`: live parser + Rich terminal UI

## Start

```bash
# Auto-select serial port (interactive if multiple ports are found)
./tools/run_gps_visual_test.sh

# Use explicit port
./tools/run_gps_visual_test.sh /dev/ttyUSB0
```

## Options

- `--baud` (default: `9600`)
- `--raw-lines` (default: `6`)
- `--refresh` (default: `0.25`)

Example:

```bash
./tools/run_gps_visual_test.sh /dev/ttyUSB0 --baud 9600 --raw-lines 12 --refresh 0.2
```

## UI Features

- Rich CLI dashboard with fixed grid layout
- Responsive layout based on terminal width
- Live fields:
  - Fix status (`RMC` / `GGA`)
  - Satellites `used` / `in view`
  - DOP values (`PDOP`, `HDOP`, `VDOP`)
  - Position / speed / course / UTC time
  - Sentence and checksum statistics
- Satellite table:
  - `PRN`, elevation (deg), azimuth (deg), SNR
  - Per-satellite signal bars
- SNR graph panel (top satellites)
- Automatic reconnect on serial read/open errors

## Keyboard Controls

- `Space`: pause/resume UI redraw (useful for selecting/copying values)
- `Ctrl+C`: quit

## Notes

- If `RMC status = V` and `GGA quality = 0`, there is no valid GPS fix yet.
- The tool still runs and continues updating as soon as valid data arrives.
- Dependencies `pyserial` and `rich` are installed automatically by the launcher when missing.
