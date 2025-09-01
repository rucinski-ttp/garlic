#include "drivers/ble_nus/inc/ble_nus.h"

#include <zephyr/bluetooth/bluetooth.h>
#include <zephyr/bluetooth/hci.h>
#include <zephyr/bluetooth/services/nus.h>
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
#include <string.h>

LOG_MODULE_REGISTER(ble_nus, LOG_LEVEL_INF);

/* State */
static struct bt_conn *s_conn;
static bool s_adv_on;
static uint8_t s_last_disc_reason;
static ble_nus_rx_cb_t s_rx_cb;
static void *s_rx_user;

#define NUS_CHUNK 20u

static void
nus_rx_cb(struct bt_conn *conn, const void *data, uint16_t len, void *ctx)
{
    ARG_UNUSED(conn);
    ARG_UNUSED(ctx);
    if (s_rx_cb && data && len) {
        s_rx_cb((const uint8_t *)data, len, s_rx_user);
    }
}

static void connected(struct bt_conn *conn, uint8_t err)
{
    if (err == 0) {
        s_conn = bt_conn_ref(conn);
        LOG_INF("BLE connected");
    } else {
        LOG_WRN("BLE connect failed: %u", err);
    }
}

static void disconnected(struct bt_conn *conn, uint8_t reason)
{
    ARG_UNUSED(conn);
    s_last_disc_reason = reason;
    if (s_conn) {
        bt_conn_unref(s_conn);
        s_conn = NULL;
    }
    LOG_INF("BLE disconnected: reason=0x%02x", reason);
}

BT_CONN_CB_DEFINE(conn_cbs) = {
    .connected = connected,
    .disconnected = disconnected,
};

static int adv_start(void)
{
    const struct bt_le_adv_param adv_param = {
        .id = BT_ID_DEFAULT,
        .sid = 0,
        .secondary_max_skip = 0,
        .options = BT_LE_ADV_OPT_CONNECTABLE,
        .interval_min = BT_GAP_ADV_FAST_INT_MIN_2,
        .interval_max = BT_GAP_ADV_FAST_INT_MAX_2,
        .peer = NULL,
    };
    const struct bt_data ad[] = {
        BT_DATA_BYTES(BT_DATA_FLAGS, (BT_LE_AD_GENERAL | BT_LE_AD_NO_BREDR)),
        BT_DATA(BT_DATA_NAME_COMPLETE,
                CONFIG_BT_DEVICE_NAME,
                strlen(CONFIG_BT_DEVICE_NAME)),
    };
    const struct bt_data sd[] = {
        BT_DATA_BYTES(BT_DATA_UUID128_ALL,
                      0x6E,
                      0x40,
                      0x00,
                      0x01,
                      0xB5,
                      0xA3,
                      0xF3,
                      0x93,
                      0xE0,
                      0xA9,
                      0xE5,
                      0x0E,
                      0x24,
                      0xDC,
                      0xCA,
                      0x9E),
    };
    int rc =
        bt_le_adv_start(&adv_param, ad, ARRAY_SIZE(ad), sd, ARRAY_SIZE(sd));
    if (rc == 0) {
        s_adv_on = true;
    }
    return rc;
}

int grlc_ble_init(ble_nus_rx_cb_t rx_cb, void *user)
{
    s_rx_cb = rx_cb;
    s_rx_user = user;

    int rc = bt_enable(NULL);
    if (rc) {
        LOG_ERR("bt_enable failed: %d", rc);
        return rc;
    }

    static struct bt_nus_cb cbs;
    memset(&cbs, 0, sizeof(cbs));
    cbs.received = nus_rx_cb;
    rc = bt_nus_cb_register(&cbs, NULL);
    if (rc) {
        LOG_ERR("bt_nus_cb_register failed: %d", rc);
        return rc;
    }

    k_msleep(10);
    rc = adv_start();
    if (rc) {
        LOG_WRN("adv_start failed: %d", rc);
    }
    return 0;
}

size_t grlc_ble_send(const uint8_t *data, size_t len)
{
    if (!s_conn || !data || !len) {
        return 0;
    }
    size_t off = 0;
    while (off < len) {
        size_t take = len - off;
        if (take > NUS_CHUNK) {
            take = NUS_CHUNK;
        }
        int rc = bt_nus_send(s_conn, &data[off], (uint16_t)take);
        if (rc) {
            break;
        }
        off += take;
    }
    return off;
}

int grlc_ble_set_advertising(bool enable)
{
    if (enable) {
        if (s_adv_on) {
            return 0;
        }
        return adv_start();
    } else {
        if (!s_adv_on) {
            return 0;
        }
        int rc = bt_le_adv_stop();
        if (rc == 0) {
            s_adv_on = false;
        }
        return rc;
    }
}

void grlc_ble_get_status(bool *advertising, bool *connected)
{
    if (advertising)
        *advertising = s_adv_on;
    if (connected)
        *connected = (s_conn != NULL);
}

uint8_t grlc_ble_last_disc_reason(void)
{
    return s_last_disc_reason;
}
