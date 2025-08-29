#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(uart_test, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

#define UART_NODE DT_NODELABEL(uart0)

static void uart_poll_test(const struct device *uart_dev)
{
    uint8_t tx_buf[] = "Hello from nRF52!\r\n";
    uint8_t rx_buf[64];
    int rx_count = 0;

    printk("Starting UART polling test...\n");
    LOG_INF("TX buffer: %s", tx_buf);

    /* Send data using polling */
    for (int i = 0; i < sizeof(tx_buf) - 1; i++) {
        uart_poll_out(uart_dev, tx_buf[i]);
    }

    LOG_INF("Sent %d bytes", sizeof(tx_buf) - 1);

    /* Try to receive with timeout */
    printk("Waiting for RX data (5 seconds)...\n");
    uint32_t start_time = k_uptime_get_32();

    while ((k_uptime_get_32() - start_time) < 5000 && rx_count < sizeof(rx_buf) - 1) {
        uint8_t c;
        if (uart_poll_in(uart_dev, &c) == 0) {
            rx_buf[rx_count++] = c;
            printk("RX: 0x%02X '%c'\n", c, (c >= 32 && c < 127) ? c : '.');

            /* Echo back */
            uart_poll_out(uart_dev, c);

            if (c == '\n' || c == '\r') {
                break;
            }
        }
        k_msleep(1);
    }

    if (rx_count > 0) {
        rx_buf[rx_count] = '\0';
        LOG_INF("Received %d bytes: %s", rx_count, rx_buf);
    } else {
        LOG_WRN("No data received");
    }
}

int main(void)
{
    int ret;

    printk("\n\n=== UART SIMPLE TEST STARTING ===\n");
    printk("Compiled: " __DATE__ " " __TIME__ "\n");

    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        LOG_ERR("LED GPIO not ready");
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        LOG_ERR("Failed to configure LED: %d", ret);
        return -1;
    }

    LOG_INF("LED configured successfully");

    /* Get UART device */
    const struct device *uart_dev = DEVICE_DT_GET(UART_NODE);
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready");
        return -1;
    }

    LOG_INF("UART device ready: %s", uart_dev->name);

    /* Configure UART */
    struct uart_config uart_cfg = {.baudrate = 115200,
                                   .parity = UART_CFG_PARITY_NONE,
                                   .stop_bits = UART_CFG_STOP_BITS_1,
                                   .data_bits = UART_CFG_DATA_BITS_8,
                                   .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

    ret = uart_configure(uart_dev, &uart_cfg);
    if (ret != 0) {
        LOG_ERR("Failed to configure UART: %d", ret);
        return -1;
    }

    LOG_INF("UART configured: 115200 8N1, no flow control");
    LOG_INF("TX pin: P0.06, RX pin: P0.08");

    /* Main loop */
    int counter = 0;
    while (1) {
        /* Toggle LED */
        gpio_pin_toggle_dt(&led);

        /* Send counter value */
        char msg[64];
        snprintf(msg, sizeof(msg), "Counter: %d\r\n", counter++);

        LOG_INF("Sending: %s", msg);

        for (int i = 0; msg[i] != '\0'; i++) {
            uart_poll_out(uart_dev, msg[i]);
        }

        /* Check for received data */
        uint8_t c;
        while (uart_poll_in(uart_dev, &c) == 0) {
            printk("RX: 0x%02X '%c'\n", c, (c >= 32 && c < 127) ? c : '.');
            /* Echo back */
            uart_poll_out(uart_dev, c);
        }

        k_msleep(1000);
    }

    return 0;
}