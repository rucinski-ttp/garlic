#!/bin/bash

# Quick flash script that minimizes connection time to avoid freezing
# Usage: ./scripts/flash-quick.sh

set -e

source "$(dirname "$0")/source.sh"

echo "====================================="
echo "Quick Flash (Anti-Freeze Method)"
echo "====================================="
echo ""
echo "This script minimizes USB connection time to avoid freezing."
echo ""

# Change to app directory
cd /projects/garlic/app

# Check hex file exists
if [ ! -f "build/zephyr/zephyr.hex" ]; then
    echo "ERROR: No hex file found. Run ./scripts/build.sh first!"
    exit 1
fi

echo "Ready to flash build/zephyr/zephyr.hex"
echo ""
echo "INSTRUCTIONS:"
echo "1. Make sure the nRF52-DK is UNPLUGGED"
echo "2. Press ENTER when ready"
echo "3. When prompted, QUICKLY plug in the board"
echo "4. The script will flash and immediately disconnect"
echo ""
read -p "Press ENTER when ready (board should be unplugged)..."

echo ""
echo "Starting flash process..."
echo ">>> PLUG IN THE BOARD NOW! <<<"
echo ""

# Try to flash with very short timeout
# Using pyocd with immediate flash and exit
timeout 10 pyocd flash -t nrf52832 build/zephyr/zephyr.hex --frequency 4000000 || {
    echo ""
    echo "Flash may have completed even if timeout occurred."
    echo "The LED should be blinking at 3Hz if successful."
}

echo ""
echo "====================================="
echo "Flash attempt complete!"
echo ""
echo "If the LED is blinking 3 times per second, it worked!"
echo "You can now unplug the board."
echo "====================================="