# Pages

Navigate between pages by rotating the encoder (CW = next, CCW = previous).

---

## 1. Sky View (`SKY VIEW`)

Polar plot of all visible satellites.

- **Centre** = zenith (90° elevation)
- **Edge** = horizon (0° elevation)
- **North** = top
- Concentric rings at 30°, 60°, 90° elevation
- Cardinal labels N / E / S / W on spokes

### Satellite dots

| Colour       | Meaning             |
|--------------|---------------------|
| Green        | SNR ≥ 35 dBHz       |
| Yellow       | SNR ≥ 20 dBHz       |
| Red          | SNR > 0 dBHz        |
| Dark grey    | No signal           |
| White ring   | Satellite used in fix |

PRN number shown next to each dot.

### Controls

| Action | Result |
|--------|--------|
| CW / CCW | Switch page |

---

## 2. Signals (`SIGNALS`)

Horizontal SNR bar chart, one row per satellite, sorted by PRN.

- PRN label on the left (cyan if used in fix)
- Bar colour: green / yellow / red by SNR
- dBHz value on the right
- "NO FIX" overlay when fix quality = 0

### Controls

| Action | Result |
|--------|--------|
| CW / CCW | Switch page |

---

## 3. Fix Info (`FIX INFO`)

Full position readout.

| Field   | Format                  |
|---------|-------------------------|
| Fix     | NO FIX / GPS FIX / DGPS FIX badge |
| Lat     | ±DD.DDDDDD °            |
| Lon     | ±DDD.DDDDDD °           |
| Alt     | metres                  |
| Speed   | km/h                    |
| Course  | degrees                 |
| HDOP    | dimensionless           |
| Time    | HH:MM:SS UTC            |
| Date    | YYYY-MM-DD              |
| Sats    | used / in-view          |

### Controls

| Action | Result |
|--------|--------|
| CW / CCW | Switch page |
| Short click | Toggle detail (reserved) |

---

## 4. NMEA Raw (`NMEA RAW`)

Scrolling stream of raw NMEA sentences.

Colour coding:
- Green  → `$GPGGA` / `$GNGGA`
- Cyan   → `$GPRMC` / `$GNRMC`
- Yellow → `$GPGSV` / `$GNGSV`
- White  → `$GPGSA` / `$GNGSA`
- Grey   → other

### Controls

| Action | Result |
|--------|--------|
| CW / CCW on page manager | Switch page |
| Short click then CW | Scroll up (older lines) |
| Short click then CCW | Scroll down (newer lines) |

> Note: Encoder CW/CCW is intercepted by the page manager for page switching.
> Within the NMEA page, scroll is via the `onEncoder` callback fired for
> CLICK events. Current implementation routes CW/CCW directly to the page
> only when the page manager is not consuming them — use the click to
> "enter scroll mode" in a future revision.


## Pushbuttons (AHH-1.0)

- Pushbutton 1: GPIO26
- Pushbutton 2: GPIO27
