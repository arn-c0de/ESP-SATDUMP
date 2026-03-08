#!/usr/bin/env bash
# flash_test.sh — Compile and upload SD_Test firmware
set -euo pipefail

SKETCH_DIR="$(cd "$(dirname "$0")/firmware/SD_Test" && pwd)"
BOARD="esp32:esp32:esp32"
BUILD_DIR="/tmp/sd-test-build"
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
        echo "ERROR: No serial port found." >&2; exit 1
    elif [[ ${#candidates[@]} -eq 1 ]]; then
        PORT="${candidates[0]}"
    else
        echo "Multiple serial ports detected, choosing the first one: ${candidates[0]}"
        PORT="${candidates[0]}"
    fi
fi

# ── Compile ──────────────────────────────────────────────────────────────────
echo "==> Compiling $SKETCH_DIR ..."
arduino-cli compile \
    --fqbn "$BOARD" \
    --build-path "$BUILD_DIR" \
    "$SKETCH_DIR"

# ── Upload ───────────────────────────────────────────────────────────────────
echo "==> Uploading to $PORT at $UPLOAD_BAUD baud..."
arduino-cli upload \
    --fqbn "$BOARD" \
    --port "$PORT" \
    --input-dir "$BUILD_DIR" \
    --upload-property "upload.speed=$UPLOAD_BAUD" \
    "$SKETCH_DIR"

echo "==> Done!"

# ── Start Serial Monitor ─────────────────────────────────────────────────────
MONITOR_SCRIPT="$(dirname "$0")/monitorv2.sh"
if [[ -x "$MONITOR_SCRIPT" ]]; then
    echo "==> Starting Serial Monitor on $PORT..."
    exec "$MONITOR_SCRIPT" "$PORT" 115200
fi
