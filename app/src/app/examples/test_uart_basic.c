#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static const struct device *uart_dev = DEVICE_DT_GET(DT_NODELABEL(uart0));

int main(void)
{
    int ret;
    bool led_state = false;
    int counter = 0;
    char msg[64];

    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return -1;
    }

    /* Check UART is ready */
    if (!device_is_ready(uart_dev)) {
        return -1;
    }

    /* Send startup message */
    const char *startup = "NRF52 Starting...\r\n";
    for (int i = 0; startup[i]; i++) {
        uart_poll_out(uart_dev, startup[i]);
    }

    while (1) {
        /* Toggle LED */
        gpio_pin_set_dt(&led, led_state);
        led_state = !led_state;

        /* Send counter message */
        int len = snprintf(msg, sizeof(msg), "Counter: %d, LED: %s\r\n", counter++,
                           led_state ? "ON" : "OFF");

        for (int i = 0; i < len; i++) {
            uart_poll_out(uart_dev, msg[i]);
        }

        /* Check for received characters and echo */
        uint8_t rx_char;
        while (uart_poll_in(uart_dev, &rx_char) == 0) {
            /* Echo the character */
            uart_poll_out(uart_dev, rx_char);

            /* Also send newline after carriage return */
            if (rx_char == '\r') {
                uart_poll_out(uart_dev, '\n');
            }
        }

        k_msleep(500);
    }

    return 0;
}