/**
 * @file circular_buffer.h
 * @brief Lock-free circular buffer implementation (one-byte-free ring)
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Circular buffer descriptor.
 */
typedef struct {
    uint8_t *buffer;
    size_t size;
    volatile size_t head;
    volatile size_t tail;
} circular_buffer_t;

/**
 * @brief Initialize a circular buffer.
 * @param cb     Buffer descriptor to initialize.
 * @param buffer Backing storage (size bytes).
 * @param size   Capacity in bytes (must be >= 2).
 * @return 0 on success, -1 on invalid parameters.
 */
int grlc_cb_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);
/** @brief Reset buffer to empty state. */
void grlc_cb_reset(circular_buffer_t *cb);
/**
 * @brief Write bytes into the buffer.
 * @param cb   Buffer descriptor.
 * @param data Source bytes.
 * @param len  Number of bytes to write.
 * @return Number of bytes actually written (<= len).
 */
size_t grlc_cb_write(circular_buffer_t *cb, const uint8_t *data, size_t len);
/**
 * @brief Read bytes from the buffer.
 * @param cb   Buffer descriptor.
 * @param data Destination buffer.
 * @param len  Maximum bytes to read.
 * @return Number of bytes actually read.
 */
size_t grlc_cb_read(circular_buffer_t *cb, uint8_t *data, size_t len);
/**
 * @brief Peek at next bytes without advancing the read index.
 * @param cb   Buffer descriptor.
 * @param data Destination buffer.
 * @param len  Maximum bytes to copy.
 * @return Number of bytes copied.
 */
size_t grlc_cb_peek(const circular_buffer_t *cb, uint8_t *data, size_t len);
/** @brief Number of bytes currently stored (readable). */
size_t grlc_cb_available(const circular_buffer_t *cb);
/** @brief Free space remaining (one byte is always left unused). */
size_t grlc_cb_free_space(const circular_buffer_t *cb);
/** @brief true if buffer contains no data. */
bool grlc_cb_is_empty(const circular_buffer_t *cb);
/** @brief true if no more data can be enqueued (keeps one byte free). */
bool grlc_cb_is_full(const circular_buffer_t *cb);
/**
 * @brief Get contiguous read block pointer and length.
 * @param cb       Buffer descriptor.
 * @param data_ptr Out pointer to contiguous readable region.
 * @param len      Out length of contiguous region.
 * @return 0 on success, -1 if buffer is empty or params invalid.
 */
int grlc_cb_get_read_block(const circular_buffer_t *cb,
                           uint8_t **data_ptr,
                           size_t *len);
/**
 * @brief Advance the read index by @p len bytes.
 * @param cb  Buffer descriptor.
 * @param len Number of bytes to consume.
 */
void grlc_cb_advance_read(circular_buffer_t *cb, size_t len);
/**
 * @brief Get contiguous write block pointer and length.
 * @param cb       Buffer descriptor.
 * @param data_ptr Out pointer to contiguous writable region.
 * @param len      Out length of contiguous region.
 * @return 0 on success (len > 0), -1 on full/invalid.
 */
int grlc_cb_get_write_block(circular_buffer_t *cb,
                            uint8_t **data_ptr,
                            size_t *len);
/**
 * @brief Advance the write index by @p len bytes.
 * @param cb  Buffer descriptor.
 * @param len Number of bytes produced.
 */
void grlc_cb_advance_write(circular_buffer_t *cb, size_t len);

/* Note: legacy circular_buffer_* names removed; use grlc_cb_* */

#ifdef __cplusplus
}
#endif

#endif /* CIRCULAR_BUFFER_H */
