#include "commands/inc/system_iface.h"

uint64_t grlc_sys_uptime_ms(void)
{
    return 123456789ULL;
}

size_t grlc_sys_flash_read(uint32_t addr, uint8_t *dst, size_t len)
{
    (void)addr;
    for (size_t i = 0; i < len; ++i) dst[i] = (uint8_t)(i & 0xFF);
    return len;
}
