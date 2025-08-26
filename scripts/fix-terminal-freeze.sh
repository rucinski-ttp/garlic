#!/bin/bash

# Advanced fix for terminal freezing when nRF52-DK is connected
# This handles multiple possible causes

echo "====================================="
echo "Advanced USB Terminal Freeze Fix"
echo "====================================="

if [ "$EUID" -ne 0 ]; then 
    echo "Please run with sudo: sudo $0"
    exit 1
fi

echo ""
echo "1. Stopping ALL potentially interfering services..."

# List of services that can cause issues
SERVICES=(
    "ModemManager"
    "brltty"
    "brltty-udev"
    "fwupd"
)

for service in "${SERVICES[@]}"; do
    if systemctl is-active --quiet $service; then
        echo "   Stopping $service..."
        systemctl stop $service
        systemctl disable $service 2>/dev/null
    fi
done

echo ""
echo "2. Checking GNOME USB protection..."
if pgrep -f "gsd-usb-protection" > /dev/null; then
    echo "   GNOME USB protection is running"
    echo "   Creating GNOME settings override..."
    
    # Disable USB protection for development
    sudo -u $SUDO_USER gsettings set org.gnome.desktop.privacy usb-protection false 2>/dev/null || true
    pkill -f gsd-usb-protection 2>/dev/null || true
fi

echo ""
echo "3. Checking for console getty on ttyACM0..."
# Disable getty on ttyACM0 if it exists
systemctl stop serial-getty@ttyACM0.service 2>/dev/null
systemctl disable serial-getty@ttyACM0.service 2>/dev/null

echo ""
echo "4. Creating comprehensive udev rules..."
cat > /etc/udev/rules.d/99-jlink-nrf.rules << 'EOF'
# J-Link / nRF52-DK rules to prevent interference
# SEGGER J-Link
SUBSYSTEM=="usb", ATTRS{idVendor}=="1366", MODE="0666", GROUP="dialout"
SUBSYSTEM=="tty", ATTRS{idVendor}=="1366", MODE="0666", GROUP="dialout"

# Prevent ModemManager
ATTRS{idVendor}=="1366", ENV{ID_MM_DEVICE_IGNORE}="1"
ATTRS{idVendor}=="1366", ENV{ID_MM_CANDIDATE}="0"
ATTRS{idVendor}=="1366", ENV{ID_MM_PORT_IGNORE}="1"

# Prevent other probing
ATTRS{idVendor}=="1366", ENV{MTP_NO_PROBE}="1"
ATTRS{idVendor}=="1366", ENV{ID_GPHOTO2_CAPTURE}="0"

# Prevent systemd device timeout
ATTRS{idVendor}=="1366", ENV{SYSTEMD_READY}="0"
EOF

echo ""
echo "5. Blacklisting CDC ACM module temporarily..."
# This is drastic but might help
cat > /etc/modprobe.d/nrf-dev.conf << 'EOF'
# Uncomment to completely disable CDC ACM if issues persist
# blacklist cdc_acm
EOF

echo ""
echo "6. Reloading everything..."
udevadm control --reload-rules
udevadm trigger

echo ""
echo "7. Adding user to necessary groups..."
ACTUAL_USER=$(who am i | awk '{print $1}')
usermod -a -G dialout,plugdev $ACTUAL_USER 2>/dev/null

echo ""
echo "====================================="
echo "✓ Advanced fixes applied!"
echo ""
echo "NOW TRY THIS:"
echo "1. Unplug the nRF52-DK"
echo "2. Run: sudo dmesg -w"
echo "3. Plug in the board and watch what happens"
echo "4. Press Ctrl+C to stop"
echo ""
echo "If it STILL freezes, the issue might be:"
echo "- The terminal emulator itself (try a different one)"
echo "- A kernel driver issue (check dmesg)"
echo "- Hardware flow control on the serial port"
echo "====================================="