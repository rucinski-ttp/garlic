// BLE-specific runtime glue: NUS driver init, transport setup, status LED
#include <zephyr/device.h>
#include <zephyr/drivers/gpio.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>

#if defined(CONFIG_BT) && defined(CONFIG_BT_ZEPHYR_NUS)

#include "drivers/ble_nus/inc/ble_nus.h"
#include "proto/inc/transport.h"
#include "stack/cmd_transport/inc/cmd_transport.h"

LOG_MODULE_REGISTER(ble_runtime, LOG_LEVEL_INF);

#define LED1_NODE DT_ALIAS(led1)
static const struct gpio_dt_spec ble_led = GPIO_DT_SPEC_GET(LED1_NODE, gpios);
static uint32_t s_last_led_time;

static struct transport_ctx s_ble_transport;
static struct cmd_transport_binding s_ble_cmd;

static size_t lower_write_ble(const uint8_t *data, size_t len)
{
    return grlc_ble_send(data, len);
}

static const struct transport_lower_if lower_if_ble = {
    .write = lower_write_ble,
};

static void ble_rx_shim(const uint8_t *data, size_t len, void *user)
{
    struct transport_ctx *t = (struct transport_ctx *)user;
    if (t && data && len) {
        grlc_transport_rx_bytes(t, data, len);
    }
}

void grlc_ble_runtime_init(void)
{
    if (gpio_is_ready_dt(&ble_led)) {
        (void)gpio_pin_configure_dt(&ble_led, GPIO_OUTPUT_INACTIVE);
    }

    grlc_cmd_transport_bind(&s_ble_cmd, &s_ble_transport);
    grlc_transport_init(&s_ble_transport, &lower_if_ble, grlc_cmd_get_transport_cb(), &s_ble_cmd);
    int ble_rc = grlc_ble_init(ble_rx_shim, &s_ble_transport);
    if (ble_rc == 0) {
        LOG_INF("BLE ready");
    } else {
        LOG_WRN("BLE init failed: %d", ble_rc);
    }
    grlc_cmd_transport_init();
}

void grlc_ble_runtime_tick(void)
{
    if (!gpio_is_ready_dt(&ble_led)) {
        return;
    }
    uint32_t now = k_uptime_get_32();
    bool adv = false, connected = false;
    grlc_ble_get_status(&adv, &connected);
    if (connected) {
        gpio_pin_set_dt(&ble_led, 1);
    } else if (adv) {
        if ((now - s_last_led_time) >= 250U) {
            gpio_pin_toggle_dt(&ble_led);
            s_last_led_time = now;
        }
    } else {
        gpio_pin_set_dt(&ble_led, 0);
    }
}

#else /* !CONFIG_BT || !CONFIG_BT_ZEPHYR_NUS */

void grlc_ble_runtime_init(void)
{
}
void grlc_ble_runtime_tick(void)
{
}

#endif
