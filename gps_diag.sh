#!/usr/bin/env bash
# gps_diag.sh — Non-interactive GPS diagnostic capture
# Connects to ESP32, reads serial for N seconds, prints structured report.
# Usage: ./gps_diag.sh [PORT] [SECONDS]
# If PORT is omitted, auto-detects. If SECONDS is omitted, reads for 15s.
set -euo pipefail

DURATION="${2:-15}"
PORT="${1:-}"

# ── Port detection ────────────────────────────────────────────────────────────
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

if [[ -z "$PORT" ]]; then
    if [[ ${#candidates[@]} -eq 0 ]]; then
        echo "ERROR: No serial ports found." >&2; exit 1
    elif [[ ${#candidates[@]} -eq 1 ]]; then
        PORT="${candidates[0]}"
    else
        echo "Available ports:"
        for i in "${!candidates[@]}"; do echo "  $((i+1))) ${candidates[$i]}"; done
        read -r -p "Select (1-${#candidates[@]}): " sel
        PORT="${candidates[$((sel-1))]}"
    fi
fi

if [[ ! -e "$PORT" ]]; then echo "ERROR: $PORT not found." >&2; exit 1; fi

if ! python3 -c "import serial" >/dev/null 2>&1; then
    echo "ERROR: python3-serial not installed. Run: sudo apt install python3-serial" >&2
    exit 1
fi

echo "==> GPS Diagnostic — port: $PORT | duration: ${DURATION}s"
echo "    (Captures raw GPS echo + sends 's' status command at end)"
echo ""

python3 - "$PORT" "$DURATION" <<'PYEOF'
import sys, serial, time, re

port     = sys.argv[1]
duration = float(sys.argv[2])

ANSI_GREEN  = "\033[32m"
ANSI_CYAN   = "\033[36m"
ANSI_YELLOW = "\033[33m"
ANSI_WHITE  = "\033[37m"
ANSI_GREY   = "\033[90m"
ANSI_RED    = "\033[31m"
ANSI_RESET  = "\033[0m"

def sentence_color(line):
    if   "GGA" in line[:8]: return ANSI_GREEN
    elif "RMC" in line[:8]: return ANSI_CYAN
    elif "GSV" in line[:8]: return ANSI_YELLOW
    elif "GSA" in line[:8]: return ANSI_WHITE
    elif line.startswith("["):  return ANSI_GREY
    else:                   return ANSI_GREY

try:
    ser = serial.Serial(port, 115200, timeout=0.1)
    ser.dtr = False
    ser.rts = False
except serial.SerialException as e:
    print(f"ERROR: Cannot open {port}: {e}", file=sys.stderr)
    sys.exit(1)

time.sleep(0.5)  # let ESP32 settle

nmea_lines   = []
status_lines = []
raw_buf      = b""
deadline     = time.monotonic() + duration

print(f"{'─'*60}")
print("RAW OUTPUT:")
print(f"{'─'*60}")

while time.monotonic() < deadline:
    chunk = ser.read(256)
    if chunk:
        raw_buf += chunk
        while b"\n" in raw_buf:
            line_b, raw_buf = raw_buf.split(b"\n", 1)
            try:
                line = line_b.decode("ascii", errors="replace").strip()
            except Exception:
                continue
            if not line:
                continue
            col = sentence_color(line)
            print(f"{col}{line}{ANSI_RESET}")
            if line.startswith("$GP") or line.startswith("$GN"):
                nmea_lines.append(line)
            elif line.startswith("["):
                status_lines.append(line)

# Send status command and wait 2s for reply
ser.write(b"s")
extra_deadline = time.monotonic() + 2.0
while time.monotonic() < extra_deadline:
    chunk = ser.read(256)
    if chunk:
        raw_buf += chunk
        while b"\n" in raw_buf:
            line_b, raw_buf = raw_buf.split(b"\n", 1)
            try:
                line = line_b.decode("ascii", errors="replace").strip()
            except Exception:
                continue
            if not line: continue
            col = sentence_color(line)
            print(f"{col}{line}{ANSI_RESET}")
            if line.startswith("["):
                status_lines.append(line)

ser.close()

# ── Summary ──────────────────────────────────────────────────────────────────
print()
print(f"{'─'*60}")
print("SUMMARY:")
print(f"{'─'*60}")

if not nmea_lines and not status_lines:
    print(f"{ANSI_RED}NO DATA received from ESP32.{ANSI_RESET}")
    print("  → Check USB connection and that firmware is flashed.")
    sys.exit(0)

sentence_types = {}
for l in nmea_lines:
    m = re.match(r'\$([A-Z]+)', l)
    if m:
        t = m.group(1)
        sentence_types[t] = sentence_types.get(t, 0) + 1

if sentence_types:
    print(f"{ANSI_GREEN}NMEA sentences received:{ANSI_RESET}")
    for t, cnt in sorted(sentence_types.items()):
        print(f"  {t:10s} × {cnt}")
else:
    print(f"{ANSI_RED}No NMEA sentences detected in output.{ANSI_RESET}")
    print("  → GPS module may not be connected or wrong pins/baud.")

for sl in status_lines:
    if sl.startswith("[STATUS]"):
        print(f"\n{ANSI_CYAN}GPS Status (from firmware):{ANSI_RESET}")
        print(f"  {sl}")
    elif sl.startswith("[SAT]"):
        print(f"  {sl}")
PYEOF
