#include "stack/cmd_transport/inc/cmd_transport.h"
#include <string.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "commands/inc/register_all.h"
#include "proto/inc/transport.h"
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cmd_transport, LOG_LEVEL_INF);
#endif

static void transport_cb(void *user, uint16_t session, const uint8_t *msg, size_t len,
                         bool is_response)
{
    struct cmd_transport_binding *b = (struct cmd_transport_binding *)user;
    bool ok = true;
    uint16_t cmd_id = 0;
    const uint8_t *req = NULL;
    uint16_t req_len = 0;

    if (!b || !b->t || is_response) {
        ok = false;
    }
    if (ok) {
        if (!grlc_cmd_parse_request(msg, len, &cmd_id, &req, &req_len)) {
            ok = false;
        }
    }

#ifdef __ZEPHYR__
    if (ok) {
        if (cmd_id == CMD_ID_ECHO) {
            LOG_INF("CMD RX ECHO len=%u", (unsigned)req_len);
        } else {
            LOG_DBG("cmd rx id=0x%04x len=%u", cmd_id, (unsigned)req_len);
        }
    }
#endif

    if (ok) {
#ifdef __ZEPHYR__
        k_mutex_lock(&b->lock, K_FOREVER);
#endif
        uint16_t status = (uint16_t)CMD_STATUS_ERR_UNSUPPORTED;
        size_t out_cap = sizeof(b->resp_buf) - 6;
        size_t actual_len = out_cap; /* in: capacity, out: actual length */
        (void)grlc_cmd_dispatch(cmd_id, req, req_len, &b->resp_buf[6], &actual_len, &status);
        if (status != (uint16_t)CMD_STATUS_OK) {
            actual_len = 0;
        } else if (actual_len > out_cap) {
            actual_len = out_cap;
        }
        size_t packed_len = 0;
        grlc_cmd_pack_response(cmd_id, status, &b->resp_buf[6], (uint16_t)actual_len, b->resp_buf,
                               sizeof(b->resp_buf), &packed_len);
        if (!grlc_transport_send_message(b->t, session, b->resp_buf, packed_len, true)) {
            /* Transport busy: make a copy of the packed response */
            if (packed_len <= sizeof(b->pending_buf)) {
                memcpy(b->pending_buf, b->resp_buf, packed_len);
                b->pending = true;
                b->pending_session = session;
                b->pending_len = packed_len;
            }
        } else {
            /* Non-blocking: nudge TX pump immediately to reduce latency */
            grlc_transport_tx_pump(b->t);
            grlc_transport_tx_pump(b->t);
            grlc_transport_tx_pump(b->t);
        }
#ifdef __ZEPHYR__
        if (cmd_id == CMD_ID_ECHO) {
            LOG_INF("CMD TX ECHO len=%u", (unsigned)actual_len);
        }
        k_mutex_unlock(&b->lock);
#endif
    }
}

void grlc_cmd_transport_init(void)
{
    static bool s_inited = false;
    if (!s_inited) {
        grlc_cmd_registry_init();
        grlc_cmd_register_builtin();
        s_inited = true;
    }
}

void grlc_cmd_transport_bind(struct cmd_transport_binding *b, struct transport_ctx *t)
{
    if (!b) {
        return;
    }
    memset(b, 0, sizeof(*b));
    b->t = t;
    b->pending = false;
#ifdef __ZEPHYR__
    k_mutex_init(&b->lock);
#endif
}

transport_msg_cb grlc_cmd_get_transport_cb(void)
{
    return transport_cb;
}

void grlc_cmd_transport_tick(struct cmd_transport_binding *b)
{
    if (!b || !b->t)
        return;
    if (b->pending && !b->t->tx_in_progress) {
        if (grlc_transport_send_message(b->t, b->pending_session, b->pending_buf, b->pending_len,
                                        true)) {
            b->pending = false;
            grlc_transport_tx_pump(b->t);
            grlc_transport_tx_pump(b->t);
            grlc_transport_tx_pump(b->t);
        }
    }
}
