# Project Garlic

Minimal, reliable firmware for the Nordic nRF52‑DK (nRF52832) built on Zephyr 3.7. The app provides a DMA‑backed, non‑blocking UART path with a simple framed transport and a small set of commands. BLE (Nordic UART Service) is supported as an alternate transport. Unit tests cover core logic; a pytest suite exercises hardware end‑to‑end.

## Quick Start

```bash
# 1. Clone and setup (if not already done)
./scripts/setup.sh

# 2. Build + flash the application
./scripts/build.sh && ./scripts/flash.sh

# 3. (Optional) Monitor serial output via UART
./scripts/monitor.sh 115200 /dev/ttyUSB0

# 4. (Optional) Run hardware tests (serial or BLE)
./scripts/test_integration.sh --interface serial --port /dev/ttyUSB0
# or over BLE by address
pytest -q tests/integration --run-hardware --interface ble --garlic-ble-address <MAC>
```

## Project Structure

```
garlic/
├── app/                    # Application source code
│   ├── src/
│   │   └── app_runtime.c  # App bootstrap, LED heartbeat, transport hookup
│   ├── CMakeLists.txt     # CMake configuration
│   ├── prj.conf           # Zephyr configuration
│   └── build/             # Build output (git-ignored)
├── scripts/               # Helper scripts
│   ├── setup.sh          # Initial environment setup
│   ├── source.sh         # Environment variables
│   ├── build.sh          # Build application
│   ├── flash.sh          # Flash to board
│   ├── monitor.sh        # Serial monitor
│   ├── check_format.sh   # Check code formatting
│   ├── fix_format.sh     # Auto-fix code formatting
│   └── format.sh         # Combined formatting tool
│   └── test_integration.sh # Build, flash, run pytest hardware tests
├── docs/                  # Additional documentation
├── zephyr/               # Zephyr RTOS source
└── modules/               # External modules (git-ignored)
```

## Development Workflow

### Formatting and Tidy
```bash
# Formatting (clang-format)
./scripts/format.sh check   # Check formatting (CI runs this)
./scripts/format.sh fix     # Auto-fix formatting

# Static analysis (clang-tidy)
./scripts/format.sh tidy        # Check tidy rules (no fixes)
./scripts/format.sh tidy-fix    # Apply tidy fixes, then run 'fix'
```

Notes:
- CI fails if formatting or tidy checks are not clean; CI does not auto‑apply fixes.
- Run `tidy-fix` and then `fix` locally before pushing to keep CI green.

### Building
```bash
./scripts/build.sh          # Normal build
./scripts/build.sh clean    # Clean build
./scripts/build.sh menuconfig # Configure Zephyr options
```

### Flashing (J-Link)
```bash
./scripts/flash.sh          # Uses SEGGER J-Link
./scripts/flash.sh jlink    # Explicitly use J-Link
```

### Monitoring (UART)
```bash
./scripts/monitor.sh        # Auto-detect port at 115200 baud
./scripts/monitor.sh 115200 /dev/ttyACM0  # Specify port
```

### Capture UART logs
```bash
# Save UART output to a log file while echoing to stdout
python3 scripts/uart_capture.py --port /dev/ttyUSB0 --baud 115200 --outfile logs/uart_$(date +%s).log
```

### Capture RTT logs (J-Link)
```bash
# Auto-select J-Link backend (Logger → RTT Client)
./scripts/rtt_capture.sh

# Explicit control
./scripts/rtt_capture.sh --tool jlink-logger                 # Use JLinkRTTLogger
./scripts/rtt_capture.sh --tool jlink --channel 0            # Use JLinkGDBServer + JLinkRTTClient

# Custom output file
./scripts/rtt_capture.sh --outfile logs/rtt_manual.log

# Direct Python
python3 scripts/rtt_capture.py --tool auto --outfile logs/rtt_$(date +%s).log

Note: the helper adds a default 10s timeout (adjustable with `--timeout`).

### BLE Temperature Streaming Demo

Stream TMP119 temperature over BLE using the same test fixtures:

```
python3 scripts/ble_temp_stream.py --address <MAC>
# or scan by name
python3 scripts/ble_temp_stream.py --name GarlicDK
```
Options: `--interval` (seconds), `--count` (0=forever), `--addr7` (TMP119 I2C address).
```

## Testing

### Unit (host)
```bash
cmake -S tests/unit -B tests/unit/build -DCMAKE_BUILD_TYPE=Release
cmake --build tests/unit/build -j
ctest --test-dir tests/unit/build --output-on-failure
```

### Integration (hardware)
Runs against the nRF52‑DK over UART or BLE. By default, scripts prefer FTDI on `/dev/ttyUSB0`; J‑Link CDC is a fallback. RTT capture is available for early boot logs.
```
./scripts/test_integration.sh --port /dev/ttyUSB0  # override port if needed
```

Transport/commands concurrency:
- The command transport binds per-link (UART and BLE) via a small `cmd_transport_binding` that
  holds a dedicated response buffer and a lock (Zephyr mutex) around response building/sending.
  This prevents cross-link races under burst load and maintains payload integrity without any
  dynamic allocation.

## Troubleshooting
- Board not detected: check USB, `lsusb | grep SEGGER`, add user to `dialout` if needed.
- Flash failures: ensure J‑Link tools are installed; kill stale sessions; power‑cycle the board.
- Build issues: clean with `./scripts/build.sh clean`; verify environment (`source scripts/source.sh`).

## Notes
- Warnings are treated as errors (`CONFIG_COMPILER_WARNINGS_AS_ERRORS=y`).
- No dynamic allocation on runtime paths.
- Public C API is namespaced: `grlc_<module>_<function>()`.

## 📚 Resources

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [nRF52-DK Documentation](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf52832_dk%2FUG%2Fnrf52_DK%2Fintro.html)
- [West Tool Documentation](https://docs.zephyrproject.org/latest/develop/west/index.html)

## Contributing
- Keep changes focused; update docs and scripts as needed.
- Run `./scripts/format.sh tidy-fix` and `./scripts/format.sh fix` before pushing.
- Ensure unit tests pass; run hardware tests for transport and command changes when feasible.

## License
TBD

---

Protocol & Commands: see `docs/2-0-PROTOCOL.md` for framing and command payloads. Unit tests cover CRC32, transport, command registry/dispatch, and UART DMA paths. Integration tests exercise the same over real hardware (UART and BLE).
