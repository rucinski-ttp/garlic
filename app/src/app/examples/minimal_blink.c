#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int ret;

    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        return -1;
    }

    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        return -1;
    }

    /* Simple blink loop - no UART, no logging */
    while (1) {
        gpio_pin_toggle_dt(&led);
        k_msleep(500); /* 1 Hz blink */
    }

    return 0;
}