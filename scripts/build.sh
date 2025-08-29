#!/bin/bash

# Build script for nRF52-DK Zephyr application
# Usage: ./scripts/build.sh [clean|menuconfig]

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
NC='\033[0m' # No Color

# Source environment
SCRIPT_DIR="$(dirname "$0")"
source "${SCRIPT_DIR}/source.sh"

# Parse arguments
case "$1" in
    clean)
        echo -e "${YELLOW}Performing clean build...${NC}"
        BUILD_ARGS="--pristine"
        ;;
    menuconfig)
        echo -e "${YELLOW}Opening menuconfig...${NC}"
        cd /projects/garlic/app
        west build -t menuconfig
        exit 0
        ;;
    *)
        BUILD_ARGS=""
        ;;
esac

BUILD_DIR=${GARLIC_BUILD_DIR:-build}

# Change to app directory
cd /projects/garlic/app

# Check if west is available
if ! command -v west &> /dev/null; then
    echo -e "${RED}Error: west not found. Please ensure the environment is properly set up.${NC}"
    exit 1
fi

# Build the application
echo -e "${GREEN}Building application for nRF52-DK...${NC}"
echo "Board: nrf52dk/nrf52832"
echo "Build directory: app/${BUILD_DIR}"
echo ""

# Run the build
if west build -d "${BUILD_DIR}" -b nrf52dk/nrf52832 $BUILD_ARGS; then
    echo ""
    echo -e "${GREEN}✓ Build completed successfully!${NC}"
else
    echo -e "${RED}✗ Build failed!${NC}"
    exit 1
fi

# Display build summary
echo ""
echo "Output files:"
if [ -f "${BUILD_DIR}/zephyr/zephyr.elf" ]; then
    echo -e "  ${GREEN}✓${NC} ELF: ${BUILD_DIR}/zephyr/zephyr.elf ($(du -h "${BUILD_DIR}/zephyr/zephyr.elf" | cut -f1))"
fi
if [ -f "${BUILD_DIR}/zephyr/zephyr.hex" ]; then
    echo -e "  ${GREEN}✓${NC} HEX: ${BUILD_DIR}/zephyr/zephyr.hex ($(du -h "${BUILD_DIR}/zephyr/zephyr.hex" | cut -f1))"
fi
if [ -f "${BUILD_DIR}/zephyr/zephyr.bin" ]; then
    echo -e "  ${GREEN}✓${NC} BIN: ${BUILD_DIR}/zephyr/zephyr.bin ($(du -h "${BUILD_DIR}/zephyr/zephyr.bin" | cut -f1))"
fi

# Display memory usage
echo ""
echo "Memory usage:"
if [ -f "${BUILD_DIR}/zephyr/zephyr.map" ]; then
    ${CROSS_COMPILE}size "${BUILD_DIR}/zephyr/zephyr.elf" | tail -n +2 | \
    while read text data bss dec hex filename; do
        flash=$((text + data))
        ram=$((data + bss))
        echo -e "  Flash: ${flash} bytes ($(printf "%.1fK" $(echo "scale=1; $flash/1024" | bc)))"
        echo -e "  RAM:   ${ram} bytes ($(printf "%.1fK" $(echo "scale=1; $ram/1024" | bc)))"
    done
fi

echo ""
echo -e "${GREEN}Ready to flash! Run: ./scripts/flash.sh${NC}"
