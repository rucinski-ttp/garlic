#include <zephyr/device.h>
#include <zephyr/drivers/flash.h>
#include <zephyr/kernel.h>
#include "commands/inc/system_iface.h"

uint64_t grlc_sys_uptime_ms(void)
{
    return (uint64_t)k_uptime_get();
}

size_t grlc_sys_flash_read(uint32_t addr, uint8_t *dst, size_t len)
{
#if defined(CONFIG_FLASH)
    const struct device *flash =
        DEVICE_DT_GET(DT_CHOSEN(zephyr_flash_controller));
    if (!device_is_ready(flash)) {
        return 0;
    }
    int rc = flash_read(flash, (off_t)addr, dst, len);
    if (rc == 0) {
        return len;
    }
    return 0;
#else
    (void)addr;
    (void)dst;
    (void)len;
    return 0;
#endif
}
