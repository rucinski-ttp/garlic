#!/usr/bin/env python3
import os, sys
sys.path.insert(0, os.path.expanduser('~/pyhelpers'))
"""Quick test of DMA UART implementation"""
from serial_open import open_serial
import time

port = "/dev/ttyUSB0"
print(f"Testing DMA UART on {port}")
print("-" * 50)

ser = open_serial(baudrate=9600, timeout=1)
ser.reset_input_buffer()
ser.reset_output_buffer()

print("1. Waiting for any data for 3 seconds...")
start = time.time()
while time.time() - start < 3:
    if ser.in_waiting > 0:
        data = ser.read(ser.in_waiting)
        print(f"   Received: {data}")
        print(f"   Decoded: {data.decode('utf-8', errors='ignore')}")
    time.sleep(0.1)
    
print("\n2. Sending test message...")
ser.write(b"Hello DMA\r\n")
time.sleep(1)

print("3. Reading response...")
if ser.in_waiting > 0:
    data = ser.read(ser.in_waiting)
    print(f"   Received: {data}")
    print(f"   Decoded: {data.decode('utf-8', errors='ignore')}")
else:
    print("   No response")

ser.close()
print("\nDone!")
