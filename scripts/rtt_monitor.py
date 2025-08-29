#!/usr/bin/env python3
"""
RTT Monitor for nRF52-DK using pyOCD or JLink
"""

import subprocess
import sys
import time
import os

def try_jlink_rtt():
    """Try to use JLinkRTTLogger"""
    try:
        # Start JLink GDB server with RTT
        print("Starting J-Link RTT logger...")
        cmd = ["JLinkRTTLogger", "-Device", "NRF52832_XXAA", "-If", "SWD", "-Speed", "4000", "-RTTChannel", "0"]
        proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.PIPE, text=True)
        
        # Read output
        while True:
            line = proc.stdout.readline()
            if line:
                print(line, end='')
            else:
                time.sleep(0.1)
                
    except FileNotFoundError:
        print("JLinkRTTLogger not found")
        return False
    except KeyboardInterrupt:
        proc.terminate()
        return True

def try_openocd_rtt():
    """Try OpenOCD RTT with telnet"""
    try:
        print("Trying OpenOCD RTT...")
        
        # Create OpenOCD config
        config = """
source [find interface/jlink.cfg]
transport select swd
source [find target/nrf52.cfg]

init
rtt setup 0x20000000 0x10000 "SEGGER RTT"
rtt start

# Keep running
reset run
"""
        
        with open("/tmp/rtt_config.cfg", "w") as f:
            f.write(config)
        
        # Start OpenOCD
        openocd = subprocess.Popen(
            ["openocd", "-f", "/tmp/rtt_config.cfg"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        time.sleep(2)  # Wait for OpenOCD to start
        
        # Connect via telnet to read RTT
        telnet = subprocess.Popen(
            ["telnet", "localhost", "4444"],
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Send RTT read commands
        telnet.stdin.write("rtt channels\n")
        telnet.stdin.write("rtt server start 9090 0\n")
        telnet.stdin.flush()
        
        time.sleep(1)
        
        # Now connect to RTT server
        print("Connecting to RTT output on port 9090...")
        nc = subprocess.Popen(
            ["nc", "localhost", "9090"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        # Read RTT output
        while True:
            line = nc.stdout.readline()
            if line:
                print(line, end='')
            else:
                time.sleep(0.1)
                
    except FileNotFoundError as e:
        print(f"Tool not found: {e}")
        return False
    except KeyboardInterrupt:
        openocd.terminate()
        telnet.terminate()
        nc.terminate()
        return True

def read_rtt_from_memory():
    """Direct memory read approach using GDB"""
    try:
        print("Attempting to read RTT buffer from memory using GDB...")
        
        # GDB commands to read RTT buffer
        gdb_commands = """
target remote localhost:2331
monitor reset
monitor go
set $rtt_cb = 0x20000000
x/256bx $rtt_cb
"""
        
        with open("/tmp/rtt_gdb.cmd", "w") as f:
            f.write(gdb_commands)
        
        # Start JLink GDB server
        gdb_server = subprocess.Popen(
            ["JLinkGDBServer", "-device", "NRF52832_XXAA", "-if", "SWD", "-speed", "4000"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE
        )
        
        time.sleep(2)
        
        # Run GDB
        gdb = subprocess.Popen(
            ["arm-none-eabi-gdb", "-batch", "-x", "/tmp/rtt_gdb.cmd"],
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True
        )
        
        output, error = gdb.communicate(timeout=5)
        print("GDB output:", output)
        
        gdb_server.terminate()
        return True
        
    except Exception as e:
        print(f"GDB approach failed: {e}")
        return False

if __name__ == "__main__":
    print("nRF52-DK RTT Monitor")
    print("===================")
    print("Attempting different RTT monitoring methods...")
    print()
    
    # Try different methods
    if not try_jlink_rtt():
        if not try_openocd_rtt():
            if not read_rtt_from_memory():
                print("\nNo RTT monitoring method succeeded.")
                print("Please ensure J-Link tools or OpenOCD are properly installed.")
                sys.exit(1)