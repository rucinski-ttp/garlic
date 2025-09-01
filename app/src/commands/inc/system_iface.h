#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @brief Get system uptime in milliseconds. */
uint64_t grlc_sys_uptime_ms(void);

/**
 * @brief Read a region of flash into a buffer.
 * @param addr Absolute flash address to read from
 * @param dst  Destination buffer pointer
 * @param len  Number of bytes to read
 * @return number of bytes read (len on success), or 0 on error
 */
size_t grlc_sys_flash_read(uint32_t addr, uint8_t *dst, size_t len);

#ifdef __cplusplus
}
#endif
