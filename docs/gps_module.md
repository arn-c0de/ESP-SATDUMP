# GPS Module — u-blox NEO-6M

## Wiring

| GPS Pin | ESP32 GPIO |
|---------|-----------|
| TX      | GPIO 26   |
| RX      | GPIO 27   |
| VCC     | 3.3V      |
| GND     | GND       |

## Serial Settings

- Baud rate: **9600** (NEO-6M default)
- Format: `SERIAL_8N1`
- ESP32 peripheral: `Serial2` (`UART2`)

## NMEA Sentences Parsed

| Sentence | Data Extracted                                     |
|----------|----------------------------------------------------|
| $GPGGA   | Lat, lon, fix quality, sats used, HDOP, altitude   |
| $GPRMC   | Speed (knots→km/h), course, date/time (UTC)        |
| $GPGSV   | Per-satellite: PRN, elevation, azimuth, SNR        |
| $GPGSA   | PRNs used in current fix (sets `SatInfo::used`)    |

Multi-constellation variants (`$GNGGA`, `$GNRMC`, `$GNGSV`, `$GNGSA`) are
handled with the same parsers.

## Default Output Sentences

The NEO-6M outputs the following by default at 1 Hz:
`$GPGGA`, `$GPGLL`, `$GPRMC`, `$GPVTG`, `$GPGSV`, `$GPGSA`

No module reconfiguration is needed for basic operation.

## First Fix

- Cold start: up to 30 s outdoors with clear sky view
- Warm start (recent almanac): ~5 s
- Hot start (recent fix + almanac): <1 s

## Baud Rate Change (optional)

To change to 38400 via UBX command (send once, then reconnect at new baud):
```
$PUBX,41,1,0007,0003,38400,0*20\r\n
```
Update `GPS_BAUD` in `config.h` accordingly.
