# App Runtime Module

This module provides a small runtime thread and hooks the transport/command stack to the UART DMA driver.

- `src/app_runtime.c` emits early boot lines (RTT + printk), initializes UART DMA and LED, wires the transport+command glue, and services the main loop via `app_runtime_tick()`.
- `inc/app_runtime.h` exposes the tiny runtime API for clarity.

The app starts from a dedicated Zephyr thread (`K_THREAD_DEFINE`) rather than relying on the weak `main()`. Early prints help the hardware tests detect boot reliably via RTT.
