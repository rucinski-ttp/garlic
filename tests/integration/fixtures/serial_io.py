import time
from dataclasses import dataclass
from typing import Optional
import logging
import serial
import serial.tools.list_ports

logger = logging.getLogger(__name__)


@dataclass
class GarlicDeviceConfig:
    port: str
    baudrate: int = 115200
    timeout: float = 1.0
    write_timeout: float = 1.0
    inter_byte_timeout: float = 0.1


class SerialGarlicDevice:
    def __init__(self, config: GarlicDeviceConfig):
        self.config = config
        self.serial: Optional[serial.Serial] = None
        self._connect()

    def _connect(self) -> None:
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
            dsrdtr=False,
        )
        self.serial.reset_input_buffer()
        self.serial.reset_output_buffer()
        time.sleep(0.2)
        logger.info(f"Connected to Garlic device on {self.config.port}")

    def disconnect(self) -> None:
        if self.serial and self.serial.is_open:
            self.serial.close()
            logger.info("Disconnected from Garlic device")

    def write(self, data: bytes) -> int:
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
        n = self.serial.write(data)
        self.serial.flush()
        return n

    def read(self, size: int = 1) -> bytes:
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
        return self.serial.read(size)

    def read_line(self) -> str:
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
        line = self.serial.readline()
        return line.decode("utf-8", errors="ignore").rstrip("\r\n")

    def read_until(self, expected: bytes, timeout: Optional[float] = None) -> bytes:
        if not self.serial or not self.serial.is_open:
            raise RuntimeError("Serial connection not open")
        old = self.serial.timeout
        if timeout is not None:
            self.serial.timeout = timeout
        try:
            return self.serial.read_until(expected)
        finally:
            self.serial.timeout = old

    def flush_input(self) -> None:
        if self.serial and self.serial.is_open:
            self.serial.reset_input_buffer()

    def flush_output(self) -> None:
        if self.serial and self.serial.is_open:
            self.serial.reset_output_buffer()

    def get_device_info(self) -> dict:
        info = {
            "port": self.config.port,
            "baudrate": self.config.baudrate,
            "is_open": self.serial.is_open if self.serial else False,
        }
        try:
            for port in serial.tools.list_ports.comports():
                if port.device == self.config.port:
                    info.update(
                        {
                            "description": port.description,
                            "hwid": port.hwid,
                            "manufacturer": port.manufacturer,
                            "product": port.product,
                            "serial_number": port.serial_number,
                        }
                    )
                    break
        except Exception as e:
            logger.warning(f"Could not get port details: {e}")
        return info


def find_garlic_device() -> Optional[str]:
    ports = list(serial.tools.list_ports.comports())
    # Prefer FTDI TTL adapters first to avoid nRF52-DK VCOM contention on P0.06/P0.08
    for port in ports:
        if "FTDI" in str(port.manufacturer) or "TTL232R" in str(port.description):
            logger.info(f"Found FTDI serial device: {port.device} - {port.description}")
            return port.device
    # Then consider J-Link CDC (onboard USB bridge)
    for port in ports:
        if "SEGGER" in port.description or "J-Link" in port.description or "ACM" in port.device:
            logger.info(f"Found potential J-Link CDC device: {port.device} - {port.description}")
            return port.device
    # Fallback: any tty/USB device
    for port in ports:
        if "USB" in port.device or "tty" in port.device:
            logger.info(f"Found serial device fallback: {port.device} - {port.description}")
            return port.device
    return None
