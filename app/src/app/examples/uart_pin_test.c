#include <zephyr/device.h>
#include <zephyr/devicetree.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>
#ifdef __GNUC__
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wmain"
#endif

LOG_MODULE_REGISTER(uart_pin_test, LOG_LEVEL_DBG);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define UART_NODE DT_NODELABEL(uart0)

/* Test direct GPIO control of UART pins */
static void prelude_led_blink(void)
{
    /* Blink LED quickly to prove boot progress before any UART config */
    for (int i = 0; i < 12; i++) { /* ~2.4s */
        gpio_pin_toggle_dt(&led);
        k_msleep(200);
    }
}
#ifdef __GNUC__
#pragma GCC diagnostic pop
#endif

static void dump_uart_info(const struct device *uart_dev)
{
    struct uart_config cfg;
    int ret;

    LOG_INF("=== UART Device Information ===");
    LOG_INF("Device name: %s", uart_dev->name);
    LOG_INF("Device ready: %s", device_is_ready(uart_dev) ? "YES" : "NO");

    ret = uart_config_get(uart_dev, &cfg);
    if (ret == 0) {
        LOG_INF("Current configuration:");
        LOG_INF("  Baudrate: %d", cfg.baudrate);
        LOG_INF("  Data bits: %d", cfg.data_bits == UART_CFG_DATA_BITS_8 ? 8 :
                                   cfg.data_bits == UART_CFG_DATA_BITS_7 ? 7 :
                                   cfg.data_bits == UART_CFG_DATA_BITS_6 ? 6 :
                                                                           5);
        LOG_INF("  Parity: %s", cfg.parity == UART_CFG_PARITY_NONE ? "None" :
                                cfg.parity == UART_CFG_PARITY_ODD  ? "Odd" :
                                                                     "Even");
        LOG_INF("  Stop bits: %d", cfg.stop_bits == UART_CFG_STOP_BITS_1 ? 1 : 2);
        LOG_INF("  Flow control: %s", cfg.flow_ctrl == UART_CFG_FLOW_CTRL_NONE    ? "None" :
                                      cfg.flow_ctrl == UART_CFG_FLOW_CTRL_RTS_CTS ? "RTS/CTS" :
                                                                                    "DTR/DSR");
    } else {
        LOG_ERR("Failed to get UART config: %d", ret);
    }

/* Check if we're using the right peripheral */
#if DT_NODE_HAS_PROP(UART_NODE, reg)
    LOG_INF("UART base address: 0x%08X", DT_REG_ADDR(UART_NODE));
#endif
}

static void error_blink_pattern(int pulses)
{
    while (1) {
        for (int i = 0; i < pulses; i++) {
            gpio_pin_toggle_dt(&led);
            k_msleep(150);
            gpio_pin_toggle_dt(&led);
            k_msleep(150);
        }
        k_msleep(1000);
    }
}

static void test_uart_loopback(const struct device *uart_dev)
{
    uint8_t tx_char = 'A';
    uint8_t rx_char = 0;
    int ret;

    LOG_INF("Testing UART loopback (send and receive)...");

    /* Send a character */
    uart_poll_out(uart_dev, tx_char);
    LOG_DBG("Sent: 0x%02X '%c'", tx_char, tx_char);

    /* Try to receive */
    k_msleep(10);
    ret = uart_poll_in(uart_dev, &rx_char);
    if (ret == 0) {
        LOG_INF("Received: 0x%02X '%c'", rx_char, rx_char);
        if (rx_char == tx_char) {
            LOG_INF("✓ Loopback successful!");
        } else {
            LOG_WRN("Loopback mismatch: sent 0x%02X, got 0x%02X", tx_char, rx_char);
        }
    } else if (ret == -1) {
        LOG_DBG("No data received (expected if no loopback wire)");
    } else {
        LOG_ERR("uart_poll_in error: %d", ret);
    }
}

int main(void)
{
    int ret;

    printk("\n\n=== UART PIN TEST STARTING ===\n");
    printk("Testing UART on P0.06 (TX) and P0.08 (RX)\n");
    printk("Compiled: " __DATE__ " " __TIME__ "\n\n");

    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED GPIO not ready");
        goto end;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED: %d", ret);
        goto end;
    }

    LOG_INF("LED configured successfully");

    /* Early visible heartbeat before any UART work */
    prelude_led_blink();

    /* Get UART device */
    const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready!");
        error_blink_pattern(2); /* 2 pulses = device not ready */
    }

    LOG_INF("UART device ready");

    /* Dump UART info */
    dump_uart_info(uart_dev);

    /* Configure UART */
    struct uart_config uart_cfg = {.baudrate = 115200,
                                   .parity = UART_CFG_PARITY_NONE,
                                   .stop_bits = UART_CFG_STOP_BITS_1,
                                   .data_bits = UART_CFG_DATA_BITS_8,
                                   .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

    ret = uart_configure(uart_dev, &uart_cfg);
    if (ret != 0) {
        LOG_ERR("Failed to configure UART: %d", ret);
        error_blink_pattern(3); /* 3 pulses = configure failed */
    }

    LOG_INF("✓ UART configured: 115200 8N1");

    /* Test loopback */
    test_uart_loopback(uart_dev);

    /* Main loop */
    LOG_INF("Starting main loop - sending data every second");
    LOG_INF("Connect terminal to /dev/ttyUSB0 at 115200 baud");

    int counter = 0;
    uint32_t last_rx_time = 0;
    int total_rx = 0;
    int total_tx = 0;

    while (1) {
        /* Toggle LED to show we're alive */
        gpio_pin_toggle_dt(&led);

        /* Create and send message */
        char msg[64];
        int len = snprintf(msg, sizeof(msg), "nRF52 Counter: %d\r\n", counter++);

        /* Send each character */
        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, msg[i]);
            total_tx++;
        }

        LOG_DBG("TX[%d]: %s", total_tx, msg);

        /* Check for received data */
        uint8_t rx_buf[64];
        int rx_count = 0;
        uint8_t c;

        while (uart_poll_in(uart_dev, &c) == 0 && rx_count < sizeof(rx_buf) - 1) {
            rx_buf[rx_count++] = c;
            total_rx++;
            last_rx_time = k_uptime_get_32();

            /* Echo back */
            uart_poll_out(uart_dev, c);

            /* Print if it's a newline or buffer is full */
            if (c == '\n' || c == '\r' || rx_count >= sizeof(rx_buf) - 1) {
                rx_buf[rx_count] = '\0';
                LOG_INF("RX[%d]: %s", total_rx, rx_buf);
                rx_count = 0;
            }
        }

        /* Log statistics every 10 seconds */
        if (counter % 10 == 0) {
            LOG_INF("=== Statistics ===");
            LOG_INF("Total TX: %d bytes", total_tx);
            LOG_INF("Total RX: %d bytes", total_rx);
            if (last_rx_time > 0) {
                LOG_INF("Last RX: %d ms ago", k_uptime_get_32() - last_rx_time);
            } else {
                LOG_INF("No data received yet");
            }
        }

        k_msleep(1000);
    }

    /* not reached */
end:
    /* park here on fatal init error */
    while (1) {
        k_msleep(1000);
    }
    return 0;
}
