#pragma once
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t cmd_sys_uptime_ms(void);
size_t cmd_sys_flash_read(uint32_t addr, uint8_t *dst, size_t len);

#ifdef __cplusplus
}
#endif
