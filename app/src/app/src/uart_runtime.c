// UART-specific runtime glue: transport setup, RX draining
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "drivers/uart_dma/inc/uart_dma.h"
#include "proto/inc/transport.h"
#include "stack/cmd_transport/inc/cmd_transport.h"

LOG_MODULE_REGISTER(uart_runtime, LOG_LEVEL_INF);

static struct transport_ctx s_uart_transport;

static size_t lower_write(const uint8_t *data, size_t len)
{
    /* Ensure we have enough TX space to enqueue the full frame to avoid drops. */
    uint32_t start = k_uptime_get_32();
    while (uart_dma_tx_free_space() < len) {
        uart_dma_process();
        if ((k_uptime_get_32() - start) > 2000U) { /* 2s safety */
            break;
        }
        k_sleep(K_MSEC(1));
    }

    size_t before_free = uart_dma_tx_free_space();
    enum uart_dma_status s = uart_dma_send(data, len);
    if (s != UART_DMA_STATUS_OK) {
        return 0;
    }

    /* Nudge DMA and give the ring a moment to drain to reduce burstiness. */
    uart_dma_process();
    uint32_t wait_start = k_uptime_get_32();
    for (;;) {
        size_t free_now = uart_dma_tx_free_space();
        if (free_now > before_free || uart_dma_tx_complete()) {
            break;
        }
        if ((k_uptime_get_32() - wait_start) > 10U) {
            break; /* don't stall the loop; best-effort pacing */
        }
        k_sleep(K_MSEC(1));
        uart_dma_process();
    }

    return len;
}

static const struct transport_lower_if lower_if = {
    .write = lower_write,
};

void uart_runtime_init(void)
{
    int init_stat = uart_dma_init();
    if (init_stat != UART_DMA_STATUS_OK) {
        LOG_ERR("UART DMA init failed: %d", init_stat);
        printk("UART DMA init failed: %d\r\n", init_stat);
    } else {
        printk("UART DMA init OK\r\n");
    }

    transport_init(&s_uart_transport, &lower_if, cmd_get_transport_cb(), &s_uart_transport);
    cmd_transport_init(&s_uart_transport);
    printk("UART transport ready\r\n");
}

void uart_runtime_tick(void)
{
    uart_dma_process();
    size_t avail = uart_dma_rx_available();
    if (avail == 0) {
        return;
    }

    uint8_t buf[256];
    while ((avail = uart_dma_rx_available()) > 0) {
        size_t to_read = avail < sizeof(buf) ? avail : sizeof(buf);
        size_t n = uart_dma_read(buf, to_read);
        if (n == 0) {
            break;
        }
        transport_rx_bytes(&s_uart_transport, buf, n);
    }
}
