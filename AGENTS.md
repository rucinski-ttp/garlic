# AGENTS Guide

Welcome! This document orients AI agents (and humans) to the Garlic nRF52-DK project so you can be effective quickly and safely.

## Project Summary

- Board: Nordic nRF52-DK (nRF52832)
- RTOS: Zephyr 3.7
- Goal: DMA-based, non-blocking UART using a circular buffer, tested with unit tests (GTest) and hardware pytest.
- Debug/logging: RTT enabled; UART used for data path and status messages.

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
- `scripts/` – Helper scripts (env, build, flash, monitor UART, capture UART logs)
- `.github/workflows/ci.yml` – CI on local runner `dell-runner`

## Guardrails / Quality

- No dynamic allocation in firmware code paths
- Treat warnings as errors: `CONFIG_COMPILER_WARNINGS_AS_ERRORS=y`
- Clang-format enforced; CI checks formatting
- All code must be unit-tested; DMA UART also validated by hardware pytest

## Environment & Build

Use the provided scripts; they source the expected Zephyr SDK path via `scripts/source.sh`. The local runner (or developer machine) should have the Zephyr SDK installed at the configured path.

- Setup once: `./scripts/setup.sh` (initializes West if needed)
- Build: `./scripts/build.sh` (or `./scripts/build.sh clean`)
- Flash: `./scripts/flash.sh` (auto-falls back to JLink if OpenOCD fails)
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

### Hardware Pytest

Run against the FTDI port:

```
./scripts/test_integration.sh --port /dev/ttyUSB0
```

This runs with `--run-hardware` and exercises:
- Startup status presence and rate
- Echo per line
- TX/RX counters
- Continuous data stream handling

## DMA UART Behavior

- RX uses async UARTE double buffering. RX inactivity timeout (20 ms) is used so partial lines are delivered promptly.
- TX uses a circular buffer; `uart_dma_process()` drains it.
- The app emits one status line per second, and one echo line per newline received.

## CI

- Workflow: `.github/workflows/ci.yml`
- Runner: `dell-runner` (local GitHub runner installed on laptop)
- Steps:
  - clang-format check
  - Build Zephyr app
  - Run unit tests
  - Upload binaries as artifacts

## Common Pitfalls

- J-Link VCOM vs FTDI contention on P0.06/P0.08: The DK’s interface MCU is wired to these pins by default. Prefer FTDI and ensure VCOM isn’t asserting on the lines during testing.
- Serial open/settle: The host can toggle DTR; tests include a short settle delay after open.
- Runtime `uart_configure()` may be unsupported on some stacks. The driver proceeds using DT defaults when configure returns an error.

## Future (RTT)

- RTT is enabled and used for logs. A robust RTT capture script can be added to complement `uart_capture.py`. Keep UART for data path; use RTT for debug logs.

## Contributing Tips for Agents

- Respect code style: run `./scripts/format.sh fix` before PRs.
- Keep changes focused; update docs and scripts as needed.
- Add tests for new functionality; avoid regressions.
- For bring-up or experiments, place one-offs under `app/src/app/examples/` and keep the main build clean.

