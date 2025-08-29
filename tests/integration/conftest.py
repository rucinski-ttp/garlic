"""
Pytest configuration and fixtures for Garlic device testing.

Medical device grade testing infrastructure for serial communication.
"""

import pytest
import serial
import serial.tools.list_ports
import time
import logging
from typing import Optional, Generator
from dataclasses import dataclass

# Configure logging
logging.basicConfig(level=logging.INFO)
logger = logging.getLogger(__name__)


@dataclass
class GarlicDeviceConfig:
    """Configuration for Garlic device testing."""
    port: str
    baudrate: int = 115200
    timeout: float = 1.0
    write_timeout: float = 1.0
    inter_byte_timeout: float = 0.1
    
    
class SerialGarlicDevice:
    """
    Fixture class for communicating with the Garlic device.
    
    Provides reliable serial communication with the nRF52-DK board
    running the Garlic firmware.
    """
    
    def __init__(self, config: GarlicDeviceConfig):
        """Initialize the serial device handler."""
        self.config = config
        self.serial: Optional[serial.Serial] = None
        self._connect()
        
    def _connect(self) -> None:
        """Establish serial connection to the device."""
        try:
            self.serial = serial.Serial(
                port=self.config.port,
                baudrate=self.config.baudrate,
                timeout=self.config.timeout,
                write_timeout=self.config.write_timeout,
                inter_byte_timeout=self.config.inter_byte_timeout,
                bytesize=serial.EIGHTBITS,
                parity=serial.PARITY_NONE,
                stopbits=serial.STOPBITS_ONE,
                xonxoff=False,
                rtscts=False,
                dsrdtr=False
            )
            
            # Clear buffers
            self.serial.reset_input_buffer()
            self.serial.reset_output_buffer()
            # Give device a short settle time after port open to avoid DTR-induced races
            time.sleep(0.2)
            
            logger.info(f"Connected to Garlic device on {self.config.port}")
            
        except serial.SerialException as e:
            logger.error(f"Failed to connect to {self.config.port}: {e}")
            raise
            
    def disconnect(self) -> None:
        """Close the serial connection."""
        if self.serial and self.serial.is_open:
            self.serial.close()
            logger.info("Disconnected from Garlic device")
            
    def write(self, data: bytes) -> int:
        """
        Write data to the device.
        
        Args:
            data: Bytes to send
            
        Returns:
            Number of bytes written
        """
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
            
        bytes_written = self.serial.write(data)
        self.serial.flush()
        return bytes_written
        
    def read(self, size: int = 1) -> bytes:
        """
        Read data from the device.
        
        Args:
            size: Number of bytes to read
            
        Returns:
            Bytes read from device
        """
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
            
        return self.serial.read(size)
        
    def read_line(self) -> str:
        """
        Read a line from the device.
        
        Returns:
            Decoded string without line endings
        """
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
            
        line = self.serial.readline()
        return line.decode('utf-8', errors='ignore').rstrip('\r\n')
        
    def read_until(self, expected: bytes, timeout: Optional[float] = None) -> bytes:
        """
        Read until expected bytes are found.
        
        Args:
            expected: Bytes to look for
            timeout: Optional timeout override
            
        Returns:
            All bytes read including the expected bytes
        """
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
            
        old_timeout = self.serial.timeout
        if timeout is not None:
            self.serial.timeout = timeout
            
        try:
            data = self.serial.read_until(expected)
            return data
        finally:
            self.serial.timeout = old_timeout
            
    def flush_input(self) -> None:
        """Clear the input buffer."""
        if self.serial and self.serial.is_open:
            self.serial.reset_input_buffer()
            
    def flush_output(self) -> None:
        """Clear the output buffer."""
        if self.serial and self.serial.is_open:
            self.serial.reset_output_buffer()
            
    def wait_for_pattern(self, pattern: str, timeout: float = 5.0) -> bool:
        """
        Wait for a specific pattern in the serial output.
        
        Args:
            pattern: String pattern to search for
            timeout: Maximum time to wait in seconds
            
        Returns:
            True if pattern found, False if timeout
        """
        start_time = time.time()
        buffer = ""
        
        while (time.time() - start_time) < timeout:
            if self.serial.in_waiting > 0:
                data = self.serial.read(self.serial.in_waiting)
                buffer += data.decode('utf-8', errors='ignore')
                
                if pattern in buffer:
                    return True
                    
            time.sleep(0.01)  # Small delay to prevent CPU hogging
            
        return False
        
    def send_and_expect(self, command: bytes, expected: str, timeout: float = 2.0) -> bool:
        """
        Send a command and wait for expected response.
        
        Args:
            command: Command to send
            expected: Expected response pattern
            timeout: Maximum time to wait
            
        Returns:
            True if expected response received
        """
        self.flush_input()
        self.write(command)
        return self.wait_for_pattern(expected, timeout)
        
    def get_device_info(self) -> dict:
        """
        Get device information for test reporting.
        
        Returns:
            Dictionary with device details
        """
        info = {
            'port': self.config.port,
            'baudrate': self.config.baudrate,
            'is_open': self.serial.is_open if self.serial else False
        }
        
        # Try to get port details
        try:
            for port in serial.tools.list_ports.comports():
                if port.device == self.config.port:
                    info.update({
                        'description': port.description,
                        'hwid': port.hwid,
                        'manufacturer': port.manufacturer,
                        'product': port.product,
                        'serial_number': port.serial_number
                    })
                    break
        except Exception as e:
            logger.warning(f"Could not get port details: {e}")
            
        return info


def find_garlic_device() -> Optional[str]:
    """
    Auto-detect the Garlic device port.
    
    Returns:
        Port name if found, None otherwise
    """
    ports = serial.tools.list_ports.comports()
    
    # Prefer FTDI devices (external USB-UART) first
    for port in ports:
        if "FTDI" in str(port.manufacturer) or "TTL232R" in str(port.description):
            logger.info(f"Found FTDI serial device: {port.device} - {port.description}")
            return port.device

    # Then J-Link CDC ACM (onboard VCOM)
    for port in ports:
        if "SEGGER" in port.description or "J-Link" in port.description or "ACM" in port.device:
            logger.info(f"Found potential J-Link CDC device: {port.device} - {port.description}")
            return port.device

    # Fallback: any USB serial
    for port in ports:
        if "USB" in port.device or "tty" in port.device:
            logger.info(f"Found serial device fallback: {port.device} - {port.description}")
            return port.device
            
    return None


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
def serial_garlic_device(garlic_port: str) -> Generator[SerialGarlicDevice, None, None]:
    """
    Fixture providing a SerialGarlicDevice instance.
    
    Automatically connects and disconnects for each test.
    """
    config = GarlicDeviceConfig(port=garlic_port)
    device = SerialGarlicDevice(config)
    
    # Log device info
    info = device.get_device_info()
    logger.info(f"Test device info: {info}")
    
    yield device
    
    device.disconnect()


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


def pytest_configure(config):
    """Configure pytest with command line options."""
    pytest.garlic_port = config.getoption("--garlic-port")
    pytest.garlic_baudrate = config.getoption("--garlic-baudrate")
    pytest.run_hardware = config.getoption("--run-hardware")
    
    # Add markers
    config.addinivalue_line(
        "markers", "hardware: mark test as requiring hardware"
    )
    config.addinivalue_line(
        "markers", "slow: mark test as slow running"
    )

def pytest_collection_modifyitems(config, items):
    """Skip hardware tests unless --run-hardware is provided."""
    if config.getoption("--run-hardware"):
        return
    skip_hw = pytest.mark.skip(reason="hardware tests skipped (use --run-hardware)")
    for item in items:
        if "hardware" in item.keywords:
            item.add_marker(skip_hw)
