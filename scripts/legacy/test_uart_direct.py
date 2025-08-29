#!/usr/bin/env python3
"""
Direct UART test for nRF52-DK
Tests communication with the simple polling implementation
"""

import os
import sys
import time
# Add pyhelpers to path
sys.path.insert(0, os.path.expanduser("~/pyhelpers"))
from serial_open import open_serial

def test_uart():
    # Use the virtual PTY through our helper
    print(f"Opening serial port via bridge...")
    try:
        ser = open_serial(baudrate=9600, timeout=1)
        print(f"Connected to: {ser.port}")
    except Exception as e:
        print(f"Failed to open serial port: {e}")
        print("Make sure the bridge is running: ~/cc-serial-bridge.sh /dev/ttyUSB0")
        return False
    
    time.sleep(0.5)  # Let port settle
    
    # Clear any pending data
    ser.reset_input_buffer()
    ser.reset_output_buffer()
    
    print("\n=== UART Test Starting ===")
    print("Listening for Counter messages...")
    print("Press Ctrl+C to exit\n")
    
    try:
        # Read incoming data
        start_time = time.time()
        rx_count = 0
        
        while True:
            # Check for incoming data
            if ser.in_waiting > 0:
                data = ser.read(ser.in_waiting)
                rx_count += len(data)
                print(f"RX ({len(data)} bytes): {data.decode('utf-8', errors='replace')}", end='')
                
            # Send a test message every 3 seconds
            if time.time() - start_time > 3:
                test_msg = f"Test from Python at {time.strftime('%H:%M:%S')}\r\n"
                print(f"\nTX: {test_msg.strip()}")
                ser.write(test_msg.encode())
                start_time = time.time()
                
            time.sleep(0.01)
            
    except KeyboardInterrupt:
        print(f"\n\nTest stopped. Received {rx_count} total bytes")
    finally:
        ser.close()
    
    return rx_count > 0

if __name__ == "__main__":
    success = test_uart()
    sys.exit(0 if success else 1)
