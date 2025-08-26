#!/bin/bash

# Flash script for nRF52-DK board
# Usage: ./scripts/flash.sh [pyocd|openocd|west]

set -e

# Source environment
source "$(dirname "$0")/source.sh"

# Default flasher
FLASHER=${1:-pyocd}

# Change to app directory
cd /projects/garlic/app

echo "Flashing nRF52-DK using $FLASHER..."

# Kill any existing debug sessions to avoid USB hangs
pkill -f pyocd 2>/dev/null || true
pkill -f openocd 2>/dev/null || true
sleep 1

case $FLASHER in
    pyocd)
        echo "Using pyOCD..."
        # Use timeout to prevent hanging
        timeout 30 pyocd flash -t nrf52832 build/zephyr/zephyr.hex
        ;;
    
    openocd)
        echo "Using OpenOCD..."
        openocd -f interface/jlink.cfg \
                -c "transport select swd" \
                -f target/nrf52.cfg \
                -c "program build/zephyr/zephyr.hex verify reset exit"
        ;;
    
    west)
        echo "Using West flash..."
        west flash --skip-rebuild
        ;;
    
    *)
        echo "Unknown flasher: $FLASHER"
        echo "Usage: $0 [pyocd|openocd|west]"
        exit 1
        ;;
esac

echo "Flash completed!"