/* Test-only UART DMA shim types and APIs (host unit tests) */

#pragma once

#ifdef __cplusplus
extern "C" {
#endif

struct device;

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
            unsigned char *buf;
            unsigned long len;
            unsigned long offset;
        } rx;
        struct {
            unsigned char *buf;
        } rx_buf;
        struct {
            unsigned long len;
        } tx;
        struct {
            int reason;
        } rx_stop;
    } data;
};

/* Test helpers from driver (available when compiled with UART_DMA_TESTING) */
void uart_dma_test_reset(void);
void uart_dma_test_invoke_event(struct uart_event *evt);
void uart_dma_test_set_hal_rx_buf_rsp(int (*fn)(const struct device *,
                                                unsigned char *,
                                                unsigned long));
void uart_dma_test_set_hal_rx_enable(int (*fn)(
    const struct device *, unsigned char *, unsigned long, unsigned int));
/* Optional HAL shims for TX path and callback registration (host tests) */
void uart_dma_test_set_hal_tx(int (*fn)(
    const struct device *, const unsigned char *, unsigned long, unsigned int));
void uart_dma_test_set_hal_callback_set(
    int (*fn)(const struct device *,
              void (*)(const struct device *, struct uart_event *, void *),
              void *));

#ifdef __cplusplus
}
#endif
