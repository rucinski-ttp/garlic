# Development Tools

## Required Tools

### Zephyr SDK
- **Location**: `~/tools/zephyr-global/zephyr-sdk-0.16.8`
- **Components**: ARM toolchain, host tools
- **Setup**: Already configured in `scripts/source.sh`

### West Meta-Tool
- **Purpose**: Zephyr repository management
- **Installation**: In Python venv (`~/zephyr-venv`)
- **Commands**:
  - `west update` - Update Zephyr and modules
  - `west build` - Build applications
  - `west flash` - Flash to board

### Python Environment
- **Venv Location**: `~/zephyr-venv`
- **Key Packages**:
  - west - Zephyr meta-tool
  - pyocd - Flashing/debugging tool
  - pyelftools - ELF file analysis

## Flashing Tools

### SEGGER J-Link (Recommended)
- On-board debugger on nRF52-DK
- Usage: `./scripts/flash.sh` (uses JLinkExe)
  - Under the hood: halts, loads HEX, verifies, resets, runs

### nrfjprog
- Nordic's official tool (not required in this project)


## Known Issues

### USB Connection Timeouts
The board may cause USB timeouts when connected for extended periods. This affects:
- Serial console access (`/dev/ttyACM0`)
- Debugger connections (J-Link)
- Flash operations

**Workarounds**:
1. Unplug and replug the board before flashing
2. Use shorter timeout values in scripts
3. Avoid keeping debugger connections open
4. Consider adding USB reset commands before operations

## Build Tools

### CMake
- Version: 3.31.6
- Handles Zephyr build configuration
- Invoked via West

### Ninja
- Fast build system
- Used by CMake for compilation

### Device Tree Compiler
- Version: 1.7.2
- Processes device tree files
- Required for hardware configuration
