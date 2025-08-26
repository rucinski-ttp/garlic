# Garlic - nRF52-DK Zephyr Development Project

## 🤖 AI Agent Development Project

This repository demonstrates embedded development using AI agents. The project is specifically designed to be AI-agent friendly, with comprehensive documentation and helper scripts.

## 📋 Project Overview

- **Board**: Nordic nRF52-DK (nRF52832)
- **Framework**: Zephyr RTOS
- **Current Application**: LED Blinky at 3Hz with serial output
- **Purpose**: Demonstrate AI agent capability for embedded development

## 🚀 Quick Start

```bash
# 1. Clone and setup (if not already done)
./scripts/setup.sh

# 2. Build the application
./scripts/build.sh

# 3. Flash to board
./scripts/flash.sh

# 4. Monitor serial output
./scripts/monitor.sh
```

## 📁 Project Structure

```
garlic/
├── app/                    # Application source code
│   ├── src/
│   │   └── main.c         # Main application (LED blinky)
│   ├── CMakeLists.txt     # CMake configuration
│   ├── prj.conf           # Zephyr configuration
│   └── build/             # Build output (git-ignored)
├── scripts/               # Helper scripts for development
│   ├── setup.sh          # Initial environment setup
│   ├── source.sh         # Environment variables
│   ├── build.sh          # Build application
│   ├── flash.sh          # Flash to board
│   └── monitor.sh        # Serial monitor
├── docs/                  # Additional documentation
├── zephyr/               # Zephyr RTOS source
└── modules/              # External modules (git-ignored)
```

## 🛠️ Development Workflow

### Building
```bash
./scripts/build.sh          # Normal build
./scripts/build.sh clean    # Clean build
./scripts/build.sh menuconfig # Configure Zephyr options
```

### Flashing
```bash
./scripts/flash.sh          # Default (OpenOCD - recommended)
./scripts/flash.sh openocd  # Explicitly use OpenOCD
./scripts/flash.sh pyocd    # Use pyOCD
./scripts/flash.sh west     # Use West (requires nrfjprog)
```

### Monitoring
```bash
./scripts/monitor.sh        # Auto-detect port at 115200 baud
./scripts/monitor.sh 115200 /dev/ttyACM0  # Specify port
```

## 🤖 AI Agent Instructions

### Key Information
1. **Board Connection**: nRF52-DK connects via USB, appears as SEGGER J-Link
2. **Serial Port**: Usually `/dev/ttyACM0` on Linux, auto-detected by monitor script
3. **Flash Method**: OpenOCD is most reliable, followed by pyOCD
4. **Build System**: Uses West and CMake, all handled by scripts

### Common Tasks

#### Modify the Application
1. Edit `app/src/main.c`
2. Run `./scripts/build.sh`
3. Run `./scripts/flash.sh`
4. Verify with `./scripts/monitor.sh`

#### Add New Source Files
1. Add `.c` files to `app/src/`
2. Update `app/CMakeLists.txt` to include new sources
3. Rebuild and flash

#### Configure Zephyr
- Edit `app/prj.conf` for configuration options
- Run `./scripts/build.sh menuconfig` for interactive configuration

### Environment Setup
The `scripts/source.sh` script sets up:
- Python virtual environment at `~/zephyr-venv`
- Zephyr SDK at `~/tools/zephyr-global/zephyr-sdk-0.16.8`
- PATH and environment variables

### Troubleshooting

#### Board Not Detected
- Check USB connection
- Verify with `lsusb | grep SEGGER`
- May need to add user to `dialout` group

#### Flash Failures
- Try different flash method: `./scripts/flash.sh openocd`
- Kill any hanging debug sessions: `pkill openocd`
- Check board is powered and not in debug mode

#### Build Issues
- Clean build: `./scripts/build.sh clean`
- Ensure environment is sourced: `source scripts/source.sh`

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