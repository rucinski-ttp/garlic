/* Simple configurable I2C mock to emulate TMP119 behaviour. */
#include <string.h>
#include <stdint.h>
#include <stddef.h>
#include <errno.h>

#include "drivers/i2c/inc/i2c.h"

static uint8_t g_present_addr = 0x48;    /* Address that ACKs */
static int16_t g_temp_raw = 0x0C80;      /* 25.000 C */
static uint16_t g_dev_id = 0x2117;       /* Expected device ID */

void i2c_mock_set_present_addr(uint8_t a) { g_present_addr = a & 0x7F; }
void i2c_mock_set_temp_raw(int16_t raw) { g_temp_raw = raw; }
void i2c_mock_set_dev_id(uint16_t id) { g_dev_id = id; }

int grlc_i2c_init(void) { return 0; }

int grlc_i2c_write(uint16_t addr, const uint8_t *data, size_t len, i2c_async_cb_t cb, void *user)
{ if (cb) cb(0, user); return 0; }

int grlc_i2c_read(uint16_t addr, uint8_t *data, size_t len, i2c_async_cb_t cb, void *user)
{ if (data && len) memset(data, 0, len); if (cb) cb(0, user); return 0; }

int grlc_i2c_write_read(uint16_t addr,
                         const uint8_t *wdata, size_t wlen,
                         uint8_t *rdata, size_t rlen,
                         i2c_async_cb_t cb, void *user)
{
    if ((addr & 0x7F) != g_present_addr) {
        if (cb) cb(-ENODEV, user);
        return -ENODEV;
    }
    if (wdata && wlen >= 1 && rdata && rlen >= 2) {
        uint8_t reg = wdata[0];
        if (reg == 0x0F) {
            rdata[0] = (uint8_t)(g_dev_id >> 8);
            rdata[1] = (uint8_t)(g_dev_id & 0xFF);
        } else if (reg == 0x00) {
            rdata[0] = (uint8_t)((g_temp_raw >> 8) & 0xFF);
            rdata[1] = (uint8_t)(g_temp_raw & 0xFF);
        } else {
            memset(rdata, 0, rlen);
        }
    }
    if (cb) cb(0, user);
    return 0;
}

int grlc_i2c_blocking_write(uint16_t addr, const uint8_t *data, size_t len, int timeout_ms)
{ return ((addr & 0x7F) == g_present_addr) ? 0 : -ENODEV; }

int grlc_i2c_blocking_read(uint16_t addr, uint8_t *data, size_t len, int timeout_ms)
{ if (data && len) memset(data, 0, len); return ((addr & 0x7F) == g_present_addr) ? 0 : -ENODEV; }

int grlc_i2c_blocking_write_read(uint16_t addr,
                            const uint8_t *wdata, size_t wlen,
                            uint8_t *rdata, size_t rlen,
                            int timeout_ms)
{
    if ((addr & 0x7F) != g_present_addr) return -ENODEV;
    if (wdata && wlen >= 1 && rdata && rlen >= 2) {
        uint8_t reg = wdata[0];
        if (reg == 0x0F) {
            rdata[0] = (uint8_t)(g_dev_id >> 8);
            rdata[1] = (uint8_t)(g_dev_id & 0xFF);
        } else if (reg == 0x00) {
            rdata[0] = (uint8_t)((g_temp_raw >> 8) & 0xFF);
            rdata[1] = (uint8_t)(g_temp_raw & 0xFF);
        } else {
            memset(rdata, 0, rlen);
        }
    }
    return 0;
}

int grlc_i2c_bus_recover(void) { return 0; }

int grlc_i2c_ping(uint16_t addr) { return ((addr & 0x7F) == g_present_addr) ? 0 : -ENODEV; }
