#!/usr/bin/env python3
"""Simple visual GPS tester for USB-TTL connected u-blox modules.

Reads NMEA lines from a serial port and renders a live terminal dashboard.
"""

from __future__ import annotations

import argparse
import select
import shutil
import signal
import sys
import termios
import time
import tty
from dataclasses import dataclass, field
from datetime import datetime
from typing import Optional

try:
    import serial
    from serial import SerialException
except ImportError:
    print("Missing dependency: pyserial")
    print("Install with: pip install pyserial")
    sys.exit(1)

try:
    from rich.columns import Columns
    from rich.console import Group
    from rich.live import Live
    from rich.panel import Panel
    from rich.table import Table
except ImportError:
    print("Missing dependency: rich")
    print("Install with: pip install rich")
    sys.exit(1)


@dataclass
class GpsState:
    last_sentence: str = "-"
    last_update_local: str = "-"
    connection_status: str = "Connecting..."
    total_sentences: int = 0
    valid_checksums: int = 0
    invalid_checksums: int = 0
    fix_quality: str = "-"
    satellites: str = "-"
    satellites_in_view: str = "-"
    hdop: str = "-"
    pdop: str = "-"
    vdop: str = "-"
    latitude: str = "-"
    longitude: str = "-"
    altitude_m: str = "-"
    speed_knots: str = "-"
    speed_kmh: str = "-"
    course_deg: str = "-"
    utc_time: str = "-"
    utc_date: str = "-"
    status: str = "-"
    sentence_counts: dict[str, int] = field(default_factory=dict)
    satellite_details: dict[str, tuple[str, str, str]] = field(default_factory=dict)


def parse_args() -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="Visual GPS tester for NMEA over USB serial"
    )
    parser.add_argument(
        "--port",
        required=True,
        help="Serial port, e.g. /dev/ttyUSB0, /dev/ttyACM0, COM3",
    )
    parser.add_argument("--baud", type=int, default=9600, help="Baud rate (default: 9600)")
    parser.add_argument(
        "--raw-lines",
        type=int,
        default=6,
        help="How many recent raw NMEA lines to show (default: 6)",
    )
    parser.add_argument(
        "--refresh",
        type=float,
        default=0.25,
        help="Screen refresh interval in seconds (default: 0.25)",
    )
    return parser.parse_args()


def parse_lat_lon(raw: str, hemi: str) -> Optional[float]:
    if not raw or raw == "0":
        return None

    # Lat format: ddmm.mmmm, Lon format: dddmm.mmmm
    dot = raw.find(".")
    if dot < 0:
        return None

    deg_len = 2 if len(raw[:dot]) in (4, 5) and hemi in ("N", "S") else 3
    if hemi in ("N", "S"):
        deg_len = 2
    elif hemi in ("E", "W"):
        deg_len = 3

    try:
        deg = float(raw[:deg_len])
        minutes = float(raw[deg_len:])
        value = deg + (minutes / 60.0)
        if hemi in ("S", "W"):
            value = -value
        return value
    except ValueError:
        return None


def checksum_valid(line: str) -> bool:
    if "*" not in line or not line.startswith("$"):
        return False
    data, checksum = line[1:].split("*", 1)
    calc = 0
    for ch in data:
        calc ^= ord(ch)
    try:
        return calc == int(checksum[:2], 16)
    except ValueError:
        return False


def parse_gga(fields: list[str], state: GpsState) -> None:
    # $GPGGA,hhmmss.ss,lat,NS,lon,EW,fix,sats,hdop,alt,M,...
    if len(fields) < 10:
        return

    lat = parse_lat_lon(fields[2], fields[3])
    lon = parse_lat_lon(fields[4], fields[5])

    state.fix_quality = fields[6] or "-"
    state.satellites = fields[7] or "-"
    state.hdop = fields[8] or "-"
    state.altitude_m = fields[9] or "-"

    if lat is not None:
        state.latitude = f"{lat:.6f}"
    if lon is not None:
        state.longitude = f"{lon:.6f}"

    if fields[1]:
        state.utc_time = fields[1]


def parse_rmc(fields: list[str], state: GpsState) -> None:
    # $GPRMC,hhmmss.ss,status,lat,NS,lon,EW,speed,course,date,...
    if len(fields) < 10:
        return

    lat = parse_lat_lon(fields[3], fields[4])
    lon = parse_lat_lon(fields[5], fields[6])

    state.status = fields[2] or "-"  # A=valid, V=void
    state.speed_knots = fields[7] or "-"
    state.course_deg = fields[8] or "-"
    state.utc_date = fields[9] or "-"

    if fields[1]:
        state.utc_time = fields[1]

    if lat is not None:
        state.latitude = f"{lat:.6f}"
    if lon is not None:
        state.longitude = f"{lon:.6f}"

    try:
        speed = float(fields[7])
        state.speed_kmh = f"{speed * 1.852:.2f}"
    except (ValueError, TypeError):
        pass


def parse_gsa(fields: list[str], state: GpsState) -> None:
    # $GPGSA,mode,fix,sat...,PDOP,HDOP,VDOP
    if len(fields) < 18:
        return

    state.pdop = fields[-3] or state.pdop
    state.hdop = fields[-2] or state.hdop
    state.vdop = fields[-1] or state.vdop


def parse_gsv(fields: list[str], state: GpsState) -> None:
    # $GPGSV,total_msgs,msg_num,sats_in_view,prn,elev,az,snr,...
    if len(fields) < 4:
        return

    state.satellites_in_view = fields[3] or state.satellites_in_view

    # Start of a new GSV sequence: clear stale per-satellite data.
    if fields[2] == "1":
        state.satellite_details.clear()

    for i in range(4, len(fields) - 3, 4):
        prn = fields[i]
        elev = fields[i + 1] or "-"
        az = fields[i + 2] or "-"
        snr = fields[i + 3] or "-"
        if prn:
            state.satellite_details[prn] = (elev, az, snr)


def fmt_fix_quality(value: str) -> str:
    mapping = {
        "0": "0 (Invalid)",
        "1": "1 (GPS fix)",
        "2": "2 (DGPS fix)",
        "4": "4 (RTK Fixed)",
        "5": "5 (RTK Float)",
    }
    return mapping.get(value, value)


def fmt_status(value: str) -> str:
    return {"A": "A (Valid)", "V": "V (No fix)"}.get(value, value)


def utc_now_local() -> str:
    return datetime.now().astimezone().strftime("%Y-%m-%d %H:%M:%S %Z")


def status_style(connection_status: str) -> str:
    return "bold green" if connection_status == "Connected" else "bold red"


def snr_bar(snr_value: str, width: int = 18) -> str:
    try:
        snr = float(snr_value)
    except (TypeError, ValueError):
        return "-" * width

    # Typical GNSS SNR range is roughly 0..60 dB-Hz.
    snr = max(0.0, min(60.0, snr))
    filled = int(round((snr / 60.0) * width))
    return "█" * filled + "░" * (width - filled)


def build_dashboard(
    state: GpsState,
    port: str,
    baud: int,
    raw_lines: list[str],
    term_width: int,
    paused: bool,
) -> Panel:
    mode_text = "[bold yellow]PAUSED[/bold yellow]" if paused else "[bold green]LIVE[/bold green]"
    header = (
        f"[bold cyan]GPS Visual Test (Rich CLI)[/bold cyan]  "
        f"[white]Port:[/white] {port}  [white]Baud:[/white] {baud}  "
        f"[white]Connection:[/white] [{status_style(state.connection_status)}]{state.connection_status}[/{status_style(state.connection_status)}]  "
        f"[white]Mode:[/white] {mode_text}"
    )

    fix_table = Table.grid(padding=(0, 2))
    fix_table.add_column(style="bold")
    fix_table.add_column()
    fix_table.add_row("RMC status", fmt_status(state.status))
    fix_table.add_row("GGA quality", fmt_fix_quality(state.fix_quality))
    fix_table.add_row("Satellites (used)", state.satellites)
    fix_table.add_row("Satellites (view)", state.satellites_in_view)
    fix_table.add_row("PDOP", state.pdop)
    fix_table.add_row("HDOP", state.hdop)
    fix_table.add_row("VDOP", state.vdop)

    pos_table = Table.grid(padding=(0, 2))
    pos_table.add_column(style="bold")
    pos_table.add_column()
    pos_table.add_row("Latitude", state.latitude)
    pos_table.add_row("Longitude", state.longitude)
    pos_table.add_row("Altitude (m)", state.altitude_m)
    pos_table.add_row("Speed (kn)", state.speed_knots)
    pos_table.add_row("Speed (km/h)", state.speed_kmh)
    pos_table.add_row("Course (deg)", state.course_deg)
    pos_table.add_row("UTC time", state.utc_time)
    pos_table.add_row("UTC date", state.utc_date)

    stats_table = Table.grid(padding=(0, 2))
    stats_table.add_column(style="bold")
    stats_table.add_column()
    stats_table.add_row("Last update", state.last_update_local)
    stats_table.add_row("Last sentence", state.last_sentence)
    stats_table.add_row("Total NMEA", str(state.total_sentences))
    stats_table.add_row("Checksum OK", str(state.valid_checksums))
    stats_table.add_row("Checksum invalid", str(state.invalid_checksums))
    stats_table.add_row("Course (deg)", state.course_deg)

    if state.sentence_counts:
        top = sorted(state.sentence_counts.items(), key=lambda x: x[1], reverse=True)
        sentence_counts = ", ".join(f"{k}:{v}" for k, v in top[:8])
    else:
        sentence_counts = "-"
    stats_table.add_row("Sentence counts", sentence_counts)

    bar_width = 10 if term_width < 110 else 14 if term_width < 150 else 18
    graph_width = 16 if term_width < 110 else 22 if term_width < 150 else 30

    sat_table = Table(show_header=True, header_style="bold")
    sat_table.add_column("PRN", justify="right")
    sat_table.add_column("Elev°", justify="right")
    sat_table.add_column("Az°", justify="right")
    sat_table.add_column("SNR", justify="right")
    sat_table.add_column("Signal")
    if state.satellite_details:
        def _sort_key(item: tuple[str, tuple[str, str, str]]) -> tuple[int, str]:
            prn = item[0]
            try:
                return (0, f"{int(prn):03d}")
            except ValueError:
                return (1, prn)

        for prn, (elev, az, snr) in sorted(state.satellite_details.items(), key=_sort_key)[:16]:
            sat_table.add_row(prn, elev, az, snr, snr_bar(snr, width=bar_width))
    else:
        sat_table.add_row("-", "-", "-", "-", "-" * bar_width)

    snr_graph = Table(show_header=True, header_style="bold")
    snr_graph.add_column("PRN", justify="right")
    snr_graph.add_column("SNR dB-Hz", justify="right")
    snr_graph.add_column("Graph")
    if state.satellite_details:
        snr_rows: list[tuple[str, float]] = []
        for prn, (_, _, snr) in state.satellite_details.items():
            try:
                snr_rows.append((prn, float(snr)))
            except (TypeError, ValueError):
                continue

        snr_rows.sort(key=lambda x: x[1], reverse=True)
        if snr_rows:
            for prn, snr in snr_rows[:12]:
                snr_graph.add_row(prn, f"{snr:.1f}", snr_bar(str(snr), width=graph_width))
        else:
            snr_graph.add_row("-", "-", "-" * graph_width)
    else:
        snr_graph.add_row("-", "-", "-" * graph_width)

    raw_content = "\n".join(raw_lines) if raw_lines else "(waiting for data...)"

    panel_fix = Panel(fix_table, title="Fix", border_style="blue")
    panel_stats = Panel(stats_table, title="Statistics", border_style="green")
    panel_pos = Panel(pos_table, title="Position", border_style="magenta")
    panel_sat = Panel(sat_table, title="Satellites", border_style="bright_blue")
    panel_snr = Panel(snr_graph, title="Signal Graph (SNR)", border_style="bright_green")
    panel_raw = Panel(raw_content, title="Recent raw NMEA", border_style="yellow")

    if term_width < 120:
        group = Group(
            header,
            panel_fix,
            panel_stats,
            panel_pos,
            panel_sat,
            panel_snr,
            panel_raw,
            "[dim]Space: Pause/Resume | Ctrl+C: Quit[/dim]",
        )
    elif term_width < 180:
        row1 = Columns([panel_fix, panel_stats], equal=True, expand=True)
        row2 = Columns([panel_pos, panel_sat], equal=True, expand=True)
        group = Group(
            header,
            row1,
            row2,
            panel_snr,
            panel_raw,
            "[dim]Space: Pause/Resume | Ctrl+C: Quit[/dim]",
        )
    else:
        row1 = Columns([panel_fix, panel_stats], equal=True, expand=True)
        row2 = Columns([panel_pos, panel_sat], equal=True, expand=True)
        row3 = Columns([panel_snr, panel_raw], equal=True, expand=True)
        group = Group(
            header,
            row1,
            row2,
            row3,
            "[dim]Space: Pause/Resume | Ctrl+C: Quit[/dim]",
        )
    return Panel(group, border_style="cyan", padding=(0, 1))


def setup_keyboard_cbreak() -> Optional[tuple[int, list[object]]]:
    if not sys.stdin.isatty():
        return None
    fd = sys.stdin.fileno()
    old_attrs = termios.tcgetattr(fd)
    tty.setcbreak(fd)
    return fd, old_attrs


def restore_keyboard(attrs: Optional[tuple[int, list[object]]]) -> None:
    if attrs is None:
        return
    fd, old_attrs = attrs
    termios.tcsetattr(fd, termios.TCSADRAIN, old_attrs)


def poll_space_toggle() -> bool:
    if not sys.stdin.isatty():
        return False

    toggles = 0
    while True:
        readable, _, _ = select.select([sys.stdin], [], [], 0)
        if not readable:
            break
        ch = sys.stdin.read(1)
        if ch == " ":
            toggles += 1
    return (toggles % 2) == 1


def main() -> int:
    args = parse_args()
    state = GpsState(last_update_local=utc_now_local())
    raw_buffer: list[str] = []
    running = True
    paused = False
    ser: Optional[serial.Serial] = None
    next_reconnect_at = 0.0
    kb_attrs = setup_keyboard_cbreak()

    def handle_sigint(_sig: int, _frame: object) -> None:
        nonlocal running
        running = False

    signal.signal(signal.SIGINT, handle_sigint)

    try:
        initial_width = shutil.get_terminal_size((120, 40)).columns
        with Live(
            build_dashboard(state, args.port, args.baud, raw_buffer, initial_width, paused),
            refresh_per_second=8,
            screen=True,
        ) as live:
            last_render = 0.0
            while running:
                now = time.monotonic()
                raw = b""
                force_refresh = False

                if poll_space_toggle():
                    paused = not paused
                    force_refresh = True

                if ser is None and now >= next_reconnect_at:
                    try:
                        ser = serial.Serial(args.port, args.baud, timeout=0.2)
                        state.connection_status = "Connected"
                    except SerialException as exc:
                        state.connection_status = f"Disconnected (retrying): {exc}"
                        next_reconnect_at = now + 1.0

                if ser is not None:
                    try:
                        raw = ser.readline()
                    except SerialException as exc:
                        state.connection_status = f"Disconnected (reconnecting): {exc}"
                        try:
                            ser.close()
                        except SerialException:
                            pass
                        ser = None
                        next_reconnect_at = now + 1.0

                if raw:
                    line = raw.decode("ascii", errors="replace").strip()
                    if line.startswith("$"):
                        state.last_sentence = line.split(",", 1)[0]
                        state.last_update_local = utc_now_local()
                        state.total_sentences += 1

                        raw_buffer.append(line)
                        if len(raw_buffer) > args.raw_lines:
                            raw_buffer.pop(0)

                        if checksum_valid(line):
                            state.valid_checksums += 1
                            payload = line[1:].split("*", 1)[0]
                            fields = payload.split(",")
                            sentence = fields[0][-3:]
                            state.sentence_counts[sentence] = state.sentence_counts.get(sentence, 0) + 1

                            if sentence == "GGA":
                                parse_gga(fields, state)
                            elif sentence == "RMC":
                                parse_rmc(fields, state)
                            elif sentence == "GSA":
                                parse_gsa(fields, state)
                            elif sentence == "GSV":
                                parse_gsv(fields, state)
                        else:
                            state.invalid_checksums += 1

                if (not paused and now - last_render >= args.refresh) or force_refresh:
                    term_width = shutil.get_terminal_size((120, 40)).columns
                    live.update(build_dashboard(state, args.port, args.baud, raw_buffer, term_width, paused))
                    last_render = now

    finally:
        restore_keyboard(kb_attrs)
        if ser is not None:
            ser.close()

    print("Exited.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
