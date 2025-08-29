/**
 * @file circular_buffer.h
 * @brief Lock-free circular buffer implementation for UART DMA
 *
 * This circular buffer is designed to work with DMA transfers,
 * providing non-blocking write operations and efficient memory usage.
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
 * @brief Circular buffer structure
 */
typedef struct {
    uint8_t *buffer;      /**< Pointer to data buffer */
    size_t size;          /**< Total size of buffer */
    volatile size_t head; /**< Write position (producer) */
    volatile size_t tail; /**< Read position (consumer) */
} circular_buffer_t;

/**
 * @brief Initialize a circular buffer
 *
 * @param cb Pointer to circular buffer structure
 * @param buffer Pointer to data storage
 * @param size Size of data storage in bytes
 * @return 0 on success, -1 on error
 */
int circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);

/**
 * @brief Reset buffer to empty state
 *
 * @param cb Pointer to circular buffer
 */
void circular_buffer_reset(circular_buffer_t *cb);

/**
 * @brief Write data to buffer (non-blocking)
 *
 * @param cb Pointer to circular buffer
 * @param data Pointer to data to write
 * @param len Length of data to write
 * @return Number of bytes actually written
 */
size_t circular_buffer_write(circular_buffer_t *cb, const uint8_t *data, size_t len);

/**
 * @brief Read data from buffer
 *
 * @param cb Pointer to circular buffer
 * @param data Pointer to destination buffer
 * @param len Maximum length to read
 * @return Number of bytes actually read
 */
size_t circular_buffer_read(circular_buffer_t *cb, uint8_t *data, size_t len);

/**
 * @brief Peek at data without removing it
 *
 * @param cb Pointer to circular buffer
 * @param data Pointer to destination buffer
 * @param len Maximum length to peek
 * @return Number of bytes actually peeked
 */
size_t circular_buffer_peek(const circular_buffer_t *cb, uint8_t *data, size_t len);

/**
 * @brief Get number of bytes available to read
 *
 * @param cb Pointer to circular buffer
 * @return Number of bytes available
 */
size_t circular_buffer_available(const circular_buffer_t *cb);

/**
 * @brief Get free space available for writing
 *
 * @param cb Pointer to circular buffer
 * @return Number of bytes free
 */
size_t circular_buffer_free_space(const circular_buffer_t *cb);

/**
 * @brief Check if buffer is empty
 *
 * @param cb Pointer to circular buffer
 * @return true if empty, false otherwise
 */
bool circular_buffer_is_empty(const circular_buffer_t *cb);

/**
 * @brief Check if buffer is full
 *
 * @param cb Pointer to circular buffer
 * @return true if full, false otherwise
 */
bool circular_buffer_is_full(const circular_buffer_t *cb);

/**
 * @brief Get linear read buffer for DMA operations
 *
 * Gets a pointer and length for a continuous block that can be read.
 * This is useful for DMA operations that need contiguous memory.
 *
 * @param cb Pointer to circular buffer
 * @param data_ptr Output: pointer to start of readable data
 * @param len Output: length of continuous readable data
 * @return 0 on success, -1 if buffer is empty
 */
int circular_buffer_get_read_block(const circular_buffer_t *cb, uint8_t **data_ptr, size_t *len);

/**
 * @brief Advance read pointer after DMA read
 *
 * @param cb Pointer to circular buffer
 * @param len Number of bytes that were read by DMA
 */
void circular_buffer_advance_read(circular_buffer_t *cb, size_t len);

/**
 * @brief Get linear write buffer for DMA operations
 *
 * Gets a pointer and length for a continuous block that can be written.
 *
 * @param cb Pointer to circular buffer
 * @param data_ptr Output: pointer to start of writable area
 * @param len Output: length of continuous writable area
 * @return 0 on success, -1 if buffer is full
 */
int circular_buffer_get_write_block(circular_buffer_t *cb, uint8_t **data_ptr, size_t *len);

/**
 * @brief Advance write pointer after DMA write
 *
 * @param cb Pointer to circular buffer
 * @param len Number of bytes that were written by DMA
 */
void circular_buffer_advance_write(circular_buffer_t *cb, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CIRCULAR_BUFFER_H */