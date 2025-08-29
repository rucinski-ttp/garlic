#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute IEEE 802.3 CRC-32 over a buffer.
 *
 * Polynomial 0x04C11DB7 (reflected form 0xEDB88320), reflected input/output,
 * initial value 0xFFFFFFFF, final XOR 0xFFFFFFFF.
 *
 * @param data Pointer to input bytes (may be NULL if len == 0).
 * @param len  Number of bytes to process.
 * @return uint32_t CRC-32 value.
 */
uint32_t crc32_ieee(const uint8_t *data, size_t len);

#ifdef __cplusplus
}
#endif
