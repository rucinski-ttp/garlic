#include <zephyr/kernel.h>
#include <zephyr/sys/printk.h>
#include <zephyr/drivers/gpio.h>

/* The nRF52-DK has 4 LEDs, we'll use LED1 (green) */
#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

int main(void)
{
    int ret;
    bool led_state = true;
    
    printk("LED Blinky at 3Hz on nRF52-DK!\n");
    
    /* Configure LED */
    if (!gpio_is_ready_dt(&led)) {
        printk("Error: LED device not ready\n");
        return -1;
    }
    
    ret = gpio_pin_configure_dt(&led, GPIO_OUTPUT_ACTIVE);
    if (ret < 0) {
        printk("Error: Failed to configure LED pin\n");
        return -1;
    }
    
    printk("Blinking LED at 3Hz (3 times per second)...\n");
    
    while (1) {
        /* Toggle LED */
        gpio_pin_set_dt(&led, led_state);
        led_state = !led_state;
        
        /* 3Hz = 3 cycles per second
         * Period = 1000ms / 3 = 333ms
         * Half period (on/off time) = 166ms */
        k_msleep(166);
    }
    
    return 0;
}