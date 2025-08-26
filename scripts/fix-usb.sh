#!/bin/bash

# Script to fix USB hanging issues with nRF52-DK

echo "Fixing USB issues..."

# 1. Stop ModemManager if it's running (common culprit)
if systemctl is-active ModemManager >/dev/null 2>&1; then
    echo "Stopping ModemManager (may require sudo)..."
    sudo systemctl stop ModemManager
    echo "ModemManager stopped. To disable permanently: sudo systemctl disable ModemManager"
fi

# 2. Kill any processes using the serial port
if [ -e /dev/ttyACM0 ]; then
    echo "Checking for processes using /dev/ttyACM0..."
    fuser /dev/ttyACM0 2>/dev/null && {
        echo "Killing processes using serial port..."
        fuser -k /dev/ttyACM0
    }
fi

# 3. Kill debug tools
echo "Killing any debug tool processes..."
pkill -f pyocd 2>/dev/null || true
pkill -f openocd 2>/dev/null || true
pkill -f JLinkExe 2>/dev/null || true

# 4. Reset USB device if possible
if command -v usbreset >/dev/null 2>&1; then
    echo "Resetting J-Link USB device..."
    # Find J-Link device
    JLINK=$(lsusb | grep -E "SEGGER|J-Link" | awk '{print $6}')
    if [ ! -z "$JLINK" ]; then
        sudo usbreset $JLINK 2>/dev/null || echo "Could not reset USB device"
    fi
fi

echo "USB fixes applied. Try unplugging and replugging the board."