#include <string.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "commands/inc/system_iface.h"

#ifndef CMD_FLASH_READ_ENABLE
#define CMD_FLASH_READ_ENABLE 1
#endif

static command_status_t flash_read_handler(const uint8_t *in,
                                           size_t in_len,
                                           uint8_t *out,
                                           size_t *out_len)
{
#if CMD_FLASH_READ_ENABLE
    command_status_t st = CMD_STATUS_OK;
    size_t produced = 0;
    if (!in || in_len < 6 || !out || !out_len) {
        st = CMD_STATUS_ERR_INVALID;
    } else {
        uint32_t addr = (uint32_t)in[0] | ((uint32_t)in[1] << 8) |
                        ((uint32_t)in[2] << 16) | ((uint32_t)in[3] << 24);
        uint16_t req_len = (uint16_t)in[4] | ((uint16_t)in[5] << 8);
        if (*out_len < req_len) {
            st = CMD_STATUS_ERR_BOUNDS;
        } else {
            size_t n = grlc_sys_flash_read(addr, out, req_len);
            produced = n;
            if (n != req_len) {
                st = CMD_STATUS_ERR_INTERNAL;
            }
        }
    }
    if (out_len) {
        *out_len = produced;
    }
    return st;
#else
    (void)in;
    (void)in_len;
    (void)out;
    (void)out_len;
    return CMD_STATUS_ERR_UNSUPPORTED;
#endif
}

void grlc_cmd_register_flash_read(void)
{
    (void)grlc_cmd_register(CMD_ID_FLASH_READ, flash_read_handler);
}
