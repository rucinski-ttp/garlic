/* Public header for the DMA-based UART driver */

#ifndef UART_DMA_H
#define UART_DMA_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/* UART DMA configuration - medical device compliant */
#define UART_DMA_TX_BUFFER_SIZE 1024
#define UART_DMA_RX_BUFFER_SIZE 1024
#define UART_DMA_RX_CHUNK_SIZE  64 /* DMA transfer chunk size */

/* UART statistics for monitoring */
struct uart_statistics {
    uint32_t tx_bytes;
    uint32_t rx_bytes;
    uint32_t tx_overruns;
    uint32_t rx_overruns;
    uint32_t framing_errors;
    uint32_t parity_errors;
};

/* UART DMA driver status */
enum uart_dma_status {
    UART_DMA_STATUS_OK = 0,
    UART_DMA_STATUS_ERROR = -1,
    UART_DMA_STATUS_BUSY = -2,
    UART_DMA_STATUS_TIMEOUT = -3,
    UART_DMA_STATUS_BUFFER_FULL = -4,
    UART_DMA_STATUS_BUFFER_EMPTY = -5,
    UART_DMA_STATUS_NOT_INITIALIZED = -6
};

/* Initialize UART with DMA - must be called before any other operations */
enum uart_dma_status uart_dma_init(void);

/* Send data using DMA - non-blocking */
enum uart_dma_status uart_dma_send(const uint8_t *data, size_t len);

/* Send single byte - non-blocking */
enum uart_dma_status uart_dma_send_byte(uint8_t byte);

/* Check how much data is available to read */
size_t uart_dma_rx_available(void);

/* Read received data - returns actual bytes read */
size_t uart_dma_read(uint8_t *data, size_t max_len);

/* Read single byte - returns UART_DMA_STATUS_BUFFER_EMPTY if no data */
enum uart_dma_status uart_dma_read_byte(uint8_t *byte);

/* Get amount of free space in TX buffer */
size_t uart_dma_tx_free_space(void);

/* Check if TX buffer is empty (all data sent) */
bool uart_dma_tx_complete(void);

/* Get UART statistics */
void uart_dma_get_statistics(struct uart_statistics *stats);

/* Reset UART statistics */
void uart_dma_reset_statistics(void);

/* Clear RX buffer */
void uart_dma_clear_rx_buffer(void);

/* Clear TX buffer */
void uart_dma_clear_tx_buffer(void);

/* Process UART tasks - must be called periodically from main loop */
void uart_dma_process(void);

#endif /* UART_DMA_H */
