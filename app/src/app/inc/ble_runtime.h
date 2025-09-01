#pragma once

#include <stddef.h>

/* BLE (NUS) runtime glue: initializes BLE driver, wires transport, drives LED. */
void ble_runtime_init(void);
void ble_runtime_tick(void);
