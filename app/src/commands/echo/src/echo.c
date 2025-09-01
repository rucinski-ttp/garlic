#include <string.h>
#ifdef __ZEPHYR__
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cmd_echo, LOG_LEVEL_INF);
#endif
#include "commands/inc/command.h"
#include "commands/inc/ids.h"

static command_status_t
echo_handler(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    command_status_t st = CMD_STATUS_OK;
    if (!out || !out_len) {
        st = CMD_STATUS_ERR_INVALID;
    } else if (*out_len < in_len) {
        st = CMD_STATUS_ERR_BOUNDS;
    } else {
#ifdef __ZEPHYR__
        if (in && in_len) {
            LOG_INF("ECHO handler len=%u first=%02x", (unsigned)in_len, in[0]);
        }
#endif
        if (in && in_len) {
            memcpy(out, in, in_len);
        }
        *out_len = in_len;
    }
    return st;
}

void grlc_cmd_register_echo(void)
{
#ifdef __ZEPHYR__
    LOG_INF("Registering ECHO command (0x%04x)", (unsigned)CMD_ID_ECHO);
#endif
    (void)grlc_cmd_register(CMD_ID_ECHO, echo_handler);
}
