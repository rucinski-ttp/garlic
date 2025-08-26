# Troubleshooting Guide

## USB Connection Issues

### Problem: System Hangs When Board Connected
**Symptoms**:
- Commands freeze when board is plugged in
- Serial port becomes unresponsive
- Flash operations timeout

**Root Cause**:
The J-Link debugger on the nRF52-DK can keep USB connections open, causing:
- File descriptor exhaustion
- USB driver conflicts
- Serial port blocking

**Solutions**:

1. **Quick Fix - Unplug/Replug**:
   ```bash
   # Before operations:
   # 1. Unplug USB cable
   # 2. Run command
   # 3. Plug in when prompted
   ```

2. **Reset USB Subsystem**:
   ```bash
   # Reset specific USB device (requires sudo)
   sudo usbreset 1366:0101  # J-Link VID:PID
   ```

3. **Kill Hanging Processes**:
   ```bash
   # Find and kill pyocd/openocd processes
   pkill -f pyocd
   pkill -f openocd
   ```

4. **Disable Auto-Connect**:
   Add to flash scripts:
   ```bash
   # Disable ModemManager interference
   sudo systemctl stop ModemManager
   ```

### Problem: Flash Operations Timeout

**Solutions**:
1. Use explicit timeouts in scripts
2. Try different flashing tools (pyocd vs openocd)
3. Reset board before flashing (press RESET button)

## Serial Console Issues

### Problem: Cannot Connect to Serial Console
**Check**:
```bash
ls -la /dev/ttyACM*
# Should show /dev/ttyACM0 with proper permissions
```

**Fix Permissions**:
```bash
sudo usermod -a -G dialout $USER
# Then logout and login again
```

**Connect**:
```bash
# Using screen
screen /dev/ttyACM0 115200

# Using minicom
minicom -D /dev/ttyACM0 -b 115200
```

## Build Issues

### Problem: West Update Fails
**Solution**:
```bash
# Clear west cache
rm -rf ~/.west
rm -rf .west

# Reinitialize
west init -l app
west update --narrow
```

### Problem: CMake Configuration Fails
**Solution**:
```bash
# Clean build directory
rm -rf app/build

# Source environment
source scripts/source.sh

# Rebuild
cd app
west build -b nrf52dk/nrf52832 --pristine
```