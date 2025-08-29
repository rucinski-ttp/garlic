#include "stack/cmd_transport/inc/cmd_transport.h"
#include <string.h>
#include "commands/inc/command.h"
#include "commands/inc/register_all.h"
#include "proto/inc/transport.h"

static uint8_t resp_buf[TRANSPORT_REASSEMBLY_MAX];
static struct transport_ctx *g_t = NULL;

static void transport_cb(void *user, uint16_t session, const uint8_t *msg, size_t len,
                         bool is_response)
{
    (void)user;
    if (is_response) {
        return;
    }
    uint16_t cmd_id = 0;
    const uint8_t *req = NULL;
    uint16_t req_len = 0;
    if (!command_parse_request(msg, len, &cmd_id, &req, &req_len)) {
        return;
    }
    uint16_t status = (uint16_t)CMD_STATUS_ERR_UNSUPPORTED;
    size_t out_cap = sizeof(resp_buf) - 6;
    size_t actual_len = out_cap; /* in: capacity, out: actual length */
    (void)command_dispatch(cmd_id, req, req_len, &resp_buf[6], &actual_len, &status);
    if (status != (uint16_t)CMD_STATUS_OK) {
        /* Ensure we never pack garbage on error paths */
        actual_len = 0;
    } else if (actual_len > out_cap) {
        /* Clamp to capacity if a handler misbehaves */
        actual_len = out_cap;
    }
    size_t packed_len = 0;
    command_pack_response(cmd_id, status, &resp_buf[6], (uint16_t)actual_len, resp_buf,
                          sizeof(resp_buf), &packed_len);
    if (g_t) {
        transport_send_message(g_t, session, resp_buf, packed_len, true);
    }
}

void cmd_transport_init(struct transport_ctx *t)
{
    g_t = t;
    command_registry_init();
    command_register_builtin();
}

transport_msg_cb cmd_get_transport_cb(void)
{
    return transport_cb;
}
