/* Test with RTT logging and gradual UART addition */
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <zephyr/sys/printk.h>

LOG_MODULE_REGISTER(test_rtt, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    int ret;
    bool led_state = false;
    const struct device *uart_dev;

    /* Use printk for immediate RTT output */
    printk("\n\n=== RTT TEST STARTING ===\n");
    LOG_INF("RTT Test Application Starting");

    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        printk("ERROR: LED device not ready\n");
        LOG_ERR("LED device not ready");
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("ERROR: LED configure failed: %d\n", ret);
        LOG_ERR("LED configure failed: %d", ret);
        return;
    }

    printk("LED configured successfully\n");
    LOG_INF("LED configured");

    /* Try to get UART device */
    printk("Getting UART device...\n");
    uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

    if (!device_is_ready(uart_dev)) {
        printk("WARNING: UART device not ready, continuing with LED only\n");
        LOG_WRN("UART device not ready");
    } else {
        printk("UART device is ready!\n");
        LOG_INF("UART device ready");

        /* Try to configure UART */
        const struct uart_config cfg = {.baudrate = 115200,
                                        .parity = UART_CFG_PARITY_NONE,
                                        .stop_bits = UART_CFG_STOP_BITS_1,
                                        .data_bits = UART_CFG_DATA_BITS_8,
                                        .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

        printk("Configuring UART...\n");
        ret = uart_configure(uart_dev, &cfg);
        if (ret != 0) {
            printk("ERROR: UART configure failed: %d\n", ret);
            LOG_ERR("UART configure failed: %d", ret);
        } else {
            printk("UART configured successfully\n");
            LOG_INF("UART configured at 115200");

            /* Send test message */
            const char *msg = "RTT_TEST_OK\r\n";
            printk("Sending test message: %s", msg);
            for (int i = 0; msg[i]; i++) {
                uart_poll_out(uart_dev, msg[i]);
            }
        }
    }

    printk("Entering main loop - LED should blink\n");
    LOG_INF("Main loop started");

    int loop_count = 0;

    /* Main loop */
    while (1) {
        gpio_pin_set_dt(&led, led_state);
        led_state = !led_state;

        /* Log periodically */
        if (++loop_count % 10 == 0) {
            printk("Loop %d: LED=%s\n", loop_count, led_state ? "ON" : "OFF");
            LOG_INF("Heartbeat: %d", loop_count);
        }

        k_msleep(333);
    }
}