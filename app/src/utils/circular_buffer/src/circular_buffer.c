/*
 * Implementation matching app/include/circular_buffer.h
 * One-byte-free ring buffer, no dynamic allocation.
 */

#include "utils/circular_buffer/inc/circular_buffer.h"
#include <string.h>

/**
 * @brief Advance an index in a circular space.
 * @param idx  Current index.
 * @param inc  Increment to apply.
 * @param size Ring capacity.
 * @return New index modulo @p size.
 */
static inline size_t cb_advance(size_t idx, size_t inc, size_t size)
{
    return (idx + inc) % size;
}

int grlc_cb_init(circular_buffer_t *cb, uint8_t *buffer, size_t size)
{
    if (cb == NULL || buffer == NULL || size < 2) { /* need at least 2 to keep one free */
        return -1;
    }
    cb->buffer = buffer;
    cb->size = size;
    cb->head = 0;
    cb->tail = 0;
    return 0;
}

void grlc_cb_reset(circular_buffer_t *cb)
{
    if (!cb) {
        return;
    }
    cb->head = 0;
    cb->tail = 0;
}

bool grlc_cb_is_empty(const circular_buffer_t *cb)
{
    if (!cb) {
        return true;
    }
    return cb->head == cb->tail;
}

bool grlc_cb_is_full(const circular_buffer_t *cb)
{
    if (!cb) {
        return true;
    }
    return cb_advance(cb->head, 1, cb->size) == cb->tail;
}

size_t grlc_cb_available(const circular_buffer_t *cb)
{
    if (!cb) {
        return 0;
    }
    if (cb->head >= cb->tail) {
        return cb->head - cb->tail;
    }
    return cb->size - (cb->tail - cb->head);
}

size_t grlc_cb_free_space(const circular_buffer_t *cb)
{
    if (!cb) {
        return 0;
    }
    /* Always keep one byte free */
    return (cb->size - 1) - grlc_cb_available(cb);
}

size_t grlc_cb_write(circular_buffer_t *cb, const uint8_t *data, size_t len)
{
    if (!cb || !data || len == 0) {
        return 0;
    }
    size_t free_space = grlc_cb_free_space(cb);
    size_t to_write = len < free_space ? len : free_space;
    for (size_t i = 0; i < to_write; i++) {
        cb->buffer[cb->head] = data[i];
        cb->head = cb_advance(cb->head, 1, cb->size);
    }
    return to_write;
}

size_t grlc_cb_read(circular_buffer_t *cb, uint8_t *data, size_t len)
{
    if (!cb || !data || len == 0) {
        return 0;
    }
    size_t avail = grlc_cb_available(cb);
    size_t to_read = len < avail ? len : avail;
    for (size_t i = 0; i < to_read; i++) {
        data[i] = cb->buffer[cb->tail];
        cb->tail = cb_advance(cb->tail, 1, cb->size);
    }
    return to_read;
}

size_t grlc_cb_peek(const circular_buffer_t *cb, uint8_t *data, size_t len)
{
    if (!cb || !data || len == 0) {
        return 0;
    }
    size_t avail = grlc_cb_available(cb);
    size_t to_peek = len < avail ? len : avail;
    size_t t = cb->tail;
    for (size_t i = 0; i < to_peek; i++) {
        data[i] = cb->buffer[t];
        t = cb_advance(t, 1, cb->size);
    }
    return to_peek;
}

int grlc_cb_get_read_block(const circular_buffer_t *cb, uint8_t **data_ptr, size_t *len)
{
    if (!cb || !data_ptr || !len || grlc_cb_is_empty(cb)) {
        return -1;
    }
    if (cb->head >= cb->tail) {
        *data_ptr = &cb->buffer[cb->tail];
        *len = cb->head - cb->tail;
    } else {
        *data_ptr = &cb->buffer[cb->tail];
        *len = cb->size - cb->tail; /* up to end of buffer */
    }
    return 0;
}

void grlc_cb_advance_read(circular_buffer_t *cb, size_t len)
{
    if (!cb || len == 0) {
        return;
    }
    size_t avail = grlc_cb_available(cb);
    size_t adv = len < avail ? len : avail;
    cb->tail = cb_advance(cb->tail, adv, cb->size);
}

int grlc_cb_get_write_block(circular_buffer_t *cb, uint8_t **data_ptr, size_t *len)
{
    if (!cb || !data_ptr || !len || grlc_cb_is_full(cb)) {
        return -1;
    }
    *data_ptr = &cb->buffer[cb->head];
    if (cb->head >= cb->tail) {
        /* space to end; keep one byte free if tail == 0 */
        size_t end_space = cb->size - cb->head;
        if (cb->tail == 0) {
            if (end_space == 0) {
                *len = 0;
            } else {
                *len = end_space - 1; /* leave one byte free */
            }
        } else {
            *len = end_space;
        }
    } else {
        /* contiguous space until just before tail */
        *len = (cb->tail - cb->head) - 1;
    }
    return (*len > 0) ? 0 : -1;
}

void grlc_cb_advance_write(circular_buffer_t *cb, size_t len)
{
    if (!cb || len == 0) {
        return;
    }
    size_t free = grlc_cb_free_space(cb);
    size_t adv = len < free ? len : free;
    cb->head = cb_advance(cb->head, adv, cb->size);
}
