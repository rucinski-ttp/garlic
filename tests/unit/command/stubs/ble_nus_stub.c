#include "drivers/ble_nus/inc/ble_nus.h"

static int s_adv_on;
static int s_conn;
static ble_nus_rx_cb_t s_cb;
static void *s_user;
static uint8_t s_reason;

int grlc_ble_init(ble_nus_rx_cb_t rx_cb, void *user)
{
    s_cb = rx_cb;
    s_user = user;
    s_adv_on = 1;
    s_conn = 0;
    s_reason = 0;
    return 0;
}

size_t grlc_ble_send(const uint8_t *data, size_t len)
{
    (void)data;
    return len;
}

int grlc_ble_set_advertising(bool enable)
{
    s_adv_on = enable ? 1 : 0;
    return 0;
}

void grlc_ble_get_status(bool *advertising, bool *connected)
{
    if (advertising) *advertising = s_adv_on != 0;
    if (connected) *connected = s_conn != 0;
}

uint8_t grlc_ble_last_disc_reason(void)
{
    return s_reason;
}
