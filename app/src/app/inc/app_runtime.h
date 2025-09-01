#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize application runtime.
 *
 * Sets up UART DMA, command transport, BLE (if enabled), and peripherals.
 */
void app_runtime_init(void);

/**
 * @brief Periodic application tick.
 *
 * Services UART DMA, feeds the transport parser, updates heartbeats/LEDs,
 * and performs 1 Hz status work.
 */
void app_runtime_tick(void);

/**
 * @brief Enable or disable BLE advertising.
 *
 * When BLE is compiled in, starts or stops connectable advertising. If BLE is
 * not enabled in the build, returns -ENOTSUP.
 *
 * @param enable true to enable advertising; false to disable
 * @return 0 on success, negative errno on failure
 */
int app_ble_set_advertising(bool enable);

/**
 * @brief Query BLE connection/advertising status.
 *
 * Populates status flags indicating whether advertising is active and
 * whether at least one BLE connection is established. When BLE is not
 * compiled in, both flags are set to false.
 *
 * @param[out] advertising true if advertising is active
 * @param[out] connected   true if a connection is established
 */
void app_ble_get_status(bool *advertising, bool *connected);

#ifdef __cplusplus
}
#endif
