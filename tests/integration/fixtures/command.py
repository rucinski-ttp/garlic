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
