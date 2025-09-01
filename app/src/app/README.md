# App Runtime Module

This module provides a small runtime thread and delegates interface-specific work to compact runtime shims.

- `src/app_runtime.c`: emits early boot lines (RTT + printk), toggles the heartbeat LED, and calls into `uart_runtime_*()` and `ble_runtime_*()` each tick.
- `src/uart_runtime.c`: initializes UART DMA, wires the transport+command glue, and drains RX into the transport.
- `src/ble_runtime.c`: initializes the BLE NUS driver, wires a BLE transport, and drives the BLE status LED (blink while advertising, solid when connected).
- `inc/app_runtime.h`: exposes the tiny runtime API for clarity.

The app starts from a dedicated Zephyr thread (`K_THREAD_DEFINE`) rather than relying on the weak `main()`. Early prints help the hardware tests detect boot reliably via RTT.
