# Multi-OS Launcher

The Multi-OS Launcher lets you flash a new firmware (`.bin`) from a FAT32 SD card directly on the device. It activates at boot and uses the existing TFT display and rotary encoder for navigation.

---

## Activation

| Method | How |
|--------|-----|
| **Hardware button** | Hold **GPIO26** (BTN_ROT_PIN) LOW while the splash screen is showing |
| **Long-press encoder** | Hold the encoder button (GPIO25) ≥ 600 ms during normal operation |
| **Serial command** | Send `l` over USB-Serial at 115200 baud |

The hardware button and serial command are the primary ways to enter the launcher. The long-press method is available at any time while the app is running — it sets an NVS flag and reboots, after which the menu appears automatically.

---

## Boot Flow

```
Power on
  └─ Splash screen (2.5 s)  ←  "Hold GPIO26 for OS Launcher" shown at bottom
       └─ encoderInit()
            └─ launcherCheckAndRun()
                 ├─ GPIO26 held LOW?  ──┐
                 └─ NVS flag set?    ───┴──► Launcher menu appears on TFT
                                            │
                                 ┌──────────▼──────────────┐
                                 │  > Boot ESP-SATDUMP     │
                                 │    Install from SD      │
                                 └─────────────────────────┘
                                 Rotate encoder = navigate
                                 Click encoder  = confirm
```

Choosing **"Boot ESP-SATDUMP"** returns immediately — the normal app starts.
Choosing **"Install from SD"** begins the install flow.

---

## Install Flow

```
Mount SD card
  └─ Scan /firmware/ for *.bin files
       └─ Firmware selection menu
            └─ Flash selected file via OTA (Update.h)
                 ├─ Progress bar shown on TFT
                 └─ Reboot into new firmware on success
```

If SD mount fails or no `.bin` files are found, an error is shown for 3 seconds and the normal app starts.

---

## SD Card Setup

The SD card uses a **separate SPI bus (VSPI)** with dedicated GPIO pins, independent from the TFT display (which uses HSPI on GPIO 18/19/23).

### Wiring

| SD Module Pin | ESP32 GPIO |
|---------------|-----------|
| MOSI          | **13**    |
| MISO          | **21**    |
| SCK           | **22**    |
| CS            | **27**    |
| VCC           | 3.3 V     |
| GND           | GND       |

### SD Card Preparation

1. Format as **FAT32**.
2. Create a folder called **`/firmware`** in the root.
3. Copy one or more ESP32 firmware `.bin` files into `/firmware/`.

```
SD card (FAT32)
└── firmware/
    ├── esp-satdump-v1.0.bin
    ├── my-other-app.bin
    └── ...
```

The filename shown in the selection menu comes directly from the file name, so use descriptive names.

---

## Partition Scheme

OTA flashing requires two app partitions. The project uses the **`min_spiffs`** scheme (built into the ESP32 Arduino core), which allocates **~1.875 MB per OTA slot** on a 4 MB flash chip.

`flash.sh` passes the required build properties automatically:

```bash
--build-property "build.partitions=min_spiffs"
--build-property "upload.maximum_size=1966080"
```

> **First flash after adding the launcher:** the partition table changes, so a full erase may be needed if you see boot-loop issues after flashing.
> Use: `esptool.py --port /dev/ttyUSBx erase_flash` before running `./flash.sh`.

---

## Safety

- OTA always writes to the **inactive** partition — the running firmware is never overwritten mid-flash.
- If the flash process fails, the device reboots into the existing firmware unchanged.
- The NVS boot flag is cleared as soon as the launcher menu opens, so a crash inside the launcher will not cause an infinite reboot loop.

---

## Source Files

| File | Description |
|------|-------------|
| `firmware/ESP_SATDUMP/launcher.h` | Public API (`launcherCheckAndRun`, `launcherRebootToLauncher`) |
| `firmware/ESP_SATDUMP/launcher.cpp` | Full implementation: NVS, TFT menu, SD listing, OTA flash |
| `firmware/ESP_SATDUMP/page_manager.cpp` | Long-press encoder event triggers `launcherRebootToLauncher()` |
| `firmware/ESP_SATDUMP/display_utils.cpp` | Splash screen hint text |
| `flash.sh` | OTA partition build flags |
