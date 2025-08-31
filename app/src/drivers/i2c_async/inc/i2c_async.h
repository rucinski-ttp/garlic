#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Simple non-blocking I2C interface using Zephyr callback API.
 *
 * Provides both asynchronous (callback-based) transfers and convenience
 * blocking wrappers layered on top (using a semaphore). Uses Zephyr's
 * `i2c_transfer_cb` when `CONFIG_I2C_CALLBACK=y` (Zephyr 3.7), otherwise
 * falls back to synchronous `i2c_transfer`.
 */

/** @brief I2C transfer completion callback.
 *
 * @param result 0 on success, negative errno on failure.
 * @param user   User pointer provided at submit time.
 */
typedef void (*i2c_async_cb_t)(int result, void *user);

/** @brief Initialize default I2C bus (&i2c0 from devicetree).
 *
 * @return 0 on success, negative errno if device is not ready.
 */
int i2c_async_init(void);

/** @brief Submit non-blocking I2C write.
 * @param addr 7-bit I2C address.
 * @param data Data buffer to write.
 * @param len  Number of bytes to write.
 * @param cb   Completion callback (may be NULL for sync fallback).
 * @param user User pointer for callback.
 * @return 0 on accepted, negative errno on error.
 */
int i2c_async_write(uint16_t addr, const uint8_t *data, size_t len, i2c_async_cb_t cb, void *user);

/** @brief Submit non-blocking I2C read.
 * @param addr 7-bit I2C address.
 * @param data Buffer to receive data.
 * @param len  Number of bytes to read.
 * @param cb   Completion callback.
 * @param user User pointer for callback.
 * @return 0 on accepted, negative errno on error.
 */
int i2c_async_read(uint16_t addr, uint8_t *data, size_t len, i2c_async_cb_t cb, void *user);

/** @brief Submit non-blocking write-then-read (repeated start).
 * @param addr 7-bit I2C address.
 * @param wdata Write buffer.
 * @param wlen  Write length.
 * @param rdata Read buffer.
 * @param rlen  Read length.
 * @param cb    Completion callback.
 * @param user  User pointer.
 * @return 0 on accepted, negative errno on error.
 */
int i2c_async_write_read(uint16_t addr, const uint8_t *wdata, size_t wlen, uint8_t *rdata,
                         size_t rlen, i2c_async_cb_t cb, void *user);

/** @brief Blocking write wrapper.
 * @param addr 7-bit I2C address.
 * @param data Data to write.
 * @param len  Byte count.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait).
 * @return 0 on success, negative errno on failure/timeout.
 */
int i2c_blocking_write(uint16_t addr, const uint8_t *data, size_t len, int timeout_ms);

/** @brief Blocking read wrapper.
 * @param addr 7-bit I2C address.
 * @param data RX buffer.
 * @param len  Byte count.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait).
 * @return 0 on success, negative errno on failure/timeout.
 */
int i2c_blocking_read(uint16_t addr, uint8_t *data, size_t len, int timeout_ms);

/** @brief Blocking write-then-read wrapper (repeated start).
 * @param addr 7-bit I2C address.
 * @param wdata Write buffer.
 * @param wlen  Write length.
 * @param rdata Read buffer.
 * @param rlen  Read length.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait).
 * @return 0 on success, negative errno on failure/timeout.
 */
int i2c_blocking_write_read(uint16_t addr, const uint8_t *wdata, size_t wlen, uint8_t *rdata,
                            size_t rlen, int timeout_ms);

/** @brief Attempt bus recovery sequence (SCL toggling) if supported.
 *
 * Uses Zephyr's `i2c_recover_bus()` when available in the driver.
 * @return 0 on success, negative errno on failure or unsupported.
 */
int i2c_bus_recover(void);

/** @brief Probe an address for ACK using a zero-length write.
 *
 * Many targets will ACK a zero-length write; this is suitable for address scans.
 * @param addr 7-bit I2C address.
 * @return 0 if ACKed, negative errno on NACK/error.
 */
int i2c_ping(uint16_t addr);

#ifdef __cplusplus
}
#endif
