#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#include <stdio.h>
#include <string.h>
#include <SEGGER_RTT.h>
#include <uart_dma.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmain"
#endif

LOG_MODULE_REGISTER(main, LOG_LEVEL_INF);

#ifndef GARLIC_GIT_HASH
#define GARLIC_GIT_HASH "unknown"
#endif

/* LED configuration */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* Application state */
static uint32_t counter = 0;
static bool led_state = false;
static uint32_t last_status_time = 0;

/* Status message interval (ms) */
#define STATUS_INTERVAL_MS 1000

/* Process received commands */
static void process_rx_data(void)
{
    static char line_accum[256];
    static size_t acc_len = 0;

    uint8_t rx_buf[128];
    size_t bytes_read;

    bytes_read = uart_dma_read(rx_buf, sizeof(rx_buf));
    if (bytes_read == 0) {
        return;
    }

    for (size_t i = 0; i < bytes_read; ++i) {
        char c = (char)rx_buf[i];
        if (c == '\r') {
            continue; /* normalize to \n */
        }
        if (c == '\n') {
            /* Emit echo for accumulated line */
            char echo_buf[300];
            int len = snprintf(echo_buf, sizeof(echo_buf), "Echo: ");
            size_t to_copy =
                (acc_len < (sizeof(echo_buf) - len - 2)) ? acc_len : (sizeof(echo_buf) - len - 2);
            memcpy(&echo_buf[len], line_accum, to_copy);
            len += to_copy;
            echo_buf[len++] = '\r';
            echo_buf[len++] = '\n';
            uart_dma_send((uint8_t *)echo_buf, len);

            /* Counter line */
            char counter_buf[64];
            len = snprintf(counter_buf, sizeof(counter_buf), "Counter: %u\r\n", counter++);
            uart_dma_send((uint8_t *)counter_buf, len);

            acc_len = 0;
        } else if (acc_len < sizeof(line_accum)) {
            line_accum[acc_len++] = c;
        } else {
            /* overflow: reset accumulator to avoid runaway */
            acc_len = 0;
        }
    }
}

/* Send periodic status message */
static void send_status_message(void)
{
    struct uart_statistics stats;
    uart_dma_get_statistics(&stats);

    char status_buf[256];
    int len = snprintf(status_buf, sizeof(status_buf),
                       "[%04u] Status: LED=%s | TX=%u bytes | RX=%u bytes | "
                       "TX_free=%u | RX_avail=%u\r\n",
                       counter, led_state ? "ON" : "OFF", stats.tx_bytes, stats.rx_bytes,
                       uart_dma_tx_free_space(), uart_dma_rx_available());

    uart_dma_send((uint8_t *)status_buf, len);

    /* Also log a concise RTT status once per second as a backup */
    LOG_INF("RTT Status: LED=%s TX=%u RX=%u TX_free=%u RX_avail=%u", led_state ? "ON" : "OFF",
            stats.tx_bytes, stats.rx_bytes, uart_dma_tx_free_space(), uart_dma_rx_available());
}

void main(void)
{
    int ret;

    /* Emit a raw console message over RTT very early to prove RTT works on reset */
    /* Configure RTT channel 0 to block if full to avoid dropping boot lines */
    SEGGER_RTT_SetFlagsUpBuffer(0, SEGGER_RTT_MODE_BLOCK_IF_FIFO_FULL);
    /* Emit raw RTT lines very early to prove RTT works on reset */
    SEGGER_RTT_WriteString(0, "RTT Boot: Garlic UART DMA starting\n");
    SEGGER_RTT_WriteString(0, "RTT Git: " GARLIC_GIT_HASH "\n");
    printk("RTT Boot: Garlic UART DMA starting\r\n");
    printk("RTT Git: %s\r\n", GARLIC_GIT_HASH);

    LOG_INF("Garlic UART DMA Application Starting");

    /* Initialize UART with DMA */
    ret = uart_dma_init();
    if (ret != UART_DMA_STATUS_OK) {
        LOG_ERR("Failed to initialize UART DMA: %d", ret);
        return;
    }

    /* One-time direct poll-out sanity to verify pins/wiring */
    const struct device *uart0 = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (device_is_ready(uart0)) {
        const char *boot = "BOOT\r\n";
        for (const char *p = boot; *p; ++p) {
            uart_poll_out(uart0, *p);
        }
    }

    /* Configure LED */
    if (gpio_is_ready_dt(&led)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret < 0) {
            LOG_ERR("Failed to configure LED: %d", ret);
        }
    } else {
        LOG_WRN("LED device not ready");
    }

    /* Send startup message */
    const char *startup_msg = "Ready\r\n";
    uart_dma_send((uint8_t *)startup_msg, strlen(startup_msg));

    LOG_INF("System initialized, entering main loop");

    /* Main loop */
    uint32_t last_led_time = 0;
    while (1) {
        uint32_t now = k_uptime_get_32();

        /* Toggle LED every ~333ms */
        if (gpio_is_ready_dt(&led) && (now - last_led_time) >= 333U) {
            gpio_pin_set_dt(&led, led_state);
            led_state = !led_state;
            last_led_time = now;
        }

        /* Process UART DMA transfers and RX */
        uart_dma_process();
        if (uart_dma_rx_available() > 0) {
            process_rx_data();
        }

        /* Emit status every ~1000ms (compensate for loop jitter) */
        while ((now - last_status_time) >= STATUS_INTERVAL_MS) {
            send_status_message();
            last_status_time += STATUS_INTERVAL_MS;
            now = k_uptime_get_32();
        }

        k_msleep(100);
    }
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif
