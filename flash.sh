#!/usr/bin/env bash
# flash.sh — Compile and upload ESP-SATDUMP firmware via arduino-cli
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "$0")/firmware/ESP_SATDUMP" && pwd)"
BOARD="esp32:esp32:esp32"
BUILD_DIR="/tmp/esp-satdump-build"
UPLOAD_BAUD="921600"

# ── Detect serial port ───────────────────────────────────────────────────────
PORT="${1:-}"
if [[ -z "$PORT" ]]; then
    declare -a candidates=()
    declare -A seen=()
    for p in /dev/ttyUSB* /dev/ttyACM* /dev/serial/by-id/*; do
        [[ -e "$p" ]] || continue
        realp=$(readlink -f "$p")
        if [[ -z "${seen[$realp]:-}" ]]; then
            seen[$realp]=1
            candidates+=("$realp")
        fi
    done

    if [[ ${#candidates[@]} -eq 0 ]]; then
        echo "ERROR: No serial port found. Plug in the ESP32 or pass port as argument." >&2
        echo "Usage: $0 [/dev/ttyUSBx]" >&2
        exit 1
    elif [[ ${#candidates[@]} -eq 1 ]]; then
        PORT="${candidates[0]}"
    else
        echo "Multiple serial ports detected:"
        for i in "${!candidates[@]}"; do
            echo "  $((i+1))) ${candidates[$i]}"
        done
        read -r -p "Select port number (1-${#candidates[@]}) or type a path: " sel
        if [[ "$sel" =~ ^[0-9]+$ ]]; then
            idx=$((sel - 1))
            if (( idx >= 0 && idx < ${#candidates[@]} )); then
                PORT="${candidates[$idx]}"
            else
                echo "Invalid selection." >&2; exit 2
            fi
        else
            PORT="$sel"
        fi
    fi
fi

if [[ ! -e "$PORT" ]]; then
    echo "ERROR: Port \"$PORT\" does not exist." >&2; exit 3
fi
echo "Using port: $PORT"

# ── Check arduino-cli ────────────────────────────────────────────────────────
if ! command -v arduino-cli &>/dev/null; then
    echo "ERROR: arduino-cli not found. Install from https://arduino.github.io/arduino-cli/" >&2
    exit 2
fi

# ── Install TFT_eSPI User_Setup if needed ────────────────────────────────────
# TFT_eSPI reads User_Setup.h from the sketch directory when using
# the ARDUINO_USER_SETUP define; or you can copy it to the library.
# We copy it to the library if the library exists.
TFT_LIB_DIR=$(arduino-cli lib list 2>/dev/null | awk '/TFT_eSPI/ {print $1}' | head -1 || true)
if [[ -n "$TFT_LIB_DIR" ]]; then
    ARDUINO_LIBS=$(arduino-cli config dump 2>/dev/null | grep 'user:' | awk '{print $2}' || true)
    if [[ -n "$ARDUINO_LIBS" ]]; then
        DEST="$ARDUINO_LIBS/libraries/TFT_eSPI/User_Setup.h"
        if [[ -f "$DEST" ]]; then
            echo "NOTE: $DEST already exists — not overwriting."
            echo "      To use this project's pinout, copy firmware/ESP_SATDUMP/User_Setup.h there."
        fi
    fi
fi

# ── Compile ──────────────────────────────────────────────────────────────────
echo ""
echo "==> Compiling $SKETCH_DIR ..."
arduino-cli compile \
    --fqbn "$BOARD" \
    --build-path "$BUILD_DIR" \
    --build-property "build.partitions=min_spiffs" \
    --build-property "upload.maximum_size=1966080" \
    "$SKETCH_DIR"

# ── Upload ───────────────────────────────────────────────────────────────────
echo ""
echo "==> Uploading to $PORT at $UPLOAD_BAUD baud..."
arduino-cli upload \
    --fqbn "$BOARD" \
    --port "$PORT" \
    --input-dir "$BUILD_DIR" \
    --upload-property "upload.speed=$UPLOAD_BAUD" \
    "$SKETCH_DIR"

echo ""
echo "==> Done! Firmware flashed successfully."
echo ""

# ── Optional: open serial monitor ────────────────────────────────────────────
MONITOR_SCRIPT="$(dirname "$0")/monitorv2.sh"
if [[ -x "$MONITOR_SCRIPT" ]]; then
    read -r -t 5 -p "Open serial monitor? [Y/n] " ans || ans="y"
    ans="${ans:-y}"
    if [[ "$ans" =~ ^[Yy]$ ]]; then
        exec "$MONITOR_SCRIPT" "$PORT" 115200
    fi
fi
