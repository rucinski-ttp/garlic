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
#include "drivers/ble_nus/inc/ble_nus.h"
#include "drivers/tmp119/inc/tmp119.h"

LOG_MODULE_REGISTER(app_runtime, LOG_LEVEL_INF);

#define LED0_NODE DT_ALIAS(led0)
static const struct gpio_dt_spec led = GPIO_DT_SPEC_GET(LED0_NODE, gpios);

static uint32_t last_led_time = 0;

void grlc_app_init(void)
{
#if defined(CONFIG_USE_SEGGER_RTT)
    SEGGER_RTT_WriteString(0, "RTT: runtime init start\n");
#endif
    if (gpio_is_ready_dt(&led)) {
        (void)gpio_pin_configure_dt(&led, GPIO_OUTPUT_INACTIVE);
    }

    /* Initialize interfaces */
    grlc_uart_runtime_init();
    grlc_ble_runtime_init();

    /* Initialize TMP119(s) at boot by scanning known addresses and verifying
     * Device ID (Sec 8.5.11, p.33). Apply default config (Sec 8.5.3).
     */
    int n = grlc_tmp119_boot_init();
    if (n > 0) {
        LOG_INF("TMP119 devices initialized: %d", n);
    } else {
        LOG_WRN("No TMP119 device initialized at boot");
    }
}

void grlc_app_tick(void)
{
    uint32_t now = k_uptime_get_32();

    if (gpio_is_ready_dt(&led) && (now - last_led_time) >= 333U) {
        gpio_pin_toggle_dt(&led);
        last_led_time = now;
    }

    /* Delegate to interface runtimes */
    grlc_uart_runtime_tick();
    grlc_ble_runtime_tick();

    static uint32_t last_hb = 0;
    if ((now - last_hb) >= 1000U) {
#if defined(CONFIG_USE_SEGGER_RTT)
        SEGGER_RTT_WriteString(0, "RTT: hb\n");
#endif
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

    /* Early boot lines via RTT only (keep UART clean) */
#if defined(CONFIG_USE_SEGGER_RTT)
    SEGGER_RTT_WriteString(0, "RTT Boot: Garlic UART DMA starting\n");
    SEGGER_RTT_WriteString(0, "RTT Git: " GARLIC_GIT_HASH "\n");
#endif

    grlc_app_init();
    while (1) {
        grlc_app_tick();
        k_msleep(5);
    }
}

/* Configure a dedicated thread for the app runtime */
K_THREAD_DEFINE(garlic_app_thread, 2048, app_thread, NULL, NULL, NULL, K_PRIO_PREEMPT(7), 0, 0);

/* BLE driver is authoritative for advertising/connect status and control */
int grlc_app_ble_set_advertising(bool enable)
{
#if defined(CONFIG_BT) && defined(CONFIG_BT_ZEPHYR_NUS)
    return grlc_ble_set_advertising(enable);
#else
    ARG_UNUSED(enable);
    return -ENOTSUP;
#endif
}

void grlc_app_ble_get_status(bool *advertising, bool *connected)
{
#if defined(CONFIG_BT) && defined(CONFIG_BT_ZEPHYR_NUS)
    grlc_ble_get_status(advertising, connected);
#else
    if (advertising) {
        *advertising = false;
    }
    if (connected) {
        *connected = false;
    }
#endif
}
