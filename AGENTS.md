# AGENTS Guide

Welcome! This document orients AI agents (and humans) to the Garlic nRF52-DK project so you can be effective quickly and safely.

## Project Summary

- Board: Nordic nRF52-DK (nRF52832)
- RTOS: Zephyr 3.7
- Goal: DMA-based, non-blocking UART using a circular buffer, tested with unit tests (GTest) and hardware pytest.
- Debug/logging: RTT enabled; UART used for data path and status messages.
- Protocol: Layered command stack (UART DMA or BLE NUS → Transport → Commands), documented in docs/PROTOCOL.md

## Code Layout

- `app/` Zephyr application
  - `src/app/main.c` – DMA UART app (echo + 1 Hz status, LED heartbeat)
  - `src/drivers/uart_dma.c` – Async UARTE driver using circular buffers
  - `src/utils/circular_buffer.c` – One-byte-free ring buffer + helpers
  - `include/` – Public headers for above
  - `src/app/examples/` – Archived bring-up examples (not built)
  - `boards/nrf52dk_nrf52832.overlay` – UART pins + speed (FTDI TX=P0.06, RX=P0.08 @ 115200)
  - `CMakeLists.txt` and `prj.conf` – Build and Kconfig
- `tests/`
  - `unit/` – GTest for circular buffer
  - `integration/` – Pytest for hardware UART with FTDI
- `scripts/` – Helper scripts (env, build, flash, monitor UART, capture UART logs, BLE temp demo)
- `.github/workflows/ci.yml` – CI on local runner `dell-runner`

## Guardrails / Quality

- No dynamic allocation in firmware code paths
- Treat warnings as errors: `CONFIG_COMPILER_WARNINGS_AS_ERRORS=y`
- Style + static analysis:
  - Clang-format enforced for C/C++ style. CI runs a formatting check and fails on diffs.
  - Clang-tidy enforced in CI (e.g., `readability-braces-around-statements`). CI does not apply fixes; it fails if issues are found.
- All code must be unit-tested; DMA UART also validated by hardware pytest
- Doxygen: Public headers and functions must include @brief/@param/@return documentation

## Environment & Build

Use the provided scripts; they source the expected Zephyr SDK path via `scripts/source.sh`. The local runner (or developer machine) should have the Zephyr SDK installed at the configured path.

- Setup once: `./scripts/setup.sh` (initializes West if needed)
- Build: `./scripts/build.sh` (or `./scripts/build.sh clean`)
- Flash: `./scripts/flash.sh` (J-Link)
- Monitor: `./scripts/monitor.sh 115200 /dev/ttyUSB0`
- Capture logs: `python3 scripts/uart_capture.py --port /dev/ttyUSB0 --baud 115200 --outfile logs/uart_$(date +%s).log`

Notes:
- Hardware pytest prefers FTDI (`/dev/ttyUSB0`), then J-Link CDC. You can override with `--garlic-port`.
- The build expects Zephyr SDK at the path configured in `scripts/source.sh`.

## Tests

### Unit

```
cmake -S tests/unit -B tests/unit/build -DCMAKE_BUILD_TYPE=Release
cmake --build tests/unit/build -j
ctest --test-dir tests/unit/build --output-on-failure
```

### Hardware Pytest (serial and BLE)

Run against the FTDI port (serial):

```
./scripts/test_integration.sh --port /dev/ttyUSB0
```

BLE runs (same suite) can be invoked by MAC or name:

```
pytest -q tests/integration --run-hardware --interface ble --garlic-ble-address <MAC>
# or
pytest -q tests/integration --run-hardware --interface ble --garlic-ble-name GarlicDK
```

This runs with `--run-hardware` and exercises:
- Startup status presence and rate
- Echo per line
- TX/RX counters
- Continuous data stream handling
- Upcoming: transport framing and command handlers via Python client classes

## UART & BLE Behavior

- RX uses async UARTE double buffering. RX inactivity timeout (20 ms) is used so partial lines are delivered promptly.
- TX uses a circular buffer over UART; NUS notifications over BLE.
- The app emits one status line per second, and one echo line per newline received.
- LED1 indicates BLE status: blink when advertising, solid when connected.

## CI

- Workflow: `.github/workflows/ci.yml`
- Runner: `dell-runner` (local GitHub runner installed on laptop)
- Steps:
  - clang-format check (no auto-fix)
  - Build Zephyr app
  - Run unit tests
  - Upload binaries as artifacts

Clang-tidy also runs after build in CI and will fail on violations (WarningsAsErrors). CI never applies tidy fixes.

## Formatting & Tidy Workflow

To ensure local results match CI:

- Check formatting only:
  - `./scripts/format.sh check`
- Auto-fix formatting:
  - `./scripts/format.sh fix`
- Run clang-tidy checks (read-only):
  - `./scripts/format.sh tidy`
- Auto-apply clang-tidy fixes, then reformat:
  - `./scripts/format.sh tidy-fix` followed by `./scripts/format.sh fix`

Notes:
- Clang-tidy uses `.clang-tidy` and the unit test compile database. The script builds `tests/unit` to generate `compile_commands.json` automatically.
- CI is intentionally non-destructive: it does not apply fixes, only reports failures. Developers must run `tidy-fix` and `fix` locally before pushing.

## Common Pitfalls

- J-Link VCOM vs FTDI contention on P0.06/P0.08: The DK’s interface MCU is wired to these pins by default. Prefer FTDI and ensure VCOM isn’t asserting on the lines during testing.
- Serial open/settle: The host can toggle DTR; tests include a short settle delay after open.
- Runtime `uart_configure()` may be unsupported on some stacks. The driver proceeds using DT defaults when configure returns an error.

## RTT Capture

- Firmware enables RTT logging (`CONFIG_LOG_BACKEND_RTT=y`). Keep UART for data; use RTT for debug logs.
- Preferred capture command (J-Link only):
  - `./scripts/rtt_capture.sh` → saves to `logs/rtt_<epoch>.log`
- Explicit backends:
  - `./scripts/rtt_capture.sh --tool jlink-logger` (requires `JLinkRTTLogger`)
  - `./scripts/rtt_capture.sh --tool jlink` (starts `JLinkGDBServer` + `JLinkRTTClient`)
- Direct Python usage:
  - `python3 scripts/rtt_capture.py --tool auto --outfile logs/rtt.log`
  - The helper enforces a default 10s timeout; pass `--timeout` to override.

## BLE Tips for Agents

- Prefer connecting by MAC address when available to avoid scan races (`--garlic-ble-address`).
- The BLE fixture reconnects automatically after device reboots and retries writes.
- A simple demo to stream temperature over BLE is provided:
  - `python3 scripts/ble_temp_stream.py --address <MAC>` or `--name GarlicDK`
- Environment overrides:
  - `GARLIC_RTT_TOOL`, `GARLIC_RTT_DEVICE`, `GARLIC_RTT_SPEED`, `GARLIC_RTT_CHANNEL`

Note: A previous experimental script was moved to `scripts/legacy/rtt_monitor.py`.

## Protocol

- Transport: framed, CRC32-verified, supports fragmentation and reassembly (no retransmissions). See docs/PROTOCOL.md.
- Commands: request/response with status and optional payload; each command in its own module under `app/src/app/commands/`.

## Pytest Structure

- Keep fixtures under `tests/integration/fixtures/` (serial, RTT, debug probe, clients)
- Tests under `tests/integration/` import fixtures; `conftest.py` focuses on options and hardware gating

## Contributing Tips for Agents

- Respect code style: run `./scripts/format.sh fix` before PRs.
- Keep changes focused; update docs and scripts as needed.
- Add tests for new functionality; avoid regressions.
- For bring-up or experiments, place one-offs under `app/src/app/examples/` and keep the main build clean.

## PR Policy

- Squash Rebase: All pull requests are squash-rebased onto `origin/main` before merge. Prefer a single, well-curated commit that captures the feature/fix.
- Non-Interactive: Use a non-interactive squash rebase (no interactive editor steps) to keep automation friendly.
- Formatting Required: clang-format must be clean prior to PR. Locally verify with either:
  - `cmake -S app -B app/build && cmake --build app/build --target format-check`, or
  - `./scripts/format.sh check` (and `./scripts/format.sh fix` if needed).
- Tests: Ensure unit tests pass locally. For hardware changes, run the integration pytest suite when feasible.
- CI Clean: PRs should be green on CI (format, build, unit tests).
