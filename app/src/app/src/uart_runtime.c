// UART-specific runtime glue: transport setup, RX draining
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#include "drivers/uart/inc/uart.h"
#include "proto/inc/transport.h"
#include "stack/cmd_transport/inc/cmd_transport.h"

LOG_MODULE_REGISTER(uart_runtime, LOG_LEVEL_INF);

static struct transport_ctx s_uart_transport;
static struct cmd_transport_binding s_uart_cmd;

/**
 * @brief Transport lower-layer write over UART DMA.
 *
 * Attempts to ensure room in the TX ring, enqueues the frame, and
 * nudges the driver to start/continue DMA.
 *
 * @param data Pointer to bytes to transmit.
 * @param len  Number of bytes to transmit.
 * @return Number of bytes accepted (len or 0 on failure).
 */
static size_t lower_write(const uint8_t *data, size_t len)
{
    if (!data || len == 0)
        return 0;
    size_t free_now = grlc_uart_tx_free_space();
    if (free_now == 0) {
        /* Non-blocking: nothing accepted */
#ifdef __ZEPHYR__
        LOG_INF("lw: no space");
#endif
        return 0;
    }
    size_t to_write = len < free_now ? len : free_now;
    /* Keep frame sizes reasonable */
    if (to_write > 256)
        to_write = 256;
    if (grlc_uart_send(data, to_write) != UART_DMA_STATUS_OK) {
#ifdef __ZEPHYR__
        LOG_INF("lw: send err");
#endif
        return 0;
    }
    /* Nudge DMA */
    grlc_uart_process();
#ifdef __ZEPHYR__
    LOG_INF("lw: wrote %u", (unsigned)to_write);
#endif
    return to_write;
}

static const struct transport_lower_if lower_if = {
    .write = lower_write,
};

void grlc_uart_runtime_init(void)
{
    int init_stat = grlc_uart_init();
    if (init_stat != UART_DMA_STATUS_OK) {
        LOG_ERR("UART DMA init failed: %d", init_stat);
    } else {
        LOG_INF("UART DMA init OK");
    }

    /* Clear any boot noise or stale bytes before starting transport. */
    grlc_uart_clear_rx_buffer();

    grlc_cmd_transport_bind(&s_uart_cmd, &s_uart_transport);
    grlc_transport_init(&s_uart_transport, &lower_if, grlc_cmd_get_transport_cb(), &s_uart_cmd);
    grlc_cmd_transport_init();
    LOG_INF("UART transport ready");
}

void grlc_uart_runtime_tick(void)
{
    /* First, try to advance any pending TX frames */
    grlc_transport_tx_pump(&s_uart_transport);
    grlc_uart_process();
    size_t avail = grlc_uart_rx_available();
    if (avail == 0) {
        /* also advance transport TX when idle on RX */
        grlc_transport_tx_pump(&s_uart_transport);
        grlc_cmd_transport_tick(&s_uart_cmd);
        return;
    }

    uint8_t buf[256];
    while ((avail = grlc_uart_rx_available()) > 0) {
        size_t to_read = avail < sizeof(buf) ? avail : sizeof(buf);
        size_t n = grlc_uart_read(buf, to_read);
        if (n == 0) {
            break;
        }
        grlc_transport_rx_bytes(&s_uart_transport, buf, n);
    }
    /* After RX handling, pump any pending TX */
    grlc_transport_tx_pump(&s_uart_transport);
    /* Service pending command response if transport was previously busy */
    grlc_cmd_transport_tick(&s_uart_cmd);
}
