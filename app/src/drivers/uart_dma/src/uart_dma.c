#include "uart_dma.h"
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#include <circular_buffer.h>

LOG_MODULE_REGISTER(uart_dma, LOG_LEVEL_INF);

/* Static buffers - no dynamic allocation per medical requirements */
static uint8_t tx_buffer_storage[UART_DMA_TX_BUFFER_SIZE];
static uint8_t rx_buffer_storage[UART_DMA_RX_BUFFER_SIZE];

/* Circular buffers for TX and RX */
static circular_buffer_t tx_buffer;
static circular_buffer_t rx_buffer;

/* DMA buffers for async operations - double buffering for RX */
static uint8_t dma_rx_buf[2][UART_DMA_RX_CHUNK_SIZE];
static uint8_t dma_tx_buf[UART_DMA_TX_BUFFER_SIZE];

/* UART device handle */
static const struct device *uart_dev = NULL;

/* RX state for double buffering */
static uint8_t rx_buf_idx = 0;
static bool rx_enabled = false;
static uint8_t *next_rx_buf = NULL;

/* TX state */
static bool tx_in_progress = false;
static size_t tx_len = 0;

/* Statistics */
static struct uart_statistics stats = {0};

/* Initialization flag */
static bool initialized = false;

/* Synchronization */
static struct k_sem tx_sem;
static struct k_sem rx_sem;

/* RX inactivity timeout to deliver partial frames (in microseconds) */
#ifndef UART_DMA_RX_TIMEOUT_US
#define UART_DMA_RX_TIMEOUT_US 20000U /* 20 ms */
#endif

/* UART async callback - handles all DMA events */
static void uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
{
    ARG_UNUSED(dev);
    ARG_UNUSED(user_data);

    switch (evt->type) {
        case UART_TX_DONE:
            LOG_DBG("TX done: %d bytes", evt->data.tx.len);
            stats.tx_bytes += evt->data.tx.len;
            tx_in_progress = false;
            k_sem_give(&tx_sem);
            break;

        case UART_TX_ABORTED:
            LOG_ERR("TX aborted");
            tx_in_progress = false;
            k_sem_give(&tx_sem);
            break;

        case UART_RX_RDY:
            LOG_DBG("RX ready: %d bytes at offset %d", evt->data.rx.len, evt->data.rx.offset);

            /* Copy received data to circular buffer (non-blocking; ISR context) */
            size_t written = circular_buffer_write(
                &rx_buffer, &evt->data.rx.buf[evt->data.rx.offset], evt->data.rx.len);

            if (written < evt->data.rx.len) {
                stats.rx_overruns++;
                LOG_WRN("RX buffer overflow, lost %d bytes", evt->data.rx.len - written);
            }
            stats.rx_bytes += written;
            break;

        case UART_RX_BUF_REQUEST:
            LOG_DBG("RX buffer request");
            /* Provide next buffer for continuous reception */
            if (next_rx_buf != NULL) {
                int ret = uart_rx_buf_rsp(dev, next_rx_buf, UART_DMA_RX_CHUNK_SIZE);
                if (ret < 0) {
                    LOG_ERR("Failed to provide RX buffer: %d", ret);
                } else {
                    /* Switch buffers */
                    rx_buf_idx = (rx_buf_idx + 1) % 2;
                    next_rx_buf = dma_rx_buf[rx_buf_idx];
                }
            }
            break;

        case UART_RX_BUF_RELEASED:
            LOG_DBG("RX buffer released: %p", evt->data.rx_buf.buf);
            /* Buffer is now free for reuse */
            break;

        case UART_RX_DISABLED:
            LOG_DBG("RX disabled");
            rx_enabled = false;
            /* Restart RX if it was intentionally disabled */
            if (initialized) {
                /* Use the released buffer for next RX */
                rx_buf_idx = 0;
                next_rx_buf = dma_rx_buf[1];
                int ret = uart_rx_enable(dev, dma_rx_buf[0], UART_DMA_RX_CHUNK_SIZE,
                                         UART_DMA_RX_TIMEOUT_US);
                if (ret == 0) {
                    rx_enabled = true;
                    LOG_DBG("RX re-enabled");
                } else {
                    LOG_ERR("Failed to re-enable RX: %d", ret);
                }
            }
            break;

        case UART_RX_STOPPED:
            LOG_ERR("RX stopped: reason=%d", evt->data.rx_stop.reason);
            rx_enabled = false;

            /* Handle different stop reasons */
            switch (evt->data.rx_stop.reason) {
                case UART_ERROR_OVERRUN:
                    stats.rx_overruns++;
                    break;
                case UART_ERROR_FRAMING:
                    stats.framing_errors++;
                    break;
                case UART_ERROR_PARITY:
                    stats.parity_errors++;
                    break;
                default:
                    break;
            }

            /* Attempt to restart RX */
            if (initialized) {
                rx_buf_idx = 0;
                next_rx_buf = dma_rx_buf[1];
                int ret = uart_rx_enable(dev, dma_rx_buf[0], UART_DMA_RX_CHUNK_SIZE,
                                         UART_DMA_RX_TIMEOUT_US);
                if (ret == 0) {
                    rx_enabled = true;
                    LOG_DBG("RX restarted after error");
                } else {
                    LOG_ERR("Failed to restart RX after error: %d", ret);
                }
            }
            break;

        default:
            LOG_DBG("Unhandled UART event type: %d", evt->type);
            break;
    }
}

enum uart_dma_status uart_dma_init(void)
{
    int ret;

    if (initialized) {
        return UART_DMA_STATUS_OK;
    }

    /* Initialize semaphores */
    k_sem_init(&tx_sem, 1, 1);
    k_sem_init(&rx_sem, 1, 1);

    /* Get UART device */
    uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return UART_DMA_STATUS_ERROR;
    }

    /* Configure UART */
    const struct uart_config cfg = {.baudrate = 115200,
                                    .parity = UART_CFG_PARITY_NONE,
                                    .stop_bits = UART_CFG_STOP_BITS_1,
                                    .data_bits = UART_CFG_DATA_BITS_8,
                                    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

    ret = uart_configure(uart_dev, &cfg);
    if (ret != 0) {
        /* Some drivers may not support runtime configure; proceed with DT config */
        LOG_WRN("UART runtime configuration failed: %d, using DT defaults", ret);
    }

    /* Initialize circular buffers */
    circular_buffer_init(&tx_buffer, tx_buffer_storage, UART_DMA_TX_BUFFER_SIZE);
    circular_buffer_init(&rx_buffer, rx_buffer_storage, UART_DMA_RX_BUFFER_SIZE);

    /* Register async callback */
    ret = uart_callback_set(uart_dev, uart_callback, NULL);
    if (ret != 0) {
        LOG_ERR("Failed to set UART callback: %d", ret);
        return UART_DMA_STATUS_ERROR;
    }

    /* Prepare for double buffering */
    rx_buf_idx = 0;
    next_rx_buf = dma_rx_buf[1];

    /* Start RX with first buffer; use finite timeout for partial frames */
    ret = uart_rx_enable(uart_dev, dma_rx_buf[0], UART_DMA_RX_CHUNK_SIZE, UART_DMA_RX_TIMEOUT_US);
    if (ret != 0) {
        LOG_ERR("Failed to enable RX: %d", ret);
        return UART_DMA_STATUS_ERROR;
    }

    rx_enabled = true;
    initialized = true;
    LOG_INF("UART DMA initialized with double buffering");

    return UART_DMA_STATUS_OK;
}

enum uart_dma_status uart_dma_send(const uint8_t *data, size_t len)
{
    if (!initialized) {
        return UART_DMA_STATUS_NOT_INITIALIZED;
    }

    if (data == NULL || len == 0) {
        return UART_DMA_STATUS_ERROR;
    }

    /* Add data to circular buffer with protection */
    k_sem_take(&tx_sem, K_FOREVER);
    size_t written = circular_buffer_write(&tx_buffer, data, len);
    k_sem_give(&tx_sem);

    if (written < len) {
        stats.tx_overruns++;
        LOG_WRN("TX buffer full, only wrote %d of %d bytes", written, len);
        return UART_DMA_STATUS_BUFFER_FULL;
    }

    /* Trigger transmission if not already in progress */
    uart_dma_process();

    return UART_DMA_STATUS_OK;
}

enum uart_dma_status uart_dma_send_byte(uint8_t byte)
{
    return uart_dma_send(&byte, 1);
}

size_t uart_dma_rx_available(void)
{
    if (!initialized) {
        return 0;
    }

    k_sem_take(&rx_sem, K_FOREVER);
    size_t available = circular_buffer_available(&rx_buffer);
    k_sem_give(&rx_sem);

    return available;
}

size_t uart_dma_read(uint8_t *data, size_t max_len)
{
    if (!initialized || data == NULL || max_len == 0) {
        return 0;
    }

    k_sem_take(&rx_sem, K_FOREVER);
    size_t read = circular_buffer_read(&rx_buffer, data, max_len);
    k_sem_give(&rx_sem);

    return read;
}

enum uart_dma_status uart_dma_read_byte(uint8_t *byte)
{
    if (!initialized || byte == NULL) {
        return UART_DMA_STATUS_NOT_INITIALIZED;
    }

    k_sem_take(&rx_sem, K_FOREVER);
    uint8_t tmp;
    size_t n = circular_buffer_read(&rx_buffer, &tmp, 1);
    k_sem_give(&rx_sem);
    if (n == 1) {
        *byte = tmp;
        return UART_DMA_STATUS_OK;
    }
    return UART_DMA_STATUS_BUFFER_EMPTY;
}

size_t uart_dma_tx_free_space(void)
{
    if (!initialized) {
        return 0;
    }

    k_sem_take(&tx_sem, K_FOREVER);
    size_t free_space = circular_buffer_free_space(&tx_buffer);
    k_sem_give(&tx_sem);

    return free_space;
}

bool uart_dma_tx_complete(void)
{
    if (!initialized) {
        return true;
    }

    k_sem_take(&tx_sem, K_FOREVER);
    bool empty = circular_buffer_is_empty(&tx_buffer);
    bool complete = empty && !tx_in_progress;
    k_sem_give(&tx_sem);

    return complete;
}

void uart_dma_get_statistics(struct uart_statistics *stats_out)
{
    if (stats_out != NULL) {
        memcpy(stats_out, &stats, sizeof(struct uart_statistics));
    }
}

void uart_dma_reset_statistics(void)
{
    memset(&stats, 0, sizeof(struct uart_statistics));
}

void uart_dma_clear_rx_buffer(void)
{
    if (initialized) {
        k_sem_take(&rx_sem, K_FOREVER);
        circular_buffer_reset(&rx_buffer);
        k_sem_give(&rx_sem);
    }
}

void uart_dma_clear_tx_buffer(void)
{
    if (initialized) {
        k_sem_take(&tx_sem, K_FOREVER);
        circular_buffer_reset(&tx_buffer);
        k_sem_give(&tx_sem);
    }
}

void uart_dma_process(void)
{
    if (!initialized) {
        return;
    }

    /* Process TX if not currently transmitting */
    k_sem_take(&tx_sem, K_NO_WAIT);
    if (!tx_in_progress && !circular_buffer_is_empty(&tx_buffer)) {
        /* Read data from circular buffer to DMA buffer */
        tx_len = circular_buffer_read(&tx_buffer, dma_tx_buf, MIN(UART_DMA_TX_BUFFER_SIZE, 256));

        if (tx_len > 0) {
            int ret = uart_tx(uart_dev, dma_tx_buf, tx_len, SYS_FOREVER_US);
            if (ret == 0) {
                tx_in_progress = true;
                LOG_DBG("Started TX of %d bytes", tx_len);
            } else {
                LOG_ERR("TX failed: %d", ret);
                /* Put data back in buffer */
                circular_buffer_write(&tx_buffer, dma_tx_buf, tx_len);
            }
        }
    }
    k_sem_give(&tx_sem);
}
