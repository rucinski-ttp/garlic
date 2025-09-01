"""
Top-level pytest configuration for integration tests.

Fixtures are defined under tests/integration/fixtures.
This file only wires options and imports the shared fixtures.
"""

import pytest
import logging
from pathlib import Path
import os
import sys
from typing import Generator

# Ensure we can import fixtures as a top-level module
_FIXTURES_DIR = Path(__file__).resolve().parent / "fixtures"
if str(_FIXTURES_DIR) not in sys.path:
    sys.path.insert(0, str(_FIXTURES_DIR))

from serial_io import SerialGarlicDevice, GarlicDeviceConfig, find_garlic_device
from ble_io import BLEGarlicDevice, BLEDeviceConfig
from fixtures.rtt import RTTCapture
from fixtures.probe import DebugProbe

# Reduce log noise during normal runs; show details in failures
logging.basicConfig(level=logging.WARNING)
logger = logging.getLogger(__name__)

@pytest.fixture(scope="session")
def garlic_port() -> str:
    """
    Fixture to get the serial port for the Garlic device.
    
    Can be overridden with --garlic-port command line option.
    """
    # This will be set by command line option or auto-detected
    port = pytest.garlic_port if hasattr(pytest, 'garlic_port') else None
    
    if not port:
        port = find_garlic_device()
        
    if not port:
        pytest.skip("No Garlic device found. Connect device or specify --garlic-port")
        
    return port


@pytest.fixture(scope="function")
def garlic_device() -> Generator[object, None, None]:
    """Interface-agnostic Garlic device fixture (serial or BLE).

    Select with --interface=serial|ble (default: serial).
    """
    interface = getattr(pytest, 'garlic_interface', 'serial')
    dev = None
    if interface == 'serial':
        port = pytest.garlic_port if hasattr(pytest, 'garlic_port') else None
        if not port:
            port = find_garlic_device()
        if not port:
            pytest.skip("No Garlic serial device found; specify --garlic-port")
        cfg = GarlicDeviceConfig(port=port, baudrate=getattr(pytest, 'garlic_baudrate', 115200))
        dev = SerialGarlicDevice(cfg)
    elif interface == 'ble':
        name = getattr(pytest, 'garlic_ble_name', 'GarlicDK')
        addr = getattr(pytest, 'garlic_ble_address', None) if hasattr(pytest, 'garlic_ble_address') else None
        cfg = BLEDeviceConfig(name=name, address=addr)
        try:
            dev = BLEGarlicDevice(cfg)
        except Exception as e:
            pytest.skip(f"BLE device not available: {e}")
    else:
        raise RuntimeError(f"Unknown interface: {interface}")

    try:
        yield dev
    finally:
        try:
            dev.disconnect()
        except Exception:
            pass


def pytest_addoption(parser):
    """Add command line options for pytest."""
    parser.addoption(
        "--garlic-port",
        action="store",
        default=None,
        help="Serial port for Garlic device (e.g., /dev/ttyACM0)"
    )
    parser.addoption(
        "--garlic-baudrate",
        action="store",
        default=115200,
        type=int,
        help="Baudrate for serial communication"
    )
    parser.addoption(
        "--run-hardware",
        action="store_true",
        default=False,
        help="Enable running hardware tests (otherwise they are skipped)",
    )
    parser.addoption(
        "--interface",
        action="store",
        default="serial",
        choices=["serial", "ble"],
        help="Device interface to use for tests",
    )
    parser.addoption(
        "--garlic-ble-name",
        action="store",
        default="GarlicDK",
        help="BLE advertised name prefix to match (NUS)",
    )
    parser.addoption(
        "--garlic-ble-address",
        action="store",
        default=None,
        help="BLE MAC address to connect to (skips scan)",
    )

    # RTT capture options
    parser.addoption(
        "--garlic-rtt-tool",
        action="store",
        default=None,
        help="RTT capture backend: auto|jlink-logger|jlink (default: auto)",
    )
    parser.addoption(
        "--garlic-rtt-channel",
        action="store",
        default="0",
        help="RTT up-channel index (default: 0)",
    )


def pytest_configure(config):
    """Configure pytest with command line options."""
    pytest.garlic_port = config.getoption("--garlic-port")
    pytest.garlic_baudrate = config.getoption("--garlic-baudrate")
    pytest.run_hardware = config.getoption("--run-hardware")
    pytest.garlic_rtt_tool = config.getoption("--garlic-rtt-tool")
    pytest.garlic_rtt_channel = int(config.getoption("--garlic-rtt-channel"))
    pytest.garlic_interface = config.getoption("--interface")
    pytest.garlic_ble_address = config.getoption("--garlic-ble-address")
    pytest.garlic_ble_name = config.getoption("--garlic-ble-name")
    
    # Add markers
    config.addinivalue_line(
        "markers", "hardware: mark test as requiring hardware"
    )
    config.addinivalue_line(
        "markers", "slow: mark test as slow running"
    )

@pytest.fixture(scope="function")
def rtt_capture(tmp_path):
    if not pytest.run_hardware:
        pytest.skip("hardware tests skipped (use --run-hardware)")

    # Ensure we have some backend available
    from shutil import which as _which
    if not (_which("JLinkRTTLogger") or _which("JLinkGDBServer")):
        pytest.skip("No RTT tool available (J-Link)")

    logs_dir = Path("logs")
    logs_dir.mkdir(exist_ok=True)
    import time
    outfile = logs_dir / f"rtt_{int(time.time())}.log"
    cap = RTTCapture(str(outfile), pytest.garlic_rtt_tool, pytest.garlic_rtt_channel)
    cap.start()
    try:
        yield cap
    finally:
        cap.stop()


@pytest.fixture(scope="function")
def debug_probe():
    if not pytest.run_hardware:
        pytest.skip("hardware tests skipped (use --run-hardware)")
    probe = DebugProbe()
    if not probe.available():
        pytest.skip("No debug probe available (need JLinkExe)")
    return probe

def pytest_collection_modifyitems(config, items):
    """Skip hardware tests unless --run-hardware is provided."""
    if config.getoption("--run-hardware"):
        return
    skip_hw = pytest.mark.skip(reason="hardware tests skipped (use --run-hardware)")
    for item in items:
        if "hardware" in item.keywords:
            item.add_marker(skip_hw)
