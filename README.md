# Garlic - nRF52-DK Zephyr Development Project

## 🤖 AI Agent Development Project

This repository demonstrates embedded development using AI agents. The project is specifically designed to be AI-agent friendly, with comprehensive documentation and helper scripts.

## 📋 Project Overview

- **Board**: Nordic nRF52-DK (nRF52832)
- **Framework**: Zephyr RTOS
- **Current Application**: UART DMA echo + 1 Hz status (LED heartbeat)
- **Protocol**: Layered serial command stack (UART DMA → Transport → Commands)
- **Purpose**: Demonstrate AI agent capability for embedded development

## 🚀 Quick Start

```bash
# 1. Clone and setup (if not already done)
./scripts/setup.sh

# 2. Build + flash the application
./scripts/build.sh && ./scripts/flash.sh

# 3. (Optional) Monitor serial output via UART
./scripts/monitor.sh 115200 /dev/ttyACM0

# 4. (Optional) Run hardware tests (auto-detects port)
./scripts/test_integration.sh
```

## 📁 Project Structure

```
garlic/
├── app/                    # Application source code
│   ├── src/
│   │   └── app/main.c     # Main application (UART DMA echo/status)
│   ├── CMakeLists.txt     # CMake configuration
│   ├── prj.conf           # Zephyr configuration
│   └── build/             # Build output (git-ignored)
├── scripts/               # Helper scripts for development
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
└── modules/              # External modules (git-ignored)
```

## 🛠️ Development Workflow

### Code Formatting and Tidy
```bash
# Formatting (clang-format)
./scripts/format.sh check   # Check formatting (CI runs this)
./scripts/format.sh fix     # Auto-fix formatting

# Static analysis (clang-tidy)
./scripts/format.sh tidy        # Check tidy rules (no fixes)
./scripts/format.sh tidy-fix    # Apply tidy fixes, then run 'fix'
```

Notes:
- CI fails if formatting or tidy checks are not clean; CI does not auto-apply fixes.
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
```

## 🤖 AI Agent Instructions

### Key Information
1. **Board Connection**: nRF52-DK connects via USB, appears as SEGGER J-Link
2. **Serial Port**: Usually `/dev/ttyACM0` on Linux, auto-detected by monitor script
3. **Flash Method**: J-Link
4. **Build System**: Uses West and CMake, all handled by scripts

### Common Tasks

#### Modify the Application
1. Runtime/loop: edit `app/src/app/src/app_runtime.c` (early boot lines, LED heartbeat, transport hookup)
2. Commands: add/modify under `app/src/commands/*` (each module has its own CMakeLists)
3. Build `./scripts/build.sh`, flash `./scripts/flash.sh`
4. Verify via UART (`./scripts/monitor.sh`) or RTT (`./scripts/rtt_capture.sh`)

#### Add New Source Files
- Prefer adding within a module (e.g., `app/src/commands/foo/src/*.c`).
- Register sources in that module’s `CMakeLists.txt` instead of the app root.
- Rebuild and flash.

#### Configure Zephyr
- Edit `app/prj.conf` for configuration options
- Run `./scripts/build.sh menuconfig` for interactive configuration

### Environment Setup
The `scripts/source.sh` script sets up:
- Python virtual environment at `~/zephyr-venv`
- Zephyr SDK at `~/tools/zephyr-global/zephyr-sdk-0.16.8`
- PATH and environment variables

### Testing & Troubleshooting

#### Unit tests (host)
```bash
cmake -S tests/unit -B tests/unit/build -DCMAKE_BUILD_TYPE=Release
cmake --build tests/unit/build -j
ctest --test-dir tests/unit/build --output-on-failure
```

#### Integration tests (hardware)
Runs against the nRF52-DK over J-Link and UART. By default uses J-Link RTT capture for early boot reliability.
```
./scripts/test_integration.sh --port /dev/ttyUSB0  # override port if needed
```

#### Board Not Detected
- Check USB connection
- Verify with `lsusb | grep SEGGER`
- May need to add user to `dialout` group

#### Flash Failures
- Ensure J-Link tools are installed (JLinkExe, JLinkGDBServer)
- Kill any hanging debug sessions: `pkill JLink`/close Ozone/VSCode
- Check board is powered and not in debug mode

#### Build Issues
- Clean build: `./scripts/build.sh clean`
- Ensure environment is sourced: `source scripts/source.sh`
- Warnings are treated as errors (CONFIG_COMPILER_WARNINGS_AS_ERRORS=y)

## 🎯 Next Development Goals

1. **Code Quality** (Next Sprint)
   - Add clang-format configuration
   - Implement pre-commit hooks
   - Add static analysis

2. **Testing Infrastructure**
   - Add Google Test for unit testing
   - Create test fixtures for hardware abstraction
   - Add pytest for integration testing

3. **UART Features**
   - Implement UART communication
   - Add command shell interface
   - Create pytest-based hardware tests

4. **CI/CD Pipeline**
   - GitHub Actions for build verification
   - Automated testing on PR
   - Release automation

## 📚 Resources

- [Zephyr Documentation](https://docs.zephyrproject.org/)
- [nRF52-DK Documentation](https://infocenter.nordicsemi.com/index.jsp?topic=%2Fug_nrf52832_dk%2FUG%2Fnrf52_DK%2Fintro.html)
- [West Tool Documentation](https://docs.zephyrproject.org/latest/develop/west/index.html)

## 🤝 Contributing

This project is designed for AI agent development. When making changes:
1. Update documentation if adding new features
2. Ensure scripts remain AI-agent friendly (clear output, good error handling)
3. Test all changes with hardware before committing

## 📝 License

[Add appropriate license]

---

**Note for AI Agents**: This repository is your workspace for embedded development. All scripts are designed to be robust and provide clear feedback. If you encounter issues, check the troubleshooting section or examine script output carefully.
### Protocol & Commands
See docs/PROTOCOL.md for the transport framing and command payload formats.

Unit tests cover CRC32, transport framing/fragmentation/reassembly and stats, command registry/dispatch (with mocks), command glue (fragmented), UART DMA RX/TX paths. Integration tests exercise command handlers and transport behavior on real hardware.
