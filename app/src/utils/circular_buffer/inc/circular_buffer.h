/**
 * @file circular_buffer.h
 * @brief Lock-free circular buffer implementation for UART DMA
 */

#ifndef CIRCULAR_BUFFER_H
#define CIRCULAR_BUFFER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    uint8_t *buffer;
    size_t size;
    volatile size_t head;
    volatile size_t tail;
} circular_buffer_t;

int circular_buffer_init(circular_buffer_t *cb, uint8_t *buffer, size_t size);
void circular_buffer_reset(circular_buffer_t *cb);
size_t circular_buffer_write(circular_buffer_t *cb, const uint8_t *data, size_t len);
size_t circular_buffer_read(circular_buffer_t *cb, uint8_t *data, size_t len);
size_t circular_buffer_peek(const circular_buffer_t *cb, uint8_t *data, size_t len);
size_t circular_buffer_available(const circular_buffer_t *cb);
size_t circular_buffer_free_space(const circular_buffer_t *cb);
bool circular_buffer_is_empty(const circular_buffer_t *cb);
bool circular_buffer_is_full(const circular_buffer_t *cb);
int circular_buffer_get_read_block(const circular_buffer_t *cb, uint8_t **data_ptr, size_t *len);
void circular_buffer_advance_read(circular_buffer_t *cb, size_t len);
int circular_buffer_get_write_block(circular_buffer_t *cb, uint8_t **data_ptr, size_t *len);
void circular_buffer_advance_write(circular_buffer_t *cb, size_t len);

#ifdef __cplusplus
}
#endif

#endif /* CIRCULAR_BUFFER_H */
