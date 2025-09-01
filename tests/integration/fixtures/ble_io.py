import asyncio
import time
import threading
from dataclasses import dataclass
from typing import Optional
import logging

logger = logging.getLogger(__name__)


NUS_SERVICE_UUID = "6E400001-B5A3-F393-E0A9-E50E24DCCA9E"
NUS_RX_UUID = "6E400002-B5A3-F393-E0A9-E50E24DCCA9E"  # Write
NUS_TX_UUID = "6E400003-B5A3-F393-E0A9-E50E24DCCA9E"  # Notify


@dataclass
class BLEDeviceConfig:
    name: Optional[str] = "GarlicDK"
    address: Optional[str] = None
    timeout: float = 10.0


class _SerialCompat:
    def __init__(self, parent: "BLEGarlicDevice"):
        self._p = parent

    @property
    def in_waiting(self) -> int:
        return self._p._rx_buf_size()


class BLEGarlicDevice:
    def __init__(self, config: BLEDeviceConfig):
        self.config = config
        self._loop = asyncio.new_event_loop()
        self._t = threading.Thread(target=self._loop.run_forever, daemon=True)
        self._t.start()
        self._client = None
        self._rx_char = None
        self._tx_char = None
        self._rx_q: asyncio.Queue[bytes] = asyncio.Queue()
        self._rx_cache = bytearray()
        self.serial = _SerialCompat(self)
        self._connect_blocking()

    def _rx_cb(self, _: str, data: bytearray):
        # bleak callback occurs in loop thread; enqueue for consumer
        self._loop.call_soon_threadsafe(self._rx_q.put_nowait, bytes(data))

    def _run_coro(self, coro):
        fut = asyncio.run_coroutine_threadsafe(coro, self._loop)
        # Allow some headroom for compound BLE ops
        return fut.result(timeout=self.config.timeout + 10.0)

    async def _ensure_connected_async(self):
        # If disconnected (e.g., target reboot), reconnect and re-subscribe
        try:
            if self._client is not None:
                if getattr(self._client, 'is_connected', False):
                    return
        except Exception:
            pass
        # Reconnect using same config (address preferred)
        # Simple approach: disconnect old client if any
        try:
            if self._client:
                await self._client.disconnect()
        except Exception:
            pass
        # Re-run connect flow
        await self._async_connect()

    async def _async_connect(self):
        from bleak import BleakScanner, BleakClient

        target = None
        if self.config.address:
            try:
                # Ensure BlueZ has the device in cache
                target = await BleakScanner.find_device_by_address(self.config.address, timeout=self.config.timeout)
            except Exception:
                target = None
        if target is None:
            # Try multiple short scans to beat auto-connect races
            for _ in range(3):
                devices = await BleakScanner.discover(timeout=min(self.config.timeout, 5.0))
                # Prefer name match
                if self.config.name:
                    for d in devices:
                        if d.name and self.config.name in d.name:
                            target = d
                            break
                if target is None:
                    for d in devices:
                        try:
                            uuids = (getattr(d, 'metadata', {}) or {}).get("uuids") or []
                        except Exception:
                            uuids = []
                        if any(str(u).lower() == NUS_SERVICE_UUID.lower() for u in uuids):
                            target = d
                            break
                if target is not None:
                    break
        if target is None:
            raise RuntimeError("BLE Garlic device not found (GarlicDK/NUS)")

        # Connection attempts with retry to handle timing races
        last_err = None
        for _ in range(3):
            try:
                client = BleakClient(target)
                await client.connect(timeout=self.config.timeout)
                # Prefer UUID-based usage (Bleak >=1.1)
                try:
                    await client.start_notify(NUS_TX_UUID, self._rx_cb)
                    self._rx_char = NUS_RX_UUID
                    self._tx_char = NUS_TX_UUID
                except Exception:
                    # Older Bleak fallback: enumerate and find characteristics
                    svcs = await client.get_services()  # type: ignore[attr-defined]
                    rx = tx = None
                    for s in svcs:
                        if s.uuid.lower() == NUS_SERVICE_UUID.lower():
                            for c in s.characteristics:
                                if c.uuid.lower() == NUS_RX_UUID.lower():
                                    rx = c
                                if c.uuid.lower() == NUS_TX_UUID.lower():
                                    tx = c
                    if not rx or not tx:
                        for s in svcs:
                            for c in s.characteristics:
                                if c.uuid.lower() == NUS_RX_UUID.lower():
                                    rx = c
                                if c.uuid.lower() == NUS_TX_UUID.lower():
                                    tx = c
                    if not rx or not tx:
                        raise RuntimeError("NUS characteristics not found on device")
                    await client.start_notify(tx, self._rx_cb)
                    self._rx_char = rx
                    self._tx_char = tx
                self._client = client
                break
            except Exception as e:
                last_err = e
                # Best effort disconnect and retry
                try:
                    await client.disconnect()
                except Exception:
                    pass
                await asyncio.sleep(0.3)
        else:
            raise RuntimeError(f"BLE connect failed after retries: {last_err}")

    def _connect_blocking(self):
        logger.info("Connecting to GarlicDK over BLE...")
        self._run_coro(self._async_connect())
        logger.info("BLE connected and notifications enabled")

    def disconnect(self):
        async def _close():
            try:
                if self._client and self._tx_char:
                    await self._client.stop_notify(self._tx_char)
            except Exception:
                pass
            if self._client:
                await self._client.disconnect()
        try:
            self._run_coro(_close())
        finally:
            try:
                self._loop.call_soon_threadsafe(self._loop.stop)
            except Exception:
                pass

    def write(self, data: bytes) -> int:
        if not self._client or not self._rx_char:
            raise RuntimeError("BLE client not connected")

        async def _w(d: bytes):
            # Ensure connected; if not, reconnect (e.g., after reboot)
            await self._ensure_connected_async()
            try:
                # Chunk to 20B to be conservative
                off = 0
                while off < len(d):
                    take = min(20, len(d) - off)
                    await self._client.write_gatt_char(self._rx_char, d[off:off+take], response=True)
                    off += take
                return len(d)
            except Exception:
                # Try one reconnect and retry write once
                await self._ensure_connected_async()
                off = 0
                while off < len(d):
                    take = min(20, len(d) - off)
                    await self._client.write_gatt_char(self._rx_char, d[off:off+take], response=True)
                    off += take
                return len(d)

        return self._run_coro(_w(bytes(data)))

    def _rx_buf_size(self) -> int:
        return len(self._rx_cache)

    def read(self, size: int = 1) -> bytes:
        # Try to gather up to `size` bytes from cache/queue with a short wait
        deadline = time.monotonic() + 0.1
        while len(self._rx_cache) < size and time.monotonic() < deadline:
            try:
                chunk = self._rx_q.get_nowait()
                self._rx_cache.extend(chunk)
            except asyncio.QueueEmpty:
                # Yield briefly to allow notify callback to enqueue
                time.sleep(0.005)
        if size <= 0:
            size = 1
        out = bytes(self._rx_cache[:size])
        del self._rx_cache[:size]
        return out
