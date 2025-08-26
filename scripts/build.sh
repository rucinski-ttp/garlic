#!/bin/bash

# Build script for nRF52-DK Zephyr application
# Usage: ./scripts/build.sh [clean]

set -e

# Source environment
source "$(dirname "$0")/source.sh"

# Parse arguments
if [ "$1" == "clean" ]; then
    echo "Performing clean build..."
    BUILD_ARGS="--pristine"
else
    BUILD_ARGS=""
fi

# Change to app directory
cd /projects/garlic/app

# Build the application
echo "Building application for nRF52-DK..."
west build -b nrf52dk/nrf52832 $BUILD_ARGS

# Display build summary
echo ""
echo "Build completed successfully!"
echo "Output files:"
echo "  ELF: build/zephyr/zephyr.elf"
echo "  HEX: build/zephyr/zephyr.hex"
echo "  BIN: build/zephyr/zephyr.bin"

# Display memory usage
echo ""
echo "Memory usage:"
grep -E "FLASH:|RAM:" build/output.log 2>/dev/null || \
    (cd build && arm-zephyr-eabi-size zephyr/zephyr.elf)