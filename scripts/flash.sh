#!/bin/bash

# Flash script for nRF52-DK board (J-Link only)
# Usage: ./scripts/flash.sh [jlink]
# Default: jlink

set -euo pipefail

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Source environment
SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
ROOT_DIR="${SCRIPT_DIR}/.."
source "${SCRIPT_DIR}/source.sh"

# Default flasher
FLASHER=${1:-jlink}

# Change to app directory
cd "${ROOT_DIR}/app"

# Resolve build directory
BUILD_DIR=${GARLIC_BUILD_DIR:-build}

# Prepare flash log file (fall back to /tmp if needed)
mkdir -p build 2>/dev/null || true
LOGFILE="build/flash.log"
if ! touch "$LOGFILE" 2>/dev/null; then
    LOGFILE=$(mktemp /tmp/garlic_flash.XXXXXX.log)
fi

# Check if hex file exists
if [ ! -f "${BUILD_DIR}/zephyr/zephyr.hex" ]; then
    echo -e "${RED}Error: zephyr.hex not found!${NC}"
    echo "Please run ./scripts/build.sh first"
    exit 1
fi

# Check for board connection
echo -e "${BLUE}Checking for nRF52-DK board...${NC}"
if lsusb | grep -q "SEGGER\|1366"; then
    echo -e "${GREEN}✓ Board detected (SEGGER J-Link)${NC}"
else
    echo -e "${YELLOW}⚠ Board not detected. Please ensure the nRF52-DK is connected via USB.${NC}"
    echo "Continuing anyway..."
fi

echo ""
echo -e "${BLUE}Flashing nRF52-DK using ${FLASHER}...${NC}"

# Kill any existing debug sessions to avoid USB conflicts
echo "Cleaning up any existing debug sessions..."
pkill -f pyocd 2>/dev/null || true
pkill -f openocd 2>/dev/null || true
pkill -f JLink 2>/dev/null || true
sleep 1

case $FLASHER in
    jlink)
        echo -e "${YELLOW}Using JLink...${NC}"
        if ! command -v JLinkExe &> /dev/null; then
            echo -e "${RED}Error: JLinkExe not found!${NC}"
            echo "Please install SEGGER J-Link tools"
            exit 1
        fi
        
        # Create JLink script
        cat > build/flash.jlink <<EOF
device NRF52832_XXAA
si SWD
speed 4000
connect
h
loadfile ${BUILD_DIR}/zephyr/zephyr.hex
r
g
q
EOF
        
        if timeout 30 JLinkExe -CommandFile build/flash.jlink; then
            echo -e "${GREEN}✓ Flash successful with JLink!${NC}"
        else
            echo -e "${RED}✗ JLink flash failed!${NC}"
            exit 1
        fi
        ;;
    
    *)
        echo -e "${RED}Unknown flasher: $FLASHER${NC}"
        echo "Usage: $0 [jlink]"
        echo "  jlink   - SEGGER J-Link tools"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Flash completed successfully!${NC}"
echo -e "${BLUE}The LED should now be blinking at 3Hz${NC}"
echo ""
echo "To monitor serial output, run: ./scripts/monitor.sh"
