import time
import struct
import binascii


SYNC0 = 0xA5
SYNC1 = 0x5A
VERSION = 1
FLAG_START = 1 << 0
FLAG_MIDDLE = 1 << 1
FLAG_END = 1 << 2
FLAG_RESP = 1 << 4
MAX_PAYLOAD = 128


def _u16(x: int) -> bytes:
    return struct.pack('<H', x)


def _rd_u16(b: bytes, off: int) -> int:
    return struct.unpack_from('<H', b, off)[0]


def _crc32_ieee(data: bytes) -> int:
    # binascii.crc32 returns signed on some Pythons; mask to uint32
    return binascii.crc32(data) & 0xFFFFFFFF


class TransportCodec:
    def __init__(self):
        self._state = 'SYNC0'
        self._hdr = bytearray()
        self._payload = bytearray()
        self._crc = bytearray()
        self._re_buf = bytearray()
        self._re_session = 0
        self._re_frag_index = 0
        self._re_frag_count = 0
        self._re_is_resp = False

    def encode_message(self, session: int, payload: bytes, is_response: bool) -> bytes:
        out = bytearray()
        if payload is None:
            payload = b''
        total = len(payload)
        frag_count = max(1, (total + MAX_PAYLOAD - 1) // MAX_PAYLOAD)
        for frag_index in range(frag_count):
            start = frag_index * MAX_PAYLOAD
            frag = payload[start:start + MAX_PAYLOAD]
            flags = 0
            if frag_index == 0:
                flags |= FLAG_START
            if frag_index == frag_count - 1:
                flags |= FLAG_END
            else:
                flags |= FLAG_MIDDLE
            if is_response:
                flags |= FLAG_RESP
            hdr = bytes([
                SYNC0, SYNC1,
                VERSION,
                flags,
            ]) + _u16(session) + _u16(frag_index) + _u16(frag_count) + _u16(len(frag))
            crc = _crc32_ieee(hdr[2:] + frag)
            out += hdr + frag + struct.pack('<I', crc)
        return bytes(out)

    def feed(self, data: bytes):
        messages = []
        for b in data:
            if self._state == 'SYNC0':
                if b == SYNC0:
                    self._state = 'SYNC1'
                continue
            if self._state == 'SYNC1':
                if b == SYNC1:
                    self._state = 'HDR'
                    self._hdr.clear()
                else:
                    self._state = 'SYNC0'
                continue
            if self._state == 'HDR':
                self._hdr.append(b)
                if len(self._hdr) == 10:
                    pay_len = _rd_u16(self._hdr, 8)
                    if pay_len > MAX_PAYLOAD:
                        # resync
                        self._state = 'SYNC0'
                        continue
                    self._payload.clear()
                    self._state = 'PAY'
                continue
            if self._state == 'PAY':
                self._payload.append(b)
                if len(self._payload) == _rd_u16(self._hdr, 8):
                    self._crc.clear()
                    self._state = 'CRC'
                continue
            if self._state == 'CRC':
                self._crc.append(b)
                if len(self._crc) == 4:
                    # verify
                    calc = _crc32_ieee(bytes(self._hdr) + bytes(self._payload))
                    got = struct.unpack('<I', bytes(self._crc))[0]
                    if calc == got:
                        ver = self._hdr[0]
                        flags = self._hdr[1]
                        session = _rd_u16(self._hdr, 2)
                        frag_index = _rd_u16(self._hdr, 4)
                        frag_count = _rd_u16(self._hdr, 6)
                        if ver == VERSION and 1 <= frag_count <= 64:
                            if flags & FLAG_START:
                                self._re_buf.clear()
                                self._re_session = session
                                self._re_frag_index = 0
                                self._re_frag_count = frag_count
                                self._re_is_resp = (flags & FLAG_RESP) != 0
                            # order check
                            if session == self._re_session and frag_index == self._re_frag_index:
                                self._re_buf += self._payload
                                self._re_frag_index += 1
                                if flags & FLAG_END:
                                    messages.append((self._re_session, bytes(self._re_buf), self._re_is_resp))
                                    self._re_buf.clear()
                    # reset parser
                    self._state = 'SYNC0'
        return messages

    def request(self, ser, session: int, payload: bytes, timeout: float = 1.0):
        ser.write(self.encode_message(session, payload, is_response=False))
        deadline = time.time() + timeout
        while time.time() < deadline:
            data = ser.read(ser.serial.in_waiting or 1)
            if data:
                for sess, msg, is_resp in self.feed(data):
                    if is_resp and sess == session:
                        return msg
        raise TimeoutError('No response message')

