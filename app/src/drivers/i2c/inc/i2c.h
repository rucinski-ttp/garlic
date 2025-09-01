/**
 * @file i2c.h
 * @brief Garlic I2C helper API (async + blocking wrappers)
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief I2C transfer completion callback type.
 *
 * Called when an asynchronous transfer completes.
 *
 * @param result 0 on success, negative errno on failure.
 * @param user   Opaque pointer provided at submit time.
 */
typedef void (*i2c_async_cb_t)(int result, void *user);

/**
 * @brief Initialize the default I2C device used by Garlic.
 *
 * Selects `&i2c0` from devicetree and configures a conservative bus speed
 * if supported by the SoC driver.
 *
 * @return 0 on success, negative errno if the device is not ready.
 */
int grlc_i2c_init(void);

/**
 * @brief Submit an asynchronous I2C write.
 *
 * If Zephyr is built with `CONFIG_I2C_CALLBACK=y`, the request is executed
 * using the callback-enabled API; otherwise this function falls back to a
 * synchronous transfer and invokes the callback immediately with the result.
 *
 * @param addr  7-bit I2C address.
 * @param data  Pointer to buffer to transmit.
 * @param len   Number of bytes to transmit.
 * @param cb    Completion callback (may be NULL).
 * @param user  Opaque pointer returned to the callback.
 * @return 0 if accepted, negative errno on error.
 */
int grlc_i2c_write(uint16_t addr,
                   const uint8_t *data,
                   size_t len,
                   i2c_async_cb_t cb,
                   void *user);

/**
 * @brief Submit an asynchronous I2C read.
 *
 * @param addr  7-bit I2C address.
 * @param data  Destination buffer for received bytes.
 * @param len   Number of bytes to read.
 * @param cb    Completion callback (may be NULL).
 * @param user  Opaque pointer returned to the callback.
 * @return 0 if accepted, negative errno on error.
 */
int grlc_i2c_read(
    uint16_t addr, uint8_t *data, size_t len, i2c_async_cb_t cb, void *user);

/**
 * @brief Submit an asynchronous write-then-read transaction (repeated start).
 *
 * @param addr   7-bit I2C address.
 * @param wdata  Buffer containing bytes to write before issuing the repeated
 * start.
 * @param wlen   Number of bytes to write.
 * @param rdata  Buffer to receive data after the repeated start.
 * @param rlen   Number of bytes to read.
 * @param cb     Completion callback (may be NULL).
 * @param user   Opaque pointer returned to the callback.
 * @return 0 if accepted, negative errno on error.
 */
int grlc_i2c_write_read(uint16_t addr,
                        const uint8_t *wdata,
                        size_t wlen,
                        uint8_t *rdata,
                        size_t rlen,
                        i2c_async_cb_t cb,
                        void *user);

/**
 * @brief Blocking write wrapper built atop the async API.
 *
 * @param addr       7-bit I2C address.
 * @param data       Pointer to buffer to transmit.
 * @param len        Number of bytes to transmit.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait).
 * @return 0 on success, negative errno on failure or timeout.
 */
int grlc_i2c_blocking_write(uint16_t addr,
                            const uint8_t *data,
                            size_t len,
                            int timeout_ms);

/**
 * @brief Blocking read wrapper built atop the async API.
 *
 * @param addr       7-bit I2C address.
 * @param data       Destination buffer for received bytes.
 * @param len        Number of bytes to read.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait).
 * @return 0 on success, negative errno on failure or timeout.
 */
int grlc_i2c_blocking_read(uint16_t addr,
                           uint8_t *data,
                           size_t len,
                           int timeout_ms);

/**
 * @brief Blocking write-then-read wrapper (repeated start).
 *
 * @param addr       7-bit I2C address.
 * @param wdata      Buffer containing bytes to write.
 * @param wlen       Number of bytes to write.
 * @param rdata      Destination buffer for received bytes.
 * @param rlen       Number of bytes to read.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait).
 * @return 0 on success, negative errno on failure or timeout.
 */
int grlc_i2c_blocking_write_read(uint16_t addr,
                                 const uint8_t *wdata,
                                 size_t wlen,
                                 uint8_t *rdata,
                                 size_t rlen,
                                 int timeout_ms);

/**
 * @brief Attempt bus recovery via SCL toggling if the driver supports it.
 *
 * @return 0 on success, negative errno on failure or if unsupported.
 */
int grlc_i2c_bus_recover(void);

/**
 * @brief Probe an address for ACK using a minimal transfer.
 *
 * Useful for I2C address scans.
 *
 * @param addr 7-bit I2C address.
 * @return 0 if the device ACKed, negative errno on error/NACK.
 */
int grlc_i2c_ping(uint16_t addr);

#ifdef __cplusplus
}
#endif
