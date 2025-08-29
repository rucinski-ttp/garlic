
#include <string.h>
#ifdef __ZEPHYR__
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(cmd_i2c, CONFIG_LOG_DEFAULT_LEVEL);
#define I2C_LOG_ERR(...) LOG_ERR(__VA_ARGS__)
#else
#define I2C_LOG_ERR(...)
#endif

#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "drivers/i2c_async/inc/i2c_async.h"

/* Request format:
 *  uint8_t  op       (0=write, 1=read, 2=write_read)
 *  uint8_t  addr7    (7-bit I2C address)
 *  uint16_t wlen
 *  uint16_t rlen
 *  uint8_t  wdata[wlen] (present if op==0 or 2)
 * Response:
 *  - write: empty
 *  - read or write_read: rdata[rlen]
 */

static command_status_t handle_i2c(const uint8_t *req, size_t req_len, uint8_t *resp,
                                   size_t *resp_len)
{
    if (req_len < 2) {
        return CMD_STATUS_ERR_INVALID;
    }
    uint8_t op = req[0];
    uint8_t addr7 = req[1] & 0x7F;
    uint16_t wlen = 0, rlen = 0;
    const uint8_t *wdata = NULL;
    if (op <= 2) {
        if (req_len < 6) {
            return CMD_STATUS_ERR_INVALID;
        }
        wlen = (uint16_t)req[2] | ((uint16_t)req[3] << 8);
        rlen = (uint16_t)req[4] | ((uint16_t)req[5] << 8);
        wdata = (req_len >= 6 + wlen) ? &req[6] : NULL;
        if ((op == 0 || op == 2) && (wdata == NULL)) {
            return CMD_STATUS_ERR_INVALID;
        }
    }
    int rc = i2c_async_init();
    if (rc) {
        I2C_LOG_ERR("i2c init failed: %d", rc);
        return CMD_STATUS_ERR_INTERNAL;
    }
    /* Optional bus recovery before activity */
    (void)i2c_bus_recover();
    int timeout_ms = 100; /* short blocking window backed by async */
    if (op == 0) {
        rc = i2c_blocking_write(addr7, wdata, wlen, timeout_ms);
        if (rc) {
            I2C_LOG_ERR("i2c write addr=0x%02x rc=%d", addr7, rc);
            return CMD_STATUS_ERR_INTERNAL;
        }
        *resp_len = 0;
        return CMD_STATUS_OK;
    } else if (op == 1) {
        if (*resp_len < rlen) {
            return CMD_STATUS_ERR_BOUNDS;
        }
        rc = i2c_blocking_read(addr7, resp, rlen, timeout_ms);
        if (rc) {
            I2C_LOG_ERR("i2c read addr=0x%02x len=%u rc=%d", addr7, (unsigned)rlen, rc);
            return CMD_STATUS_ERR_INTERNAL;
        }
        *resp_len = rlen;
        return CMD_STATUS_OK;
    } else if (op == 2) { /* write_read */
        if (*resp_len < rlen) {
            return CMD_STATUS_ERR_BOUNDS;
        }
        rc = i2c_blocking_write_read(addr7, wdata, wlen, resp, rlen, timeout_ms);
        if (rc) {
            I2C_LOG_ERR("i2c wr rd addr=0x%02x wlen=%u rlen=%u rc=%d", addr7, (unsigned)wlen,
                        (unsigned)rlen, rc);
            return CMD_STATUS_ERR_INTERNAL;
        }
        *resp_len = rlen;
        return CMD_STATUS_OK;
    } else if (op == 0x10) { /* scan */
        size_t cap = *resp_len;
        size_t n = 0;
        if (cap == 0) {
            return CMD_STATUS_ERR_BOUNDS;
        }
        resp[0] = 0;
        for (uint16_t a = 0x03; a <= 0x77; ++a) {
            if (i2c_ping(a) == 0) {
                if (1 + n < cap) {
                    resp[1 + n] = (uint8_t)a;
                    n++;
                }
            }
        }
        resp[0] = (uint8_t)n;
        *resp_len = (n + 1 <= cap) ? (n + 1) : cap;
        return CMD_STATUS_OK;
    }
    return CMD_STATUS_ERR_INVALID;
}

void command_register_i2c(void)
{
    command_register(CMD_ID_I2C_TRANSFER, handle_i2c);
}
