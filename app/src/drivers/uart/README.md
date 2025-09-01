# UART Driver (DMA, Async)

This module provides a non-blocking UART driver built on Zephyr's async API. TX and RX use
ring buffers; RX is double-buffered with a 20 ms inactivity timeout to deliver partial lines.

- Public API: `drivers/uart/inc/uart.h` (`grlc_uart_*`)
- No dynamic allocation on runtime paths
- Designed for medical-grade reliability (warnings as errors, unit tested)

Key features:
- Async RX with double buffering and inactivity timeout
- Circular buffers (one-byte-free ring) for TX/RX
- Minimal ISR work; main thread drains and services DMA
