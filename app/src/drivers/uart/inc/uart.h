/**
 * @file uart.h
 * @brief DMA-based UART driver (async RX/TX via Zephyr UART API)
 */
#ifndef UART_H
#define UART_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/**
 * @brief UART buffer sizing.
 */
#define UART_DMA_TX_BUFFER_SIZE 2048
#define UART_DMA_RX_BUFFER_SIZE 1024
#define UART_DMA_RX_CHUNK_SIZE  64 /* DMA transfer chunk size */

/**
 * @brief Runtime UART statistics for monitoring.
 */
struct uart_statistics {
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_overruns;
    uint32_t rx_overruns;
    uint32_t framing_errors;
    uint32_t parity_errors;
};

/**
 * @brief UART driver status/result codes.
 */
enum uart_dma_status {
    UART_DMA_STATUS_OK = 0,
    UART_DMA_STATUS_ERROR = -1,
    UART_DMA_STATUS_BUSY = -2,
    UART_DMA_STATUS_TIMEOUT = -3,
    UART_DMA_STATUS_BUFFER_FULL = -4,
    UART_DMA_STATUS_BUFFER_EMPTY = -5,
    UART_DMA_STATUS_NOT_INITIALIZED = -6
};

/** @brief Initialize UART and start asynchronous RX. */
enum uart_dma_status grlc_uart_init(void);

/**
 * @brief Queue bytes for transmission.
 * @param data Pointer to buffer to send.
 * @param len  Number of bytes to enqueue.
 * @return UART_DMA_STATUS_OK on success, or a negative error/status.
 */
enum uart_dma_status grlc_uart_send(const uint8_t *data, size_t len);

/**
 * @brief Convenience wrapper to send a single byte.
 * @param byte Byte to enqueue for transmission.
 * @return UART_DMA_STATUS_OK on success or BUFFER_FULL if ring is full.
 */
enum uart_dma_status grlc_uart_send_byte(uint8_t byte);

/**
 * @brief Query number of bytes available in the RX ring.
 * @return Count of readable bytes.
 */
size_t grlc_uart_rx_available(void);

/**
 * @brief Read up to @p max_len bytes from the RX ring.
 * @param data    Destination buffer.
 * @param max_len Capacity of @p data.
 * @return Number of bytes copied (<= max_len).
 */
size_t grlc_uart_read(uint8_t *data, size_t max_len);

/**
 * @brief Read one byte from the RX ring.
 * @param byte Out parameter for the received byte.
 * @return OK on success, BUFFER_EMPTY if no byte is available.
 */
enum uart_dma_status grlc_uart_read_byte(uint8_t *byte);

/**
 * @brief Query free space in the TX ring.
 * @return Number of bytes that can be enqueued without blocking.
 */
size_t grlc_uart_tx_free_space(void);

/**
 * @brief Check whether TX ring is empty and no DMA is active.
 * @return true if all queued data has been transmitted.
 */
bool grlc_uart_tx_complete(void);

/**
 * @brief Copy current statistics.
 * @param stats Pointer to structure to receive counters.
 */
void grlc_uart_get_statistics(struct uart_statistics *stats);

/** @brief Reset all statistics counters to zero. */
void grlc_uart_reset_statistics(void);

/** @brief Clear all unread bytes from the RX ring. */
void grlc_uart_clear_rx_buffer(void);

/** @brief Clear all pending bytes from the TX ring. */
void grlc_uart_clear_tx_buffer(void);

/**
 * @brief Service the UART driver (kick TX, copy DMA RX).
 * Should be called frequently from the main loop.
 */
void grlc_uart_process(void);

#endif /* UART_H */
