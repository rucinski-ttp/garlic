/* Minimal LED blink test - no UART */
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

void main(void)
{
    int ret;
    bool led_state = false;

    /* Configure LED only */
    if (!gpio_is_ready_dt(&led)) {
        return;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return;
    }

    /* Just blink LED */
    while (1) {
        gpio_pin_set_dt(&led, led_state);
        led_state = !led_state;
        k_msleep(333);
    }
}