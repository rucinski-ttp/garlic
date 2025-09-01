// Runtime loop and thread bootstrap for the Garlic app
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>
#if defined(CONFIG_USE_SEGGER_RTT)
#include <SEGGER_RTT.h>
#endif

#include "app/inc/ble_runtime.h"
#include "app/inc/uart_runtime.h"
#include "drivers/tmp119/inc/tmp119.h"

LOG_MODULE_REGISTER(app_runtime, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static uint32_t last_led_time = 0;

void app_runtime_init(void)
{
#if defined(CONFIG_USE_SEGGER_RTT)
    SEGGER_RTT_WriteString(0, "RTT: runtime init start\n");
#endif
    if (gpio_is_ready_dt(&led)) {
        (void)gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    }

    /* Initialize interfaces */
    uart_runtime_init();
    ble_runtime_init();

    /* Initialize TMP119(s) at boot by scanning known addresses and verifying
     * Device ID (Sec 8.5.11, p.33). Apply default config (Sec 8.5.3).
     */
    int n = tmp119_boot_init();
    if (n > 0) {
        LOG_INF("TMP119 devices initialized: %d", n);
    } else {
        LOG_WRN("No TMP119 device initialized at boot");
    }
}

void app_runtime_tick(void)
{
    uint32_t now = k_uptime_get_32();

    if (gpio_is_ready_dt(&led) && (now - last_led_time) >= 333U) {
        gpio_pin_toggle_dt(&led);
        last_led_time = now;
    }

    /* Delegate to interface runtimes */
    uart_runtime_tick();
    ble_runtime_tick();

    static uint32_t last_hb = 0;
    if ((now - last_hb) >= 1000U) {
#if defined(CONFIG_USE_SEGGER_RTT)
        SEGGER_RTT_WriteString(0, "RTT: hb\n");
#endif
        printk("HB\r\n");
        last_hb = now;
    }
}

/*
 * App bootstrap policy:
 * - We intentionally avoid overriding Zephyr's weak main(). Doing so is
 *   fragile with respect to link order and modular CMake. Instead, the app
 *   starts from a dedicated Zephyr thread defined here, which is robust and
 *   easy to verify with a post-link symbol check.
 */
static void app_thread(void *a, void *b, void *c)
{
    ARG_UNUSED(a);
    ARG_UNUSED(b);
    ARG_UNUSED(c);

    /* Early boot lines for integration tests */
    printk("RTT Boot: Garlic UART DMA starting\r\n");
    printk("RTT Git: %s\r\n", GARLIC_GIT_HASH);

    app_runtime_init();
    while (1) {
        app_runtime_tick();
        k_msleep(10);
    }
}

/* Configure a dedicated thread for the app runtime */
K_THREAD_DEFINE(garlic_app_thread, 2048, app_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, 0);

/* BLE driver is authoritative for advertising/connect status and control */
