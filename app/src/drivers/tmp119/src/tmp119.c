#include <errno.h>
#include <stdbool.h>
#include <string.h>
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(tmp119, CONFIG_LOG_DEFAULT_LEVEL);
#else
#define LOG_DBG(...)
#define LOG_INF(...)
#define LOG_WRN(...)
#define LOG_ERR(...)
#endif

#include <stdio.h>
#include "drivers/i2c/inc/i2c.h"
#include "drivers/tmp119/inc/tmp119.h"
#include "utils/assert/inc/project_assert.h"

static inline int addr7_to_zephyr(uint8_t addr7)
{
    /* Zephyr expects 7-bit address in i2c_transfer APIs; pass-through. */
    return (int)addr7;
}

/* Expected device ID (Section 8.5.11, p.33) */
#define TMP119_DEVICE_ID_EXPECTED 0x2117u

/* Default configuration (Section 8.5.3): continuous conversion, comparator mode,
 * typical defaults. Exact field mapping: refer to datasheet 8.5.3.
 */
#define TMP119_CONFIG_DEFAULT 0x0000u

static bool s_verified[128];

static int reg_read16(uint8_t addr7, uint8_t reg, uint16_t *out)
{
    uint8_t rx[2] = {0};
    int rc = grlc_i2c_blocking_write_read(addr7_to_zephyr(addr7), &reg, 1, rx, 2, 100);
    if (rc)
        return rc;
    /* TMP119 returns MSB first; assemble big-endian to host */
    *out = ((uint16_t)rx[0] << 8) | rx[1];
    return 0;
}

static int reg_write16(uint8_t addr7, uint8_t reg, uint16_t val)
{
    uint8_t tx[3];
    tx[0] = reg;
    tx[1] = (uint8_t)((val >> 8) & 0xFF);
    tx[2] = (uint8_t)(val & 0xFF);
    return grlc_i2c_blocking_write(addr7_to_zephyr(addr7), tx, sizeof(tx), 100);
}

int grlc_tmp119_init(void)
{
    return grlc_i2c_init();
}

static int tmp119_try_init_addr(uint8_t addr7)
{
    uint16_t id = 0;
    int rc = reg_read16(addr7, TMP119_REG_DEVICE_ID, &id);
    if (rc)
        return rc;
    if (id != TMP119_DEVICE_ID_EXPECTED) {
        return -ENODEV;
    }
    /* Apply default configuration to ensure continuous conversions (8.5.3) */
    rc = reg_write16(addr7, TMP119_REG_CONFIG, TMP119_CONFIG_DEFAULT);
    if (rc)
        return rc;
    s_verified[addr7 & 0x7F] = true;
    LOG_INF("TMP119 @0x%02x initialized (ID=0x%04x)", addr7, id);
    return 0;
}

int grlc_tmp119_boot_init(void)
{
    int rc = grlc_tmp119_init();
    if (rc)
        return rc;
    int count = 0;
    for (uint8_t a = 0x48; a <= 0x4B; ++a) {
        /* If address already verified, skip */
        if (s_verified[a]) {
            count++;
            continue;
        }
        /* Ping first to avoid delays */
        if (grlc_i2c_ping(a) != 0) {
            continue;
        }
        if (tmp119_try_init_addr(a) == 0) {
            count++;
        }
    }
    return count;
}

static void ensure_initialized(uint8_t addr7)
{
    if (!s_verified[addr7 & 0x7F]) {
        int rc = grlc_tmp119_init();
        if (rc == 0) {
            (void)tmp119_try_init_addr(addr7);
        }
    }
    if (!s_verified[addr7 & 0x7F]) {
        char msg[64];
        snprintf(msg, sizeof msg, "TMP119 not initialized at 0x%02x", addr7 & 0x7F);
        grlc_assert_fatal(msg);
    }
}

int grlc_tmp119_read_device_id(uint8_t addr7, uint16_t *id_out)
{
    if (!id_out)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, TMP119_REG_DEVICE_ID, id_out);
}

int grlc_tmp119_read_temperature_raw(uint8_t addr7, uint16_t *raw_out)
{
    if (!raw_out)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, TMP119_REG_TEMPERATURE, raw_out);
}

int grlc_tmp119_read_temperature_mC(uint8_t addr7, int32_t *mC_out)
{
    if (!mC_out)
        return -EINVAL;
    ensure_initialized(addr7);
    uint16_t raw = 0;
    int rc = reg_read16(addr7, TMP119_REG_TEMPERATURE, &raw);
    if (rc)
        return rc;
    /* Two's complement in 16-bit, LSB = 1/128 °C */
    int16_t sraw = (int16_t)raw;
    int32_t mc = ((int32_t)sraw * 1000) / 128; /* truncate toward zero */
    *mC_out = mc;
    return 0;
}

int grlc_tmp119_read_config(uint8_t addr7, uint16_t *cfg_out)
{
    if (!cfg_out)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, TMP119_REG_CONFIG, cfg_out);
}

int grlc_tmp119_write_config(uint8_t addr7, uint16_t cfg)
{
    ensure_initialized(addr7);
    return reg_write16(addr7, TMP119_REG_CONFIG, cfg);
}

int grlc_tmp119_read_high_limit(uint8_t addr7, uint16_t *val_out)
{
    if (!val_out)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, TMP119_REG_HIGH_LIMIT, val_out);
}

int grlc_tmp119_write_high_limit(uint8_t addr7, uint16_t val)
{
    ensure_initialized(addr7);
    return reg_write16(addr7, TMP119_REG_HIGH_LIMIT, val);
}

int grlc_tmp119_read_low_limit(uint8_t addr7, uint16_t *val_out)
{
    if (!val_out)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, TMP119_REG_LOW_LIMIT, val_out);
}

int grlc_tmp119_write_low_limit(uint8_t addr7, uint16_t val)
{
    ensure_initialized(addr7);
    return reg_write16(addr7, TMP119_REG_LOW_LIMIT, val);
}

int grlc_tmp119_unlock_eeprom(uint8_t addr7)
{
    ensure_initialized(addr7);
    return reg_write16(addr7, TMP119_REG_EE_UNLOCK, 0x0001);
}

static inline uint8_t ee_index_to_reg(uint8_t index)
{
    switch (index) {
        case 1:
            return TMP119_REG_EE1;
        case 2:
            return TMP119_REG_EE2;
        case 3:
            return TMP119_REG_EE3; /* Note: non-contiguous at 0x08 */
        default:
            return 0xFF;
    }
}

int grlc_tmp119_read_eeprom(uint8_t addr7, uint8_t index, uint16_t *val_out)
{
    if (!val_out)
        return -EINVAL;
    uint8_t reg = ee_index_to_reg(index);
    if (reg == 0xFF)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, reg, val_out);
}

int grlc_tmp119_write_eeprom(uint8_t addr7, uint8_t index, uint16_t val)
{
    uint8_t reg = ee_index_to_reg(index);
    if (reg == 0xFF)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_write16(addr7, reg, val);
}

int grlc_tmp119_read_offset(uint8_t addr7, uint16_t *val_out)
{
    if (!val_out)
        return -EINVAL;
    ensure_initialized(addr7);
    return reg_read16(addr7, TMP119_REG_TEMP_OFFSET, val_out);
}

int grlc_tmp119_write_offset(uint8_t addr7, uint16_t val)
{
    ensure_initialized(addr7);
    return reg_write16(addr7, TMP119_REG_TEMP_OFFSET, val);
}

void grlc_tmp119_require_initialized(uint8_t addr7)
{
    ensure_initialized(addr7);
}
