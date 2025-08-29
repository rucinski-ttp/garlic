"""
Pytest configuration and fixtures for Garlic device testing.

Medical device grade testing infrastructure for serial communication.
"""

import pytest
import os
import time
from pathlib import Path
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
    
    # Add markers
    config.addinivalue_line(
        "markers", "hardware: mark test as requiring hardware"
    )
    config.addinivalue_line(
        "markers", "slow: mark test as slow running"
    )

def _which(name: str):
    from shutil import which
    return which(name)

class RTTCapture:
    def __init__(self, outfile: str, tool: str | None, channel: int):
        self.outfile = outfile
        self.tool = tool or "auto"
        self.channel = channel
        self.proc = None

    def start(self):
        import sys, subprocess, time
        script = Path("scripts") / "rtt_capture.py"
        if not script.exists():
            raise RuntimeError("scripts/rtt_capture.py not found")
        # Prefer a concrete backend using J-Link tools
        tool = self.tool
        if tool in (None, "auto"):
            if _which("JLinkGDBServer"):
                tool = "jlink"
            elif _which("JLinkRTTLogger"):
                tool = "jlink-logger"
            else:
                tool = "auto"
        self.tool = tool

        cmd = [sys.executable or "python3", str(script), "--tool", tool, "--outfile", self.outfile]
        if self.channel is not None:
            cmd += ["--channel", str(self.channel)]
        self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
        time.sleep(0.8)  # Allow backend to attach
        # If the requested backend failed to start immediately (common with OpenOCD on some hosts),
        # fall back to J-Link backends automatically for robustness.
        rc = self.proc.poll()
        if rc is not None and rc != 0:
            # Only auto-fallback when tool was 'openocd' or 'auto'
            if tool in ("openocd", "auto"):
                # Try J-Link GDB server + RTT client first
                if _which("JLinkGDBServer"):
                    self.tool = "jlink"
                    cmd = [sys.executable or "python3", str(script), "--tool", self.tool, "--outfile", self.outfile]
                    if self.channel is not None:
                        cmd += ["--channel", str(self.channel)]
                    self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                    time.sleep(0.8)
                    return
                # Then J-Link RTT Logger
                if _which("JLinkRTTLogger"):
                    self.tool = "jlink-logger"
                    cmd = [sys.executable or "python3", str(script), "--tool", self.tool, "--outfile", self.outfile]
                    if self.channel is not None:
                        cmd += ["--channel", str(self.channel)]
                    self.proc = subprocess.Popen(cmd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True)
                    time.sleep(0.8)
                    return

    def stop(self):
        import time
        if self.proc is not None:
            try:
                self.proc.terminate()
                self.proc.wait(timeout=1.0)
            except Exception:
                pass

    def reset_board(self):
        import subprocess, os, socket, time
        # If using J-Link GDB backend, issue reset via GDB (to the running GDB server)
        if self.tool == "jlink" and _which("JLinkGDBServer"):
            # Try to find a suitable gdb
            def _find_gdb():
                for name in ("arm-none-eabi-gdb", "arm-zephyr-eabi-gdb", "gdb-multiarch"):
                    if _which(name):
                        return name
                return None
            gdb = _find_gdb()
            if gdb:
                # Wait briefly for JLinkGDBServer to accept connections on :2331
                deadline = time.time() + 3.0
                ready = False
                while time.time() < deadline:
                    try:
                        with socket.create_connection(("127.0.0.1", 2331), timeout=0.3):
                            ready = True
                            break
                    except Exception:
                        time.sleep(0.1)
                if ready:
                    # Fast batch reset via monitor commands (quiet)
                    cmd = [gdb, "-q", "-batch", "-ex", "set confirm off", "-ex", "set pagination off",
                           "-ex", "target remote :2331", "-ex", "monitor reset", "-ex", "monitor go", "-ex", "quit"]
                    subprocess.run(cmd, check=False, stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL)
                    return
                # If GDB server not ready, fall through to discrete reset methods
        # Otherwise fall back to discrete reset via J-Link
        if _which("JLinkExe"):
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\nr\ng\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)
            return

    def expect_in_log(self, pattern: str, timeout: float = 5.0) -> bool:
        import time
        deadline = time.time() + timeout
        while time.time() < deadline:
            if os.path.exists(self.outfile):
                try:
                    text = Path(self.outfile).read_text(errors="ignore")
                    if pattern in text:
                        return True
                except Exception:
                    pass
            time.sleep(0.2)
        return False

    def read(self) -> str:
        try:
            return Path(self.outfile).read_text(errors="ignore")
        except Exception:
            return ""

@pytest.fixture(scope="function")
def rtt_capture(tmp_path):
    if not pytest.run_hardware:
        pytest.skip("hardware tests skipped (use --run-hardware)")

    # Ensure we have some backend available
    if not (_which("JLinkRTTLogger") or _which("JLinkGDBServer") or _which("openocd")):
        pytest.skip("No RTT tool available (J-Link or OpenOCD)")

    logs_dir = Path("logs")
    logs_dir.mkdir(exist_ok=True)
    outfile = logs_dir / f"rtt_{int(time.time())}.log"
    cap = RTTCapture(str(outfile), pytest.garlic_rtt_tool, pytest.garlic_rtt_channel)
    cap.start()
    try:
        yield cap
    finally:
        cap.stop()


class DebugProbe:
    def __init__(self):
        self.backend = None
        if _which("JLinkExe"):
            self.backend = "jlink"
        else:
            self.backend = None

    def available(self) -> bool:
        return self.backend is not None

    def reset_run(self):
        import subprocess, os
        if self.backend == "jlink":
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\nr\ng\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)

    def reset_halt(self):
        import subprocess, os
        if self.backend == "jlink":
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\nr\nh\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)

    def go(self):
        import subprocess, os
        if self.backend == "jlink":
            script = "device NRF52832_XXAA\nif SWD\nspeed 4000\nconnect\ng\nq\n"
            subprocess.run(["JLinkExe"], input=script.encode(), check=False)


@pytest.fixture(scope="function")
def debug_probe():
    if not pytest.run_hardware:
        pytest.skip("hardware tests skipped (use --run-hardware)")
    probe = DebugProbe()
    if not probe.available():
        pytest.skip("No debug probe available (need JLinkExe or OpenOCD)")
    return probe

def pytest_collection_modifyitems(config, items):
    """Skip hardware tests unless --run-hardware is provided."""
    if config.getoption("--run-hardware"):
        return
    skip_hw = pytest.mark.skip(reason="hardware tests skipped (use --run-hardware)")
    for item in items:
        if "hardware" in item.keywords:
            item.add_marker(skip_hw)
