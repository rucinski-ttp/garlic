# BLE NUS Driver

A small driver that exposes the Nordic UART Service (NUS) as a byte-oriented
link for the Garlic command transport. It owns BLE bring-up, advertising, and
connection state and provides a simple TX API and an RX callback.

## Overview

- Stack: Zephyr Bluetooth (controller + host)
- Service: Nordic UART Service (NUS)
- Role: Peripheral, connectable advertising (GarlicDK)
- Framing: The Garlic transport (CRC32, fragmentation) rides on top of NUS

## Public API

Declared in `drivers/ble_nus/inc/ble_nus.h`:

- `int ble_nus_init(ble_nus_rx_cb_t rx_cb, void *user)`
  - Enables BLE, registers RX callback, and starts connectable advertising.
- `size_t ble_nus_send(const uint8_t *data, size_t len)`
  - Sends bytes via NUS notifications (split into ATT-sized chunks).
- `int ble_nus_set_advertising(bool enable)`
  - Starts or stops advertising.
- `void ble_nus_get_status(bool *advertising, bool *connected)`
  - Reports current link state.
- `uint8_t ble_nus_last_disc_reason(void)`
  - Returns last disconnect reason (HCI error code semantics).

## Integration Pattern

In your application, bridge the driver to your transport:

```
static void ble_rx_shim(const uint8_t *data, size_t len, void *user) {
    struct transport_ctx *t = (struct transport_ctx *)user;
    if (t && data && len) transport_rx_bytes(t, data, len);
}

transport_init(&t_ble, &lower_if_ble, cmd_get_transport_cb(), &t_ble);
ble_nus_init(ble_rx_shim, &t_ble);
```

Where `lower_if_ble.write = ble_nus_send` feeds transport TX back to NUS.

## Kconfig Notes

Recommended settings (see `app/prj.conf`):

- `CONFIG_BT=y`, `CONFIG_BT_PERIPHERAL=y`
- `CONFIG_BT_ZEPHYR_NUS=y`, `CONFIG_BT_ZEPHYR_NUS_DEFAULT_INSTANCE=y`
- Disable early auto-procedures to avoid LL collisions on connect:
  - `CONFIG_BT_GAP_AUTO_UPDATE_CONN_PARAMS=n`
  - `CONFIG_BT_AUTO_PHY_UPDATE=n`
  - `CONFIG_BT_AUTO_DATA_LEN_UPDATE=n`

Optionally increase heap and ACL buffers if your traffic grows.

## LED Status (optional)

The app maps LED1 to BLE status:
- Blink while advertising (not connected)
- Solid when connected
- Off when not advertising

## Unit Testing

Host unit tests link a stub replacement (`tests/unit/command/stubs/ble_nus_stub.c`) that
implements the driver API without Zephyr BLE and allows testing the command
layer and glue without hardware.

## Design Notes

- The driver is minimal by design: no pairing/bonding, no SMP.
- RX callbacks may run in Bluetooth stack context; keep them lightweight.
- TX is best-effort and may short-write if the link is back-pressured.

## Known Issues / Tips

- Desktop BLE stacks sometimes race during service discovery right after
  connection; the app Kconfig mitigates early controller procedures that can
  trigger disconnects.
- When debugging with RTT, avoid attaching a GDB server to the controller for
  long periods. The provided RTT capture helper uses a short default timeout
  to minimize interference.

