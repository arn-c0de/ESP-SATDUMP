#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")" && pwd)"
REPO_ROOT="$(cd -- "${SCRIPT_DIR}/.." && pwd)"
VENV_DIR="${REPO_ROOT}/.venv"
PY_SCRIPT="${SCRIPT_DIR}/gps_visual_test.py"

usage() {
  cat <<'EOF'
Usage:
  tools/run_gps_visual_test.sh <PORT> [extra options]

Examples:
  tools/run_gps_visual_test.sh /dev/ttyUSB0
  tools/run_gps_visual_test.sh /dev/ttyACM0 --baud 9600 --raw-lines 10

Behavior:
  - Uses existing .venv in the project root if present
  - Creates .venv automatically if missing
  - Installs pyserial + rich automatically if missing
EOF
}

detect_port() {
  local candidates=()
  local d
  local idx
  local choice
  for d in /dev/ttyUSB* /dev/ttyACM*; do
    if [[ -e "${d}" ]]; then
      candidates+=("${d}")
    fi
  done

  if [[ "${#candidates[@]}" -eq 0 ]]; then
    echo "Error: no serial GPS port detected." >&2
    echo "Checked: /dev/ttyUSB* and /dev/ttyACM*" >&2
    echo "Tip: reconnect adapter and run: ls /dev/ttyUSB* /dev/ttyACM* 2>/dev/null" >&2
    return 1
  fi

  if [[ "${#candidates[@]}" -gt 1 ]]; then
    if [[ ! -t 0 ]]; then
      echo "Error: multiple serial ports detected. Please choose one explicitly." >&2
      printf 'Detected ports:\n' >&2
      printf '  %s\n' "${candidates[@]}" >&2
      echo "Example: tools/run_gps_visual_test.sh /dev/ttyUSB0" >&2
      return 1
    fi

    echo "Multiple serial ports detected. Please choose one:"
    for idx in "${!candidates[@]}"; do
      printf '  [%d] %s\n' "$((idx + 1))" "${candidates[idx]}"
    done

    while true; do
      printf 'Select port [1-%d] (q to quit): ' "${#candidates[@]}"
      read -r choice

      if [[ "${choice}" == "q" || "${choice}" == "Q" ]]; then
        echo "Aborted."
        return 1
      fi

      if [[ "${choice}" =~ ^[0-9]+$ ]] && (( choice >= 1 && choice <= ${#candidates[@]} )); then
        printf '%s\n' "${candidates[choice-1]}"
        return 0
      fi

      echo "Invalid selection: ${choice}"
    done
  fi

  printf '%s\n' "${candidates[0]}"
}

if [[ "${1:-}" == "-h" || "${1:-}" == "--help" ]]; then
  usage
  exit 0
fi

if [[ ! -f "${PY_SCRIPT}" ]]; then
  echo "Error: Python tool not found: ${PY_SCRIPT}" >&2
  exit 1
fi

if [[ -d "${VENV_DIR}" ]]; then
  echo "Using existing virtual environment: ${VENV_DIR}"
else
  echo "Creating virtual environment: ${VENV_DIR}"
  python3 -m venv "${VENV_DIR}"
fi

# shellcheck disable=SC1091
source "${VENV_DIR}/bin/activate"

if ! python -c "import serial, rich" >/dev/null 2>&1; then
  echo "Installing missing dependency: pyserial rich"
  python -m pip install --upgrade pip >/dev/null
  python -m pip install pyserial rich
fi

if [[ $# -ge 1 ]]; then
  PORT="$1"
  shift
else
  PORT="$(detect_port)"
  echo "Auto-detected serial port: ${PORT}"
fi

exec python "${PY_SCRIPT}" --port "${PORT}" "$@"
