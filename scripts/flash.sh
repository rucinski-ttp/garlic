#!/bin/bash

# Flash script for nRF52-DK board
# Usage: ./scripts/flash.sh [openocd|pyocd|west|jlink]
# Default: openocd (most reliable)

set -e

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[0;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# Source environment
SCRIPT_DIR="$(dirname "$0")"
source "${SCRIPT_DIR}/source.sh"

# Default flasher (OpenOCD has been most reliable)
FLASHER=${1:-openocd}

# Change to app directory
cd /projects/garlic/app

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

# Flash based on selected method
case $FLASHER in
    openocd)
        echo -e "${YELLOW}Using OpenOCD (recommended)...${NC}"
        if ! command -v openocd &> /dev/null; then
            echo -e "${RED}Error: OpenOCD not found!${NC}"
            echo "Please install: sudo apt-get install openocd"
            exit 1
        fi
        
        if openocd -f interface/jlink.cfg \
                   -c "transport select swd" \
                   -f target/nrf52.cfg \
                   -c "program ${BUILD_DIR}/zephyr/zephyr.hex verify reset exit" 2>&1 | tee "$LOGFILE"; then
            echo -e "${GREEN}✓ Flash successful with OpenOCD!${NC}"
        else
            echo -e "${RED}✗ OpenOCD flash failed!${NC}"
            echo "Check $LOGFILE for details"
            echo -e "${YELLOW}Falling back to JLink...${NC}"
            FLASHER=jlink
            # Fall through to jlink case
        fi
        ;;
    
    pyocd)
        echo -e "${YELLOW}Using pyOCD...${NC}"
        if ! command -v pyocd &> /dev/null; then
            echo -e "${RED}Error: pyOCD not found!${NC}"
            echo "Please install: pip install pyocd"
            exit 1
        fi
        
        # Use timeout to prevent hanging
        if timeout 30 pyocd flash -t nrf52832 build/zephyr/zephyr.hex; then
            echo -e "${GREEN}✓ Flash successful with pyOCD!${NC}"
        else
            echo -e "${RED}✗ pyOCD flash failed!${NC}"
            echo "Try: ./scripts/flash.sh openocd"
            exit 1
        fi
        ;;
    
    west)
        echo -e "${YELLOW}Using West flash...${NC}"
        if ! command -v west &> /dev/null; then
            echo -e "${RED}Error: west not found!${NC}"
            exit 1
        fi
        
        if west flash --skip-rebuild; then
            echo -e "${GREEN}✓ Flash successful with West!${NC}"
        else
            echo -e "${RED}✗ West flash failed!${NC}"
            echo "Note: West may require nrfjprog. Try: ./scripts/flash.sh openocd"
            exit 1
        fi
        ;;
    
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
exit
EOF
        
        if JLinkExe -CommandFile build/flash.jlink; then
            echo -e "${GREEN}✓ Flash successful with JLink!${NC}"
        else
            echo -e "${RED}✗ JLink flash failed!${NC}"
            exit 1
        fi
        ;;
    
    *)
        echo -e "${RED}Unknown flasher: $FLASHER${NC}"
        echo "Usage: $0 [openocd|pyocd|west|jlink]"
        echo "  openocd - OpenOCD with J-Link (recommended)"
        echo "  pyocd   - pyOCD universal debugger"
        echo "  west    - Zephyr west tool (requires nrfjprog)"
        echo "  jlink   - SEGGER J-Link tools"
        exit 1
        ;;
esac

echo ""
echo -e "${GREEN}Flash completed successfully!${NC}"
echo -e "${BLUE}The LED should now be blinking at 3Hz${NC}"
echo ""
echo "To monitor serial output, run: ./scripts/monitor.sh"
