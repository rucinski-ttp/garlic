#!/usr/bin/env python3
import os, sys
sys.path.insert(0, os.path.expanduser('~/pyhelpers'))
from serial_open import open_serial
import time

port = "/dev/ttyUSB0"
print(f"Testing simple UART on {port}")

ser = open_serial(baudrate=9600, timeout=1)
ser.reset_input_buffer()

print("Waiting for startup message...")
time.sleep(1)

if ser.in_waiting > 0:
    data = ser.read(ser.in_waiting)
    print(f"Startup: {data}")
    if b"UART_TEST_READY" in data:
        print("✓ Got startup message!")
else:
    print("No startup message")

print("\nTesting echo...")
test_bytes = [0x41, 0x42, 0x43, 0x0A]  # ABC\n

for b in test_bytes:
    ser.write(bytes([b]))
    time.sleep(0.05)
    if ser.in_waiting > 0:
        echo = ser.read(1)
        print(f"Sent: 0x{b:02x}, Got: 0x{echo[0]:02x}")
    else:
        print(f"Sent: 0x{b:02x}, No echo")

ser.close()