import os
import struct
import time
import pytest

from command import CommandClient


@pytest.mark.hardware
def test_tmp119_id_and_temperature(garlic_device):
    cc = CommandClient(garlic_device)
    # Ensure device is up
    _ = cc.get_uptime_ms(timeout=2.0)
    # TMP119 expected ID 0x2117 (datasheet Sec 8.5.11, p.33)
    did = cc.tmp119_read_id(0x48, timeout=1.0)
    assert did == 0x2117
    # Temperature in a reasonable indoor range (0..50 C)
    mc = cc.tmp119_read_temp_mc(0x48, timeout=1.0)
    c = mc / 1000.0
    assert -5.0 <= c <= 60.0


@pytest.mark.hardware
def test_tmp119_fatal_on_uninitialized_address(garlic_device):
    # Only run this destructive test when explicitly requested
    if not os.environ.get("GARLIC_RUN_FATAL"):
        pytest.skip("Set GARLIC_RUN_FATAL=1 to run fatal-assert test")

    cc = CommandClient(garlic_device)
    # Trigger an access to an address that is not initialized (0x49)
    # This should print fatal and halt the device (no response afterward)
    with pytest.raises(TimeoutError):
        _ = cc.tmp119_read_id(0x49, timeout=0.5)

    # Subsequent echo should also fail due to halted device
    with pytest.raises(TimeoutError):
        _ = cc.echo(b"ping", timeout=0.5)
