#pragma once

#include <stddef.h>

/** @brief Initialize BLE runtime (NUS driver and transport binding). */
void grlc_ble_runtime_init(void);

/** @brief Periodic BLE runtime tick: update status LED. */
void grlc_ble_runtime_tick(void);
