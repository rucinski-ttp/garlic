# Drivers Module

Hardware-facing components such as the UART DMA adapter. Keeps HAL specifics isolated from app logic.

- `uart_dma`: Non-blocking TX/RX with ring buffers feeding the transport layer.

