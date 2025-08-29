#!/usr/bin/env python3
"""
Capture UART output to a timestamped log file while echoing to stdout.

Usage:
  ./scripts/uart_capture.py --port /dev/ttyUSB0 --baud 115200 --outfile logs/uart_$(date +%s).log
"""

import argparse
import sys
import time
from pathlib import Path

try:
    import serial
except Exception as e:
    print("pyserial not installed. Install with: pip install pyserial", file=sys.stderr)
    sys.exit(1)


def parse_args():
    p = argparse.ArgumentParser()
    p.add_argument("--port", required=True, help="Serial port, e.g. /dev/ttyUSB0")
    p.add_argument("--baud", type=int, default=115200, help="Baud rate (default 115200)")
    p.add_argument("--outfile", required=True, help="Output log file path")
    p.add_argument("--timeout", type=float, default=0.5, help="Read timeout seconds")
    return p.parse_args()


def main():
    args = parse_args()
    out_path = Path(args.outfile)
    out_path.parent.mkdir(parents=True, exist_ok=True)

    ser = serial.Serial(args.port, args.baud, timeout=args.timeout)
    ser.reset_input_buffer()
    ser.reset_output_buffer()

    print(f"Capturing UART from {args.port} @ {args.baud} to {out_path}")
    with out_path.open("a", buffering=1) as f:
        try:
            while True:
                line = ser.readline()
                if not line:
                    continue
                ts = time.strftime("%Y-%m-%d %H:%M:%S")
                text = line.decode("utf-8", errors="ignore").rstrip("\r\n")
                rec = f"{ts} | {text}\n"
                sys.stdout.write(rec)
                f.write(rec)
        except KeyboardInterrupt:
            pass
        finally:
            ser.close()


if __name__ == "__main__":
    main()

