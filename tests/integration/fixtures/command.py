import struct
from transport import TransportCodec


def pack_request(cmd_id: int, payload: bytes) -> bytes:
    return struct.pack('<HH', cmd_id, len(payload)) + payload


def parse_response(data: bytes):
    if len(data) < 6:
        raise ValueError('short response header')
    cmd_id, status, length = struct.unpack_from('<HHH', data, 0)
    if len(data) < 6 + length:
        raise ValueError('short response payload')
    payload = data[6:6 + length]
    return cmd_id, status, payload


class CommandClient:
    def __init__(self, serial_dev):
        self.ser = serial_dev
        self.tx_session = 1
        self.t = TransportCodec()

    def _req(self, cmd_id: int, payload: bytes, timeout: float = 1.0):
        sess = self.tx_session
        self.tx_session = (self.tx_session + 1) & 0xFFFF
        msg = pack_request(cmd_id, payload)
        resp_msg = self.t.request(self.ser, sess, msg, timeout=timeout)
        return parse_response(resp_msg)

    def get_git_version(self, timeout: float = 1.0) -> str:
        cmd_id, status, payload = self._req(0x0001, b'', timeout)
        if status != 0:
            raise RuntimeError(f'GET_GIT_VERSION failed: {status}')
        return payload.decode('ascii', errors='ignore')

    def get_uptime_ms(self, timeout: float = 1.0) -> int:
        cmd_id, status, payload = self._req(0x0002, b'', timeout)
        if status != 0 or len(payload) != 8:
            raise RuntimeError(f'GET_UPTIME failed: {status}')
        lo, hi = struct.unpack('<II', payload)
        return (hi << 32) | lo

    def echo(self, payload: bytes, timeout: float = 1.0) -> bytes:
        cmd_id, status, data = self._req(0x0005, payload, timeout)
        if status != 0:
            raise RuntimeError(f'ECHO failed: {status}')
        return data

    def flash_read(self, addr: int, length: int, timeout: float = 1.0) -> bytes:
        if not (0 <= addr < (1<<32)):
            raise ValueError('addr out of range')
        if not (0 < length <= 256):
            raise ValueError('length out of range')
        payload = struct.pack('<IH', addr, length)
        cmd_id, status, data = self._req(0x0003, payload, timeout)
        if status != 0 or len(data) != length:
            raise RuntimeError(f'FLASH_READ failed: status={status}, len={len(data)}')
        return data

    def reboot(self, timeout: float = 0.3) -> None:
        # Request reboot; device should ack and reboot shortly after.
        self._req(0x0004, b'', timeout)
        # After this call, the device may reset; caller can verify via RTT.

    # --- Extended helpers for I2C and TMP119 bring-up ---
    def i2c_write(self, addr7: int, data: bytes, timeout: float = 1.0) -> None:
        if not (0 <= addr7 < 128):
            raise ValueError('addr7 out of range')
        payload = struct.pack('<BBHH', 0, addr7 & 0x7F, len(data), 0) + data
        _, status, _ = self._req(0x0100, payload, timeout)
        if status != 0:
            raise RuntimeError(f'I2C write failed: status={status}')

    def i2c_read(self, addr7: int, rlen: int, timeout: float = 1.0) -> bytes:
        if not (0 <= addr7 < 128) or rlen <= 0:
            raise ValueError('args out of range')
        payload = struct.pack('<BBHH', 1, addr7 & 0x7F, 0, rlen)
        _, status, data = self._req(0x0100, payload, timeout)
        if status != 0 or len(data) != rlen:
            raise RuntimeError(f'I2C read failed: status={status}, len={len(data)}')
        return data

    def i2c_write_read(self, addr7: int, wdata: bytes, rlen: int, timeout: float = 1.0) -> bytes:
        if not (0 <= addr7 < 128) or rlen <= 0:
            raise ValueError('args out of range')
        payload = struct.pack('<BBHH', 2, addr7 & 0x7F, len(wdata), rlen) + wdata
        _, status, data = self._req(0x0100, payload, timeout)
        if status != 0 or len(data) != rlen:
            raise RuntimeError(f'I2C write_read failed: status={status}, len={len(data)}')
        return data

    def i2c_scan(self, timeout: float = 1.0) -> list[int]:
        payload = struct.pack('<BB', 0x10, 0)  # op=0x10, addr ignored
        _, status, data = self._req(0x0100, payload, timeout)
        if status != 0 or len(data) == 0:
            raise RuntimeError(f'I2C scan failed: status={status}, len={len(data)}')
        count = data[0]
        addrs = list(data[1:1+count])
        return addrs

    def tmp119_read_id(self, addr7: int = 0x48, timeout: float = 1.0) -> int:
        _, status, data = self._req(0x0119, struct.pack('<BB', 0x00, addr7 & 0x7F), timeout)
        if status != 0 or len(data) != 2:
            raise RuntimeError(f'TMP119 READ_ID failed: status={status}')
        return struct.unpack('<H', data)[0]

    def tmp119_read_temp_mc(self, addr7: int = 0x48, timeout: float = 1.0) -> int:
        _, status, data = self._req(0x0119, struct.pack('<BB', 0x01, addr7 & 0x7F), timeout)
        if status != 0 or len(data) != 4:
            raise RuntimeError(f'TMP119 READ_TEMP_mC failed: status={status}')
        return struct.unpack('<i', data)[0]

    def tmp119_read_temp_raw(self, addr7: int = 0x48, timeout: float = 1.0) -> int:
        _, status, data = self._req(0x0119, struct.pack('<BB', 0x02, addr7 & 0x7F), timeout)
        if status != 0 or len(data) != 2:
            raise RuntimeError(f'TMP119 READ_TEMP_RAW failed: status={status}')
        return struct.unpack('<H', data)[0]

    def tmp119_read_config(self, addr7: int = 0x48, timeout: float = 1.0) -> int:
        _, status, data = self._req(0x0119, struct.pack('<BB', 0x03, addr7 & 0x7F), timeout)
        if status != 0 or len(data) != 2:
            raise RuntimeError(f'TMP119 READ_CONFIG failed: status={status}')
        return struct.unpack('<H', data)[0]

    def tmp119_write_config(self, cfg: int, addr7: int = 0x48, timeout: float = 1.0) -> None:
        payload = struct.pack('<BBH', 0x04, addr7 & 0x7F, cfg & 0xFFFF)
        _, status, _ = self._req(0x0119, payload, timeout)
        if status != 0:
            raise RuntimeError(f'TMP119 WRITE_CONFIG failed: status={status}')

    def tmp119_unlock_eeprom(self, addr7: int = 0x48, timeout: float = 1.0) -> None:
        _, status, _ = self._req(0x0119, struct.pack('<BB', 0x09, addr7 & 0x7F), timeout)
        if status != 0:
            raise RuntimeError(f'TMP119 UNLOCK_EEPROM failed: status={status}')

    def tmp119_read_eeprom(self, idx: int, addr7: int = 0x48, timeout: float = 1.0) -> int:
        _, status, data = self._req(0x0119, struct.pack('<BBB', 0x0A, addr7 & 0x7F, idx & 0xFF), timeout)
        if status != 0 or len(data) != 2:
            raise RuntimeError(f'TMP119 READ_EEPROM failed: status={status}')
        return struct.unpack('<H', data)[0]

    def tmp119_write_eeprom(self, idx: int, val: int, addr7: int = 0x48, timeout: float = 1.0) -> None:
        payload = struct.pack('<BBBH', 0x0B, addr7 & 0x7F, idx & 0xFF, val & 0xFFFF)
        _, status, _ = self._req(0x0119, payload, timeout)
        if status != 0:
            raise RuntimeError(f'TMP119 WRITE_EEPROM failed: status={status}')

    def tmp119_read_offset(self, addr7: int = 0x48, timeout: float = 1.0) -> int:
        _, status, data = self._req(0x0119, struct.pack('<BB', 0x0C, addr7 & 0x7F), timeout)
        if status != 0 or len(data) != 2:
            raise RuntimeError(f'TMP119 READ_OFFSET failed: status={status}')
        return struct.unpack('<H', data)[0]

    def tmp119_write_offset(self, val: int, addr7: int = 0x48, timeout: float = 1.0) -> None:
        payload = struct.pack('<BBH', 0x0D, addr7 & 0x7F, val & 0xFFFF)
        _, status, _ = self._req(0x0119, payload, timeout)
        if status != 0:
            raise RuntimeError(f'TMP119 WRITE_OFFSET failed: status={status}')

    # --- BLE control ---
    def ble_get_status(self, timeout: float = 1.0) -> tuple[bool, bool]:
        _, status, data = self._req(0x0200, struct.pack('<B', 0x00), timeout)
        if status != 0 or len(data) != 2:
            raise RuntimeError(f'BLE GET_STATUS failed: status={status}')
        return (data[0] != 0, data[1] != 0)

    def ble_set_advertising(self, enable: bool, timeout: float = 1.0) -> None:
        _, status, data = self._req(0x0200, struct.pack('<BB', 0x01, 1 if enable else 0), timeout)
        if status != 0:
            raise RuntimeError(f'BLE SET_ADV failed: status={status}')
