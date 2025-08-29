#include <string.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"

#ifndef GARLIC_GIT_HASH
#define GARLIC_GIT_HASH "unknown"
#endif

static command_status_t git_version_handler(const uint8_t *in, size_t in_len, uint8_t *out,
                                            size_t *out_len)
{
    (void)in;
    (void)in_len;
    const char *hash = GARLIC_GIT_HASH;
    size_t need = strlen(hash);
    if (!out || !out_len) {
        return CMD_STATUS_ERR_INVALID;
    }
    if (*out_len < need) {
        return CMD_STATUS_ERR_BOUNDS;
    }
    memcpy(out, hash, need);
    *out_len = need;
    return CMD_STATUS_OK;
}

void command_register_git_version(void)
{
    (void)command_register(CMD_ID_GET_GIT_VERSION, git_version_handler);
}
