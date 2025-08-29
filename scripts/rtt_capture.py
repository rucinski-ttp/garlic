#!/usr/bin/env python3
"""
Reliable RTT capture for nRF52-DK.

Tries multiple backends in order:
  1) J-Link RTT Logger (single-process, robust)
  2) J-Link GDB Server + JLinkRTTClient

Writes timestamped lines to an output file and echoes to stdout.

Usage examples:
  ./scripts/rtt_capture.py --outfile logs/rtt_$(date +%s).log
  ./scripts/rtt_capture.py --tool jlink --channel 0 --outfile logs/rtt.log
  GARLIC_RTT_TOOL=openocd ./scripts/rtt_capture.py --outfile logs/rtt.log

Environment overrides:
  GARLIC_RTT_TOOL      : auto|jlink-logger|jlink (default: auto)
  GARLIC_RTT_DEVICE    : J-Link device name (default: NRF52832_XXAA)
  GARLIC_RTT_SPEED     : SWD speed in kHz (default: 4000)
  GARLIC_RTT_CHANNEL   : RTT up channel to capture (default: 0)

Dependencies:
  - J-Link tools (JLinkRTTLogger or JLinkGDBServer + JLinkRTTClient), or
  - OpenOCD (with J-Link) + Python stdlib only
"""

from __future__ import annotations

import argparse
import os
import signal
import socket
import subprocess
import sys
import time
from pathlib import Path
from shutil import which


def now_ts() -> str:
    return time.strftime("%Y-%m-%d %H:%M:%S")


def info(msg: str) -> None:
    sys.stderr.write(f"[rtt] {msg}\n")


def parse_args():
    p = argparse.ArgumentParser(description="Capture SEGGER RTT output to file + stdout (J-Link only)")
    p.add_argument(
        "--tool",
        choices=["auto", "jlink-logger", "jlink"],
        default=os.getenv("GARLIC_RTT_TOOL", "auto"),
        help="Backend to use (default: auto)",
    )
    p.add_argument(
        "--channel",
        type=int,
        default=int(os.getenv("GARLIC_RTT_CHANNEL", "0")),
        help="RTT up-channel index to read (default 0)",
    )
    p.add_argument(
        "--device",
        default=os.getenv("GARLIC_RTT_DEVICE", "NRF52832_XXAA"),
        help="Target device for J-Link/OpenOCD (default NRF52832_XXAA)",
    )
    p.add_argument(
        "--speed",
        type=int,
        default=int(os.getenv("GARLIC_RTT_SPEED", "4000")),
        help="SWD speed in kHz (default 4000)",
    )
    p.add_argument(
        "--outfile",
        default=None,
        help="Output log file path (default logs/rtt_<epoch>.log)",
    )
    p.add_argument("--gdb-rtt-port", type=int, default=19021, help="J-Link GDB Server RTT port")
    p.add_argument(
        "--no-timestamps",
        action="store_true",
        help="Do not prefix lines with timestamps",
    )
    return p.parse_args()


def write_line(fp, text: str, add_ts: bool) -> None:
    rec = f"{now_ts()} | {text}\n" if add_ts else f"{text}\n"
    sys.stdout.write(rec)
    fp.write(rec)
    fp.flush()


def run_jlink_logger(args, out_path: Path) -> int:
    exe = which("JLinkRTTLogger") or which("JLinkRTTLoggerCL")
    if not exe:
        return 127
    info("Using J-Link RTT Logger backend")

    # JLinkRTTLogger requires target selection and can benefit from an explicit RTT address.
    # Many versions accept -Device/-If/-Speed/-RTTChannel/-RTTAddress/-FileName.
    cmd = [exe, "-Device", args.device, "-If", "SWD", "-Speed", str(args.speed), "-RTTChannel", str(args.channel)]

    # Try to resolve _SEGGER_RTT address from the current ELF and pass it to the logger for reliability.
    elf = _find_elf(None)
    if elf:
        addr = _resolve_rtt_addr(elf)
        if addr:
            # Ensure 0x prefix
            if not addr.startswith("0x") and not addr.startswith("0X"):
                addr = "0x" + addr
            cmd += ["-RTTAddress", addr]
            info(f"Using RTT address {addr} from {elf}")

    cmd += ["-FileName", str(out_path)]

    info("Starting JLinkRTTLogger; press Ctrl-C to stop")
    try:
        # Run subprocess and also tail the file for live echo
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        # Some versions only write to file; we'll follow the file and also emit any stdout.
        time.sleep(0.5)
        with out_path.open("a", buffering=1) as fp:
            # Lightweight follow loop
            last_size = out_path.stat().st_size if out_path.exists() else 0
            while True:
                # Echo any tool output lines too
                if proc.stdout and not proc.stdout.closed:
                    while True:
                        line = proc.stdout.readline()
                        if not line:
                            break
                        # Logger messages go to stderr; keep them visible
                        sys.stderr.write(line)

                # Show new content from file
                try:
                    cur_size = out_path.stat().st_size
                    if cur_size > last_size:
                        with out_path.open("r") as rf:
                            rf.seek(last_size)
                            chunk = rf.read(cur_size - last_size)
                            for raw in chunk.splitlines():
                                # RTTLogger already timestamps; avoid double stamping
                                sys.stdout.write(raw + "\n")
                        last_size = cur_size
                except FileNotFoundError:
                    pass

                rc = proc.poll()
                if rc is not None:
                    return rc
                time.sleep(0.1)
    except KeyboardInterrupt:
        try:
            proc.terminate()
        except Exception:
            pass
        return 0


def run_jlink_gdb_client(args, out_path: Path) -> int:
    gdbsrv = which("JLinkGDBServer")
    rttclient = which("JLinkRTTClient")
    if not (gdbsrv and rttclient):
        return 127
    info("Using J-Link GDB Server + JLinkRTTClient backend")

    gdb_cmd = [
        gdbsrv,
        "-device",
        args.device,
        "-if",
        "SWD",
        "-speed",
        str(args.speed),
        "-singlerun",
        "-quiet",
    ]

    # Start GDB server
    gdb_proc = subprocess.Popen(gdb_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    # Wait for RTT port to be ready
    start = time.time()
    while time.time() - start < 5.0:
        try:
            with socket.create_connection(("127.0.0.1", args.gdb_rtt_port), timeout=0.5):
                break
        except Exception:
            pass
        time.sleep(0.1)

    # Connect RTT client
    # Many versions of JLinkRTTClient do not accept CLI args; default connects to :19021
    client_cmd = [rttclient]
    info(f"Starting JLinkRTTClient; press Ctrl-C to stop")
    client_proc = subprocess.Popen(client_cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)

    try:
        with out_path.open("a", buffering=1) as fp:
            while True:
                line = client_proc.stdout.readline()
                if not line:
                    # Check processes still alive
                    rc_client = client_proc.poll()
                    rc_gdb = gdb_proc.poll()
                    if rc_client is not None:
                        return rc_client
                    if rc_gdb is not None and rc_gdb != 0:
                        info(f"JLinkGDBServer exited with code {rc_gdb}")
                        return rc_gdb
                    time.sleep(0.05)
                    continue

                # JLinkRTTClient emits raw text lines; prefix timestamp unless disabled
                line = line.rstrip("\r\n")
                write_line(fp, line, not args.no_timestamps)
    except KeyboardInterrupt:
        pass
    finally:
        try:
            client_proc.terminate()
        except Exception:
            pass
        try:
            # Send Ctrl-C to gdb server then terminate
            if gdb_proc and gdb_proc.poll() is None:
                gdb_proc.send_signal(signal.SIGINT)
                time.sleep(0.2)
                gdb_proc.terminate()
        except Exception:
            pass
    return 0


def _find_elf(explicit: str | None) -> Path | None:
    if explicit:
        p = Path(explicit)
        return p if p.exists() else None
    # Try common Zephyr build locations
    candidates: list[Path] = []
    try:
        for pat in ["app/build*/zephyr/zephyr.elf", "app/build/zephyr/zephyr.elf", "**/build*/zephyr/zephyr.elf"]:
            for f in Path.cwd().glob(pat):
                candidates.append(f)
        if candidates:
            # Newest by mtime first
            candidates.sort(key=lambda p: p.stat().st_mtime, reverse=True)
            return candidates[0]
    except Exception:
        pass
    return None


def _resolve_rtt_addr(elf_path: Path) -> str | None:
    # Try nm first
    nm = which("nm")
    if nm and elf_path.exists():
        try:
            out = subprocess.check_output([nm, "-n", str(elf_path)], text=True, errors="ignore")
            for line in out.splitlines():
                parts = line.strip().split()
                if len(parts) >= 3 and parts[-1] == "_SEGGER_RTT":
                    addr = parts[0]
                    return addr
        except Exception:
            pass
    # Fallback: try readelf
    readelf = which("readelf")
    if readelf:
        try:
            out = subprocess.check_output([readelf, "-sW", str(elf_path)], text=True, errors="ignore")
            for line in out.splitlines():
                if "_SEGGER_RTT" in line:
                    # Format: Num: Value Size Type Bind Vis Ndx Name
                    fields = line.split()
                    if len(fields) >= 8:
                        return fields[1]
        except Exception:
            pass
    return None


    return 127


def main() -> int:
    args = parse_args()
    out_path = Path(args.outfile) if args.outfile else Path("logs") / f"rtt_{int(time.time())}.log"
    out_path.parent.mkdir(parents=True, exist_ok=True)

    # Try tool selection
    tool_order = []
    if args.tool == "auto":
        # Prefer GDBServer + RTTClient for stable streaming
        tool_order = ["jlink", "jlink-logger"]
    else:
        tool_order = [args.tool]

    rc = 127
    for tool in tool_order:
        if tool == "jlink-logger":
            rc = run_jlink_logger(args, out_path)
        elif tool == "jlink":
            rc = run_jlink_gdb_client(args, out_path)
        elif tool == "openocd":
            rc = 127
        else:
            continue

        if rc == 0:
            return 0
        elif rc == 127:
            info(f"Backend '{tool}' not available; trying next")
            continue
        else:
            # Backend ran but exited with error
            return rc

    info("No RTT backend succeeded. Please install J-Link or OpenOCD.")
    return 1


if __name__ == "__main__":
    sys.exit(main())
