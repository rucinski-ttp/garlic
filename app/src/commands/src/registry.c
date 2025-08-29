#include <string.h>
#include "commands/inc/command.h"

#ifndef CMD_REGISTRY_MAX
#define CMD_REGISTRY_MAX 32
#endif

typedef struct {
    uint16_t cmd_id;
    command_handler_fn fn;
} entry_t;

static entry_t reg_tbl[CMD_REGISTRY_MAX];
static size_t reg_count = 0;

void command_registry_init(void)
{
    memset(reg_tbl, 0, sizeof(reg_tbl));
    reg_count = 0;
}

bool command_register(uint16_t cmd_id, command_handler_fn handler)
{
    if (!handler) {
        return false;
    }
    for (size_t i = 0; i < reg_count; ++i) {
        if (reg_tbl[i].cmd_id == cmd_id) {
            return false;
        }
    }
    if (reg_count >= CMD_REGISTRY_MAX) {
        return false;
    }
    reg_tbl[reg_count].cmd_id = cmd_id;
    reg_tbl[reg_count].fn = handler;
    reg_count++;
    return true;
}

bool command_dispatch(uint16_t cmd_id, const uint8_t *in, size_t in_len, uint8_t *out,
                      size_t *out_len, uint16_t *status_out)
{
    for (size_t i = 0; i < reg_count; ++i) {
        if (reg_tbl[i].cmd_id == cmd_id) {
            size_t cap = out_len ? *out_len : 0;
            command_status_t st = reg_tbl[i].fn(in, in_len, out, out_len ? out_len : &cap);
            if (status_out) {
                *status_out = (uint16_t)st;
            }
            return true;
        }
    }
    if (status_out) {
        *status_out = (uint16_t)CMD_STATUS_ERR_UNSUPPORTED;
    }
    return false;
}

bool command_pack_request(uint16_t cmd_id, const uint8_t *payload, uint16_t payload_len,
                          uint8_t *out_buf, size_t out_cap, size_t *out_len)
{
    size_t need = 2 + 2 + payload_len;
    if (out_cap < need) {
        return false;
    }
    out_buf[0] = (uint8_t)(cmd_id & 0xFF);
    out_buf[1] = (uint8_t)(cmd_id >> 8);
    out_buf[2] = (uint8_t)(payload_len & 0xFF);
    out_buf[3] = (uint8_t)(payload_len >> 8);
    if (payload_len && payload) {
        memcpy(&out_buf[4], payload, payload_len);
    }
    if (out_len) {
        *out_len = need;
    }
    return true;
}

bool command_parse_request(const uint8_t *in, size_t in_len, uint16_t *cmd_id_out,
                           const uint8_t **payload, uint16_t *payload_len)
{
    if (!in || in_len < 4) {
        return false;
    }
    uint16_t cmd = (uint16_t)in[0] | ((uint16_t)in[1] << 8);
    uint16_t len = (uint16_t)in[2] | ((uint16_t)in[3] << 8);
    if (in_len < (size_t)(4 + len)) {
        return false;
    }
    if (cmd_id_out) {
        *cmd_id_out = cmd;
    }
    if (payload) {
        *payload = &in[4];
    }
    if (payload_len) {
        *payload_len = len;
    }
    return true;
}

bool command_pack_response(uint16_t cmd_id, uint16_t status, const uint8_t *payload,
                           uint16_t payload_len, uint8_t *out_buf, size_t out_cap, size_t *out_len)
{
    size_t need = 2 + 2 + 2 + payload_len;
    if (out_cap < need) {
        return false;
    }
    out_buf[0] = (uint8_t)(cmd_id & 0xFF);
    out_buf[1] = (uint8_t)(cmd_id >> 8);
    out_buf[2] = (uint8_t)(status & 0xFF);
    out_buf[3] = (uint8_t)(status >> 8);
    out_buf[4] = (uint8_t)(payload_len & 0xFF);
    out_buf[5] = (uint8_t)(payload_len >> 8);
    if (payload_len && payload) {
        memcpy(&out_buf[6], payload, payload_len);
    }
    if (out_len) {
        *out_len = need;
    }
    return true;
}

bool command_parse_response(const uint8_t *in, size_t in_len, uint16_t *cmd_id_out,
                            uint16_t *status_out, const uint8_t **payload, uint16_t *payload_len)
{
    if (!in || in_len < 6) {
        return false;
    }
    uint16_t cmd = (uint16_t)in[0] | ((uint16_t)in[1] << 8);
    uint16_t st = (uint16_t)in[2] | ((uint16_t)in[3] << 8);
    uint16_t len = (uint16_t)in[4] | ((uint16_t)in[5] << 8);
    if (in_len < (size_t)(6 + len)) {
        return false;
    }
    if (cmd_id_out) {
        *cmd_id_out = cmd;
    }
    if (status_out) {
        *status_out = st;
    }
    if (payload) {
        *payload = &in[6];
    }
    if (payload_len) {
        *payload_len = len;
    }
    return true;
}
