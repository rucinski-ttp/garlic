import asyncio
import pytest

from fixtures.command import CommandClient
from fixtures.serial_io import SerialGarlicDevice, GarlicDeviceConfig, find_garlic_device


async def _discover_name(name: str, timeout: float = 6.0) -> bool:
    try:
        from bleak import BleakScanner
    except Exception:
        pytest.skip("bleak not available; install tests/integration/requirements.txt")
    devices = await BleakScanner.discover(timeout=timeout)
    for d in devices:
        if d.name and name in d.name:
            return True
    return False


@pytest.mark.hardware
@pytest.mark.timeout(20)
def test_ble_advertising_visible_by_name():
    name = getattr(pytest, 'garlic_ble_name', 'GarlicDK')
    found = asyncio.run(_discover_name(name, timeout=8.0))
    assert found, f"BLE device named {name} not found in scan"


@pytest.mark.hardware
@pytest.mark.timeout(40)
def test_ble_toggle_advertising_via_serial():
    # Set up serial command path irrespective of selected interface
    port = getattr(pytest, 'garlic_port', None) or find_garlic_device()
    if not port:
        pytest.skip("No serial port available to control device")
    dev = SerialGarlicDevice(GarlicDeviceConfig(port=port))
    cc = CommandClient(dev)

    name = getattr(pytest, 'garlic_ble_name', 'GarlicDK')

    # Ensure BLE is on and visible
    try:
        cc.ble_set_advertising(True, timeout=2.0)
    except RuntimeError as e:
        if 'status=2' in str(e):
            pytest.skip('BLE control command unsupported on device firmware')
        raise
    on_found = asyncio.run(_discover_name(name, timeout=8.0))
    assert on_found, "Expected BLE advertising to be discoverable"

    # Turn off and verify not visible
    cc.ble_set_advertising(False, timeout=2.0)
    off_found = asyncio.run(_discover_name(name, timeout=6.0))
    assert not off_found, "BLE advertising still visible after disabling"

    # Re-enable and verify again
    cc.ble_set_advertising(True, timeout=2.0)
    re_found = asyncio.run(_discover_name(name, timeout=8.0))
    assert re_found, "BLE advertising not visible after re-enabling"

    dev.disconnect()
