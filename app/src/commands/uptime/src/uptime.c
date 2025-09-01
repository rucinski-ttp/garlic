#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "commands/inc/system_iface.h"

static command_status_t
uptime_handler(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    (void)in;
    (void)in_len;
    command_status_t st = CMD_STATUS_OK;
    size_t produced = 0;
    if (!out || !out_len || *out_len < 8) {
        st = CMD_STATUS_ERR_BOUNDS;
    } else {
        uint64_t ms = grlc_sys_uptime_ms();
        out[0] = (uint8_t)(ms & 0xFF);
        out[1] = (uint8_t)((ms >> 8) & 0xFF);
        out[2] = (uint8_t)((ms >> 16) & 0xFF);
        out[3] = (uint8_t)((ms >> 24) & 0xFF);
        out[4] = (uint8_t)((ms >> 32) & 0xFF);
        out[5] = (uint8_t)((ms >> 40) & 0xFF);
        out[6] = (uint8_t)((ms >> 48) & 0xFF);
        out[7] = (uint8_t)((ms >> 56) & 0xFF);
        produced = 8;
    }
    if (out_len) {
        *out_len = produced;
    }
    return st;
}

void grlc_cmd_register_uptime(void)
{
    (void)grlc_cmd_register(CMD_ID_GET_UPTIME, uptime_handler);
}
