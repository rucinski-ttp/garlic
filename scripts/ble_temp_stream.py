#!/usr/bin/env python3
"""
BLE Temperature Streamer for Garlic nRF52-DK

Uses the same BLE fixtures as the integration tests to connect to the
device over Nordic UART Service (NUS) and periodically reads the TMP119
temperature, printing it to stdout.

Examples:
  # Connect by MAC address (recommended)
  python3 scripts/ble_temp_stream.py --address DC:AB:77:A1:08:34

  # Or connect by advertised name (will scan)
  python3 scripts/ble_temp_stream.py --name GarlicDK

  # Customize sampling interval and count
  python3 scripts/ble_temp_stream.py --address <MAC> --interval 0.5 --count 60
"""

import argparse
import os
import sys
import time
from pathlib import Path


def _add_fixtures_to_path():
    # Reuse the exact same fixtures used by pytest
    repo_root = Path(__file__).resolve().parents[1]
    fixtures = repo_root / "tests" / "integration" / "fixtures"
    if not fixtures.exists():
        print("ERROR: fixtures not found; run from repository root.", file=sys.stderr)
        sys.exit(2)
    sys.path.insert(0, str(fixtures))


def parse_args():
    p = argparse.ArgumentParser(description="Stream TMP119 temperature over BLE using test fixtures")
    p.add_argument("--address", help="BLE MAC address to connect to (skips scan)", default=None)
    p.add_argument("--name", help="BLE advertised name to match (scan)", default="GarlicDK")
    p.add_argument("--interval", type=float, default=1.0, help="Seconds between readings")
    p.add_argument("--count", type=int, default=0, help="Number of readings (0=forever)")
    p.add_argument("--addr7", type=lambda x: int(x, 0), default=0x48, help="TMP119 7-bit I2C address (default 0x48)")
    return p.parse_args()


def main():
    _add_fixtures_to_path()
    from ble_io import BLEGarlicDevice, BLEDeviceConfig
    from command import CommandClient

    args = parse_args()

    cfg = BLEDeviceConfig(name=args.name, address=args.address)
    print(f"Connecting over BLE (name={cfg.name}, address={cfg.address}) ...", flush=True)
    dev = BLEGarlicDevice(cfg)
    cc = CommandClient(dev)

    print("Connected. Streaming temperature; Ctrl-C to stop.")
    n = 0
    try:
        while True:
            try:
                mc = cc.tmp119_read_temp_mc(args.addr7, timeout=2.0)
                c = mc / 1000.0
                print(f"{time.strftime('%H:%M:%S')} | {c:.3f} °C", flush=True)
            except Exception as e:
                # Print and continue; BLE fixture will auto-reconnect on next write
                print(f"WARN: read failed: {e}", file=sys.stderr, flush=True)
            n += 1
            if args.count and n >= args.count:
                break
            time.sleep(max(0.01, args.interval))
    except KeyboardInterrupt:
        pass
    finally:
        try:
            dev.disconnect()
        except Exception:
            pass


if __name__ == "__main__":
    main()

