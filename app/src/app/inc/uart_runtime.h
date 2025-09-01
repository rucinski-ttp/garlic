#pragma once

#include <stddef.h>

/* UART runtime glue: initializes UART DMA, wires transport, drains RX. */
void uart_runtime_init(void);
void uart_runtime_tick(void);
