# Introduction

## Project Overview

This is a Zephyr RTOS project for the Nordic nRF52-DK development board. The project provides a foundation for embedded development on the nRF52832 SoC.

## Hardware

- **Board**: Nordic nRF52-DK
- **SoC**: nRF52832
  - ARM Cortex-M4F @ 64MHz
  - 512KB Flash
  - 64KB RAM
  - Bluetooth 5.0 / BLE
  - NFC-A

## Software Stack

- **RTOS**: Zephyr v3.7.0
- **Build System**: CMake + West
- **Toolchain**: Zephyr SDK 0.16.8

## Quick Start

1. Source the environment:
   ```bash
   source scripts/source.sh
   ```

2. Build the application:
   ```bash
   ./scripts/build.sh
   ```

3. Flash to board:
   ```bash
   ./scripts/flash.sh
   ```

## Project Structure

```
/projects/garlic/
├── app/               # Application source code
│   ├── src/          # C source files
│   ├── CMakeLists.txt
│   └── prj.conf      # Kconfig settings
├── zephyr/           # Zephyr RTOS source
├── modules/          # External modules (HAL, etc.)
├── scripts/          # Build and utility scripts
└── docs/            # Documentation
```