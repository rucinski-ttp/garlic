/**
 * @brief BLE NUS driver interface
 *
 * Provides a simple API to send/receive bytes over the Nordic UART Service
 * (NUS) using Zephyr's Bluetooth stack. Incoming bytes are delivered via a
 * registered callback. This driver owns the BLE stack bring-up and
 * advertising lifecycle.
 */

#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** Callback invoked when bytes are received over NUS */
typedef void (*ble_nus_rx_cb_t)(const uint8_t *data, size_t len, void *user);

/**
 * @brief Initialize the BLE NUS driver and start advertising.
 *
 * @param rx_cb Callback invoked on received bytes (may be called from BT
 *              context). May be NULL to disable RX delivery.
 * @param user  Opaque pointer passed back to @p rx_cb.
 * @return 0 on success, negative errno on failure.
 */
int grlc_ble_init(ble_nus_rx_cb_t rx_cb, void *user);

/**
 * @brief Send data over BLE NUS (notifications).
 *
 * Splits into ATT-sized chunks internally. Returns the number of bytes
 * accepted for transmission.
 *
 * @param data Pointer to bytes to send
 * @param len  Number of bytes to send
 * @return number of bytes consumed (<= len)
 */
size_t grlc_ble_send(const uint8_t *data, size_t len);

/**
 * @brief Enable or disable advertising.
 *
 * @param enable true to start advertising; false to stop
 * @return 0 on success, negative errno on failure
 */
int grlc_ble_set_advertising(bool enable);

/**
 * @brief Query BLE advertising/connection status.
 *
 * @param[out] advertising true if advertising is active
 * @param[out] connected   true if at least one connection is active
 */
void grlc_ble_get_status(bool *advertising, bool *connected);

/**
 * @brief Get last disconnect reason (HCI error code semantics).
 *
 * @return last disconnect reason code (0 if none)
 */
uint8_t grlc_ble_last_disc_reason(void);
/* Legacy names removed; use grlc_ble_* */

#ifdef __cplusplus
}
#endif
