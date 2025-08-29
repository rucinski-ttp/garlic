#include <string.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "commands/inc/system_iface.h"

#ifndef CMD_FLASH_READ_ENABLE
#define CMD_FLASH_READ_ENABLE 1
#endif

static command_status_t flash_read_handler(const uint8_t *in, size_t in_len, uint8_t *out,
                                           size_t *out_len)
{
#if CMD_FLASH_READ_ENABLE
    if (!in || in_len < 6 || !out || !out_len) {
        return CMD_STATUS_ERR_INVALID;
    }
    uint32_t addr = (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
                    ((uint32_t)in[3] << 24);
    uint16_t len = (uint16_t)in[4] | ((uint16_t)in[5] << 8);
    if (*out_len < len) {
        return CMD_STATUS_ERR_BOUNDS;
    }
    size_t n = cmd_sys_flash_read(addr, out, len);
    *out_len = (size_t)n;
    return (n == len) ? CMD_STATUS_OK : CMD_STATUS_ERR_INTERNAL;
#else
    (void)in;
    (void)in_len;
    (void)out;
    (void)out_len;
    return CMD_STATUS_ERR_UNSUPPORTED;
#endif
}

void command_register_flash_read(void)
{
    (void)command_register(CMD_ID_FLASH_READ, flash_read_handler);
}
