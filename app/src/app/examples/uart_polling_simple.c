#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/drivers/uart.h>
#include <zephyr/kernel.h>

/* LED for status indication */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

/* UART device */
#define UART_NODE DT_NODELABEL(uart0)

int main(void)
{
    int ret;
    const struct device *uart_dev;

    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return -1;
    }

    /* Get UART device */
    uart_dev = DEVICE_DT_GET(UART_NODE);
    if (!device_is_ready(uart_dev)) {
        /* Fast blink if UART not ready */
        while (1) {
            gpio_pin_toggle_dt(&led);
            k_msleep(100);
        }
    }

    /* Configure UART - simple polling mode */
    struct uart_config uart_cfg = {.baudrate = 115200,
                                   .parity = UART_CFG_PARITY_NONE,
                                   .stop_bits = UART_CFG_STOP_BITS_1,
                                   .data_bits = UART_CFG_DATA_BITS_8,
                                   .flow_ctrl = UART_CFG_FLOW_CTRL_NONE};

    ret = uart_configure(uart_dev, &uart_cfg);
    if (ret != 0) {
        /* Very fast blink if config failed */
        while (1) {
            gpio_pin_toggle_dt(&led);
            k_msleep(50);
        }
    }

    /* Main loop - send counter and echo received data */
    int counter = 0;
    int led_counter = 0;

    while (1) {
        /* Send counter every second */
        if (counter % 100 == 0) { /* Every 1 second (10ms * 100) */
            char msg[32];
            int len = 0;

            /* Simple number to string conversion */
            int num = counter / 100;
            if (num == 0) {
                msg[len++] = '0';
            } else {
                char temp[10];
                int i = 0;
                while (num > 0) {
                    temp[i++] = '0' + (num % 10);
                    num /= 10;
                }
                /* Reverse the string */
                while (i > 0) {
                    msg[len++] = temp[--i];
                }
            }

            /* Add newline */
            msg[len++] = '\r';
            msg[len++] = '\n';

            /* Send each character */
            for (int i = 0; i < len; i++) {
                uart_poll_out(uart_dev, msg[i]);
            }
        }

        /* Check for received data and echo it */
        uint8_t c;
        while (uart_poll_in(uart_dev, &c) == 0) {
            /* Echo the character */
            uart_poll_out(uart_dev, c);

            /* Toggle LED on each received character */
            gpio_pin_toggle_dt(&led);
            led_counter = 50; /* Keep LED on for 500ms */
        }

        /* LED control - slow blink when idle, stay on when receiving */
        if (led_counter > 0) {
            led_counter--;
            gpio_pin_set_dt(&led, 1);
        } else {
            /* Slow blink when idle */
            if (counter % 50 == 0) { /* Every 500ms */
                gpio_pin_toggle_dt(&led);
            }
        }

        counter++;
        k_msleep(10);
    }

    return 0;
}