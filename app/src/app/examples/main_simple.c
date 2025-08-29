/* Simple working UART - start from what worked */
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <stdio.h>

LOG_MODULE_REGISTER(main_simple, LOG_LEVEL_INF);

/* LED configuration */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    const struct device *uart_dev;
    bool led_state = false;
    int counter = 0;
    int ret;

    LOG_INF("Starting simple UART test");

    /* Get UART device */
    uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));
    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART device not ready!");
        return;
    }
    LOG_INF("UART device ready");

    /* Configure UART */
    const struct uart_config cfg = {.baudrate = 115200,
                                    .parity = UART_CFG_PARITY_NONE,
                                    .stop_bits = UART_CFG_STOP_BITS_1,
                                    .data_bits = UART_CFG_DATA_BITS_8,
                                    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};
    ret = uart_configure(uart_dev, &cfg);
    if (ret != 0) {
        LOG_ERR("UART configure failed: %d", ret);
        return;
    }
    LOG_INF("UART configured at 115200 baud");

    /* Configure LED */
    if (gpio_is_ready_dt(&led)) {
        ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
        if (ret != 0) {
            LOG_ERR("LED configure failed: %d", ret);
        } else {
            LOG_INF("LED configured");
        }
    } else {
        LOG_WRN("LED device not ready");
    }

    /* Send startup message */
    const char *msg = "Ready\r\n";
    LOG_INF("Sending startup message: %s", msg);
    for (int i = 0; msg[i]; i++) {
        uart_poll_out(uart_dev, msg[i]);
    }

    LOG_INF("Entering main loop - LED should blink at 3Hz");

    /* Main loop */
    while (1) {
        /* Toggle LED */
        if (gpio_is_ready_dt(&led)) {
            gpio_pin_set_dt(&led, led_state);
            led_state = !led_state;
        }

        /* Check for received data and echo */
        uint8_t c;
        if (uart_poll_in(uart_dev, &c) == 0) {
            /* Echo the character */
            uart_poll_out(uart_dev, c);

            if (c == '\r' || c == '\n') {
                uart_poll_out(uart_dev, '\r');
                uart_poll_out(uart_dev, '\n');

                /* Send counter */
                char buf[64];
                snprintf(buf, sizeof(buf), "Counter: %d\r\n", counter++);
                for (int i = 0; buf[i]; i++) {
                    uart_poll_out(uart_dev, buf[i]);
                }
            }
        }

        /* Sleep for LED blink rate */
        k_msleep(333); /* 3Hz */
    }
}