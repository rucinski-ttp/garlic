#include <string.h>
#include "drivers/uart/inc/uart.h"
#include "utils/circular_buffer/inc/circular_buffer.h"

#ifndef UART_DMA_TESTING
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#else
#include <stdint.h>
#ifndef MIN
#define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#define ARG_UNUSED(x)  (void)(x)
/* Minimal types to satisfy driver in host tests */
struct device {
    int dummy;
};
struct k_sem {
    int dummy;
};
static inline void k_sem_init(struct k_sem *s, int a, int b)
{
    ARG_UNUSED(s);
    ARG_UNUSED(a);
    ARG_UNUSED(b);
}
static inline int k_sem_take(struct k_sem *s, int t)
{
    ARG_UNUSED(s);
    ARG_UNUSED(t);
    return 0;
}
static inline void k_sem_give(struct k_sem *s)
{
    ARG_UNUSED(s);
}
#define K_FOREVER      0
#define K_NO_WAIT      0
#define SYS_FOREVER_US 0
#define LOG_MODULE_REGISTER(x, y)
#define LOG_DBG(...)
#define LOG_INF(...)
#define LOG_WRN(...)
#define LOG_ERR(...)

enum uart_error_reason {
    UART_ERROR_OVERRUN = 1,
    UART_ERROR_FRAMING = 2,
    UART_ERROR_PARITY = 3
};

enum {
    UART_TX_DONE,
    UART_TX_ABORTED,
    UART_RX_RDY,
    UART_RX_BUF_REQUEST,
    UART_RX_BUF_RELEASED,
    UART_RX_DISABLED,
    UART_RX_STOPPED,
};
struct uart_event {
    int type;
    union {
        struct {
            uint8_t *buf;
            size_t len;
            size_t offset;
        } rx;
        struct {
            uint8_t *buf;
        } rx_buf;
        struct {
            size_t len;
        } tx;
        struct {
            int reason;
        } rx_stop;
    } data;
};
#endif

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

/*
 * HAL indirection (used for unit tests). In firmware builds, these resolve to
 * Zephyr driver calls. In tests, setter functions can override them to capture
 * or simulate behavior.
 */
#ifndef UART_DMA_TESTING
#define hal_uart_rx_buf_rsp   uart_rx_buf_rsp
#define hal_uart_rx_enable    uart_rx_enable
#define hal_uart_tx           uart_tx
#define hal_uart_callback_set uart_callback_set
#define hal_device_is_ready   device_is_ready
#define hal_DEVICE_DT_GET(x)  DEVICE_DT_GET(x)
#else
static int (*hal_uart_rx_buf_rsp)(const struct device *,
                                  uint8_t *,
                                  size_t) = NULL;
static int (*hal_uart_rx_enable)(const struct device *,
                                 uint8_t *,
                                 size_t,
                                 uint32_t) = NULL;
static int (*hal_uart_tx)(const struct device *,
                          const uint8_t *,
                          size_t,
                          uint32_t) = NULL;
static int (*hal_uart_callback_set)(const struct device *,
                                    void (*)(const struct device *,
                                             struct uart_event *,
                                             void *),
                                    void *) = NULL;
static int hal_device_is_ready(const struct device *d)
{
    ARG_UNUSED(d);
    return 1;
}
#define hal_DEVICE_DT_GET(x) ((const struct device *)(&uart_dev_stub))
static struct device uart_dev_stub;

/* Test helpers to inject HAL */
void uart_dma_test_set_hal_rx_buf_rsp(int (*fn)(const struct device *,
                                                uint8_t *,
                                                size_t))
{
    hal_uart_rx_buf_rsp = fn;
}
void uart_dma_test_set_hal_rx_enable(
    int (*fn)(const struct device *, uint8_t *, size_t, uint32_t))
{
    hal_uart_rx_enable = fn;
}
void uart_dma_test_set_hal_tx(
    int (*fn)(const struct device *, const uint8_t *, size_t, uint32_t))
{
    hal_uart_tx = fn;
}
void uart_dma_test_set_hal_callback_set(
    int (*fn)(const struct device *,
              void (*)(const struct device *, struct uart_event *, void *),
              void *))
{
    hal_uart_callback_set = fn;
}
#endif

/* UART async callback - handles all DMA events */
/**
 * @brief UART async event handler (ISR context).
 *
 * Handles TX completion/abort and RX ready/buffer events, updates statistics,
 * maintains double-buffered RX, and restarts RX on errors.
 */
static void
uart_callback(const struct device *dev, struct uart_event *evt, void *user_data)
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
            LOG_DBG("RX ready: %d bytes at offset %d",
                    evt->data.rx.len,
                    evt->data.rx.offset);

            /* Copy received data to circular buffer (non-blocking; ISR context)
             */
            size_t written =
                grlc_cb_write(&rx_buffer,
                              &evt->data.rx.buf[evt->data.rx.offset],
                              evt->data.rx.len);

            if (written < evt->data.rx.len) {
                stats.rx_overruns++;
                LOG_WRN("RX buffer overflow, lost %d bytes",
                        evt->data.rx.len - written);
            }
            stats.rx_bytes += written;
            break;

        case UART_RX_BUF_REQUEST: {
            LOG_DBG("RX buffer request");
            /* Provide the alternate buffer for continuous reception */
            uint8_t *buf = dma_rx_buf[rx_buf_idx];
            int ret = hal_uart_rx_buf_rsp(dev, buf, UART_DMA_RX_CHUNK_SIZE);
            if (ret < 0) {
                LOG_ERR("Failed to provide RX buffer: %d", ret);
            } else {
                /* Alternate between buffer 0 and 1 */
                rx_buf_idx = (rx_buf_idx + 1) % 2;
            }
            break;
        }

        case UART_RX_BUF_RELEASED:
            LOG_DBG("RX buffer released: %p", evt->data.rx_buf.buf);
            /* Buffer is now free for reuse */
            break;

        case UART_RX_DISABLED:
            LOG_DBG("RX disabled");
            rx_enabled = false;
            /* Restart RX if it was intentionally disabled */
            if (initialized) {
                /* Restart RX with buffer 0 and set next to 1 */
                rx_buf_idx = 1;
                int ret = hal_uart_rx_enable(dev,
                                             dma_rx_buf[0],
                                             UART_DMA_RX_CHUNK_SIZE,
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
                rx_buf_idx = 1;
                int ret = hal_uart_rx_enable(dev,
                                             dma_rx_buf[0],
                                             UART_DMA_RX_CHUNK_SIZE,
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

enum uart_dma_status grlc_uart_init(void)
{
    int ret;

    if (initialized) {
        return UART_DMA_STATUS_OK;
    }

    /* Initialize semaphores */
    k_sem_init(&tx_sem, 1, 1);
    k_sem_init(&rx_sem, 1, 1);

    /* Get UART device */
    uart_dev = hal_DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!hal_device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return UART_DMA_STATUS_ERROR;
    }

    /* Configure UART (firmware build only) */
#ifndef UART_DMA_TESTING
    const struct uart_config cfg = {.baudrate = 115200,
                                    .parity = UART_CFG_PARITY_NONE,
                                    .stop_bits = UART_CFG_STOP_BITS_1,
                                    .data_bits = UART_CFG_DATA_BITS_8,
                                    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

    ret = uart_configure(uart_dev, &cfg);
    if (ret != 0) {
        /* Some drivers may not support runtime configure; proceed with DT
         * config */
        LOG_WRN("UART runtime configuration failed: %d, using DT defaults",
                ret);
    }
#endif

    /* Initialize circular buffers */
    grlc_cb_init(&tx_buffer, tx_buffer_storage, UART_DMA_TX_BUFFER_SIZE);
    grlc_cb_init(&rx_buffer, rx_buffer_storage, UART_DMA_RX_BUFFER_SIZE);

    /* Register async callback */
    ret = hal_uart_callback_set(uart_dev, uart_callback, NULL);
    if (ret != 0) {
        LOG_ERR("Failed to set UART callback: %d", ret);
        return UART_DMA_STATUS_ERROR;
    }

    /* Prepare for double buffering */
    rx_buf_idx = 1; /* enable with buf[0], next provided will be buf[1] */

    /* Start RX with first buffer; use finite timeout for partial frames */
    ret = hal_uart_rx_enable(uart_dev,
                             dma_rx_buf[0],
                             UART_DMA_RX_CHUNK_SIZE,
                             UART_DMA_RX_TIMEOUT_US);
    if (ret == 0) {
        rx_enabled = true;
        initialized = true;
        LOG_INF("UART RX enabled");
        return UART_DMA_STATUS_OK;
    }
    LOG_ERR("Failed to enable UART RX: %d", ret);
    return UART_DMA_STATUS_ERROR;
}

enum uart_dma_status grlc_uart_send(const uint8_t *data, size_t len)
{
    enum uart_dma_status rc = UART_DMA_STATUS_OK;
    if (!initialized || data == NULL || len == 0) {
        rc = UART_DMA_STATUS_NOT_INITIALIZED;
    } else {
        k_sem_take(&tx_sem, K_FOREVER);
        size_t free_space = grlc_cb_free_space(&tx_buffer);
        if (free_space < len) {
            rc = UART_DMA_STATUS_BUFFER_FULL;
        } else {
            size_t written = grlc_cb_write(&tx_buffer, data, len);
            (void)written;
        }
        k_sem_give(&tx_sem);
        if (rc == UART_DMA_STATUS_OK) {
            /* Kick the TX processing */
            grlc_uart_process();
        }
    }
    return rc;
}

enum uart_dma_status grlc_uart_send_byte(uint8_t byte)
{
    return grlc_uart_send(&byte, 1) == UART_DMA_STATUS_OK ?
               UART_DMA_STATUS_OK :
               UART_DMA_STATUS_BUFFER_FULL;
}

size_t grlc_uart_rx_available(void)
{
    if (!initialized) {
        return 0;
    }

    if (k_sem_take(&rx_sem, K_NO_WAIT) != 0) {
        return 0;
    }
    size_t available = grlc_cb_available(&rx_buffer);
    k_sem_give(&rx_sem);

    return available;
}

size_t grlc_uart_read(uint8_t *data, size_t max_len)
{
    size_t nread = 0;
    if (initialized && data != NULL && max_len != 0) {
        k_sem_take(&rx_sem, K_FOREVER);
        nread = grlc_cb_read(&rx_buffer, data, max_len);
        k_sem_give(&rx_sem);
    }
    return nread;
}

enum uart_dma_status grlc_uart_read_byte(uint8_t *byte)
{
    enum uart_dma_status st = UART_DMA_STATUS_BUFFER_EMPTY;
    if (!initialized || byte == NULL) {
        st = UART_DMA_STATUS_NOT_INITIALIZED;
    } else {
        k_sem_take(&rx_sem, K_FOREVER);
        uint8_t tmp;
        size_t n = grlc_cb_read(&rx_buffer, &tmp, 1);
        k_sem_give(&rx_sem);
        if (n == 1) {
            *byte = tmp;
            st = UART_DMA_STATUS_OK;
        }
    }
    return st;
}

size_t grlc_uart_tx_free_space(void)
{
    size_t free_space = 0;
    if (initialized) {
        k_sem_take(&tx_sem, K_FOREVER);
        free_space = grlc_cb_free_space(&tx_buffer);
        k_sem_give(&tx_sem);
    }
    return free_space;
}

bool grlc_uart_tx_complete(void)
{
    bool complete = true;
    if (initialized) {
        k_sem_take(&tx_sem, K_FOREVER);
        bool empty = grlc_cb_is_empty(&tx_buffer);
        complete = empty && !tx_in_progress;
        k_sem_give(&tx_sem);
    }
    return complete;
}

void grlc_uart_get_statistics(struct uart_statistics *stats_out)
{
    if (stats_out != NULL) {
        memcpy(stats_out, &stats, sizeof(struct uart_statistics));
    }
}

void grlc_uart_reset_statistics(void)
{
    memset(&stats, 0, sizeof(struct uart_statistics));
}

void grlc_uart_clear_rx_buffer(void)
{
    if (initialized) {
        k_sem_take(&rx_sem, K_FOREVER);
        grlc_cb_reset(&rx_buffer);
        k_sem_give(&rx_sem);
    }
}

void grlc_uart_clear_tx_buffer(void)
{
    if (initialized) {
        k_sem_take(&tx_sem, K_FOREVER);
        grlc_cb_reset(&tx_buffer);
        k_sem_give(&tx_sem);
    }
}

void grlc_uart_process(void)
{
    if (initialized) {
        /* Process TX if not currently transmitting */
        k_sem_take(&tx_sem, K_NO_WAIT);
        if (!tx_in_progress && !grlc_cb_is_empty(&tx_buffer)) {
            /* Read data from circular buffer to DMA buffer */
            tx_len = grlc_cb_read(
                &tx_buffer, dma_tx_buf, MIN(UART_DMA_TX_BUFFER_SIZE, 256));

            if (tx_len > 0) {
                int ret =
                    hal_uart_tx(uart_dev, dma_tx_buf, tx_len, SYS_FOREVER_US);
                if (ret == 0) {
                    tx_in_progress = true;
                    LOG_DBG("Started TX of %d bytes", tx_len);
                } else {
                    LOG_ERR("TX failed: %d", ret);
                    /* Put data back in buffer */
                    (void)grlc_cb_write(&tx_buffer, dma_tx_buf, tx_len);
                }
            }
        }
        k_sem_give(&tx_sem);
    }
}

#ifdef UART_DMA_TESTING
/* Test-only helpers */
void uart_dma_test_reset(void)
{
    rx_buf_idx = 0;
    rx_enabled = false;
    tx_in_progress = false;
    tx_len = 0;
    memset(&stats, 0, sizeof(stats));
    initialized = true; /* allow restart paths */
}

void uart_dma_test_invoke_event(struct uart_event *evt)
{
    uart_callback(
        uart_dev ? uart_dev : (const struct device *)&uart_dev_stub, evt, NULL);
}
#endif
