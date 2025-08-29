/* Simple UART test to verify P0.06/P0.08 connectivity */
#include <zephyr/device.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

LOG_MODULE_REGISTER(test_uart, LOG_LEVEL_INF);

void main(void)
{
    const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

    if (!device_is_ready(uart_dev)) {
        LOG_ERR("UART not ready!");
        return;
    }

    LOG_INF("UART test starting - TX:P0.06 RX:P0.08");

    /* Configure UART */
    const struct uart_config cfg = {.baudrate = 115200,
                                    .parity = UART_CFG_PARITY_NONE,
                                    .stop_bits = UART_CFG_STOP_BITS_1,
                                    .data_bits = UART_CFG_DATA_BITS_8,
                                    .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

    int ret = uart_configure(uart_dev, &cfg);
    if (ret != 0) {
        LOG_ERR("UART config failed: %d", ret);
        return;
    }

    LOG_INF("Sending startup message...");

    /* Send startup message */
    const char *msg = "UART_TEST_READY\r\n";
    for (int i = 0; msg[i]; i++) {
        uart_poll_out(uart_dev, msg[i]);
    }

    LOG_INF("Entering echo loop");

    /* Simple echo loop */
    while (1) {
        uint8_t c;
        if (uart_poll_in(uart_dev, &c) == 0) {
            /* Got character, echo it */
            uart_poll_out(uart_dev, c);
            LOG_DBG("Echoed: 0x%02x", c);
        }
        k_msleep(10);
    }
}