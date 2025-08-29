#include <string.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"

static command_status_t echo_handler(const uint8_t *in, size_t in_len, uint8_t *out,
                                     size_t *out_len)
{
    if (!out || !out_len) {
        return CMD_STATUS_ERR_INVALID;
    }
    if (*out_len < in_len) {
        return CMD_STATUS_ERR_BOUNDS;
    }
    if (in && in_len) {
        memcpy(out, in, in_len);
    }
    *out_len = in_len;
    return CMD_STATUS_OK;
}

void command_register_echo(void)
{
    (void)command_register(CMD_ID_ECHO, echo_handler);
}
