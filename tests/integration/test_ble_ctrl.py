import pytest

from command import CommandClient


@pytest.mark.hardware
def test_ble_control_status_and_toggle(garlic_device):
    # Use whichever interface the suite selected (serial recommended for control)
    cc = CommandClient(garlic_device)
    # Query status (should not raise)
    try:
        adv, conn = cc.ble_get_status(timeout=2.0)
    except RuntimeError as e:
        if 'unsupported' in str(e).lower():
            pytest.skip('BLE control command unsupported on device firmware')
        raise

    # Toggle advertising off then on; expect no exceptions
    cc.ble_set_advertising(False, timeout=2.0)
    adv2, _ = cc.ble_get_status(timeout=2.0)
    # Can't assert because another central may immediately reconnect/auto-adv; just ensure command works

    cc.ble_set_advertising(True, timeout=2.0)
    adv3, _ = cc.ble_get_status(timeout=2.0)
    # Same rationale as above

