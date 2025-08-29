#pragma once
#include <stdint.h>

enum {
    CMD_ID_GET_GIT_VERSION = 0x0001,
    CMD_ID_GET_UPTIME = 0x0002,
    CMD_ID_FLASH_READ = 0x0003,
    CMD_ID_REBOOT = 0x0004,
    CMD_ID_ECHO = 0x0005,
};
