#pragma once

#include <stddef.h>

/**
 * @brief Initialize UART runtime (driver + transport binding).
 */
void grlc_uart_runtime_init(void);

/**
 * @brief Periodic UART runtime tick.
 *
 * Drains RX bytes into the transport parser and services UART TX.
 */
void grlc_uart_runtime_tick(void);
