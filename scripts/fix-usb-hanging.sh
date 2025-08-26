#!/bin/bash

# Script to fix USB hanging issues with nRF52-DK
# Run with: sudo ./scripts/fix-usb-hanging.sh

echo "====================================="
echo "Fixing USB hanging issues for nRF52-DK"
echo "====================================="

# Check if running as root
if [ "$EUID" -ne 0 ]; then 
    echo "Please run with sudo: sudo $0"
    exit 1
fi

echo ""
echo "1. Stopping ModemManager..."
systemctl stop ModemManager 2>/dev/null
if [ $? -eq 0 ]; then
    echo "   ModemManager stopped"
    systemctl disable ModemManager
    echo "   ModemManager disabled"
else
    echo "   ModemManager not found or already stopped"
fi

echo ""
echo "2. Checking for brltty (braille display service)..."
if systemctl status brltty >/dev/null 2>&1; then
    echo "   Found brltty, stopping..."
    systemctl stop brltty
    systemctl disable brltty
    echo "   brltty stopped and disabled"
else
    echo "   brltty not found (good)"
fi

echo ""
echo "3. Creating udev rules to prevent auto-probing..."
cat > /etc/udev/rules.d/99-nrf52dk.rules << 'EOF'
# Nordic nRF52-DK / J-Link - prevent ModemManager from probing
# J-Link devices
ATTRS{idVendor}=="1366", ENV{ID_MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="1366", ENV{ID_MM_CANDIDATE}="0"
ATTRS{idVendor}=="1366", ENV{MTP_NO_PROBE}="1"

# Also add user to dialout group for serial access
SUBSYSTEM=="tty", ATTRS{idVendor}=="1366", MODE="0666", GROUP="dialout"
EOF
echo "   Created /etc/udev/rules.d/99-nrf52dk.rules"

echo ""
echo "4. Reloading udev rules..."
udevadm control --reload-rules
udevadm trigger
echo "   udev rules reloaded"

echo ""
echo "5. Adding current user to dialout group..."
ACTUAL_USER=$(who am i | awk '{print $1}')
usermod -a -G dialout $ACTUAL_USER 2>/dev/null
echo "   User $ACTUAL_USER added to dialout group"

echo ""
echo "====================================="
echo "✓ USB fixes applied!"
echo ""
echo "IMPORTANT:"
echo "1. Unplug and replug your nRF52-DK board"
echo "2. You may need to logout and login for group changes"
echo ""
echo "The terminal should no longer freeze when you plug in the board."
echo "====================================="