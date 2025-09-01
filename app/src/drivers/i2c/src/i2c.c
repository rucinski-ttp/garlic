#include <zephyr/device.h>
#include <zephyr/drivers/i2c.h>
#include <zephyr/kernel.h>

#include "drivers/i2c/inc/i2c.h"

static const struct device *i2c_dev;

struct i2c_req_ctx {
    struct k_sem done;
    volatile int result;
};

int grlc_i2c_init(void)
{
    /* Default to &i2c0 for nRF52-DK overlay */
    i2c_dev = DEVICE_DT_GET(DT_NODELABEL(i2c0));
    if (!device_is_ready(i2c_dev)) {
        return -ENODEV;
    }
    /* Try to set a conservative speed; some stacks may not support runtime
     * configure. */
    (void)i2c_configure(
        i2c_dev, I2C_MODE_CONTROLLER | I2C_SPEED_SET(I2C_SPEED_STANDARD));
    return 0;
}

int grlc_i2c_write(uint16_t addr,
                   const uint8_t *data,
                   size_t len,
                   i2c_async_cb_t cb,
                   void *user)
{
    if (!i2c_dev || !data || len == 0)
        return -EINVAL;
    struct i2c_msg msg = {
        .buf = (uint8_t *)data,
        .len = (uint32_t)len,
        .flags = I2C_MSG_WRITE | I2C_MSG_STOP,
    };
#ifdef CONFIG_I2C_CALLBACK
    int rc = i2c_transfer_cb(i2c_dev, &msg, 1, addr, (i2c_callback_t)cb, user);
    if (rc != -ENOSYS) {
        return rc;
    }
#endif
    int res = i2c_transfer(i2c_dev, &msg, 1, addr);
    if (cb)
        cb(res, user);
    return 0;
}

int grlc_i2c_read(
    uint16_t addr, uint8_t *data, size_t len, i2c_async_cb_t cb, void *user)
{
    if (!i2c_dev || !data || len == 0)
        return -EINVAL;
    struct i2c_msg msg = {
        .buf = data,
        .len = (uint32_t)len,
        .flags = I2C_MSG_READ | I2C_MSG_STOP,
    };
#ifdef CONFIG_I2C_CALLBACK
    int rc = i2c_transfer_cb(i2c_dev, &msg, 1, addr, (i2c_callback_t)cb, user);
    if (rc != -ENOSYS) {
        return rc;
    }
#endif
    int res = i2c_transfer(i2c_dev, &msg, 1, addr);
    if (cb)
        cb(res, user);
    return 0;
}

int grlc_i2c_write_read(uint16_t addr,
                        const uint8_t *wdata,
                        size_t wlen,
                        uint8_t *rdata,
                        size_t rlen,
                        i2c_async_cb_t cb,
                        void *user)
{
    if (!i2c_dev || !wdata || wlen == 0 || !rdata || rlen == 0)
        return -EINVAL;
    struct i2c_msg msgs[2] = {
        {
            .buf = (uint8_t *)wdata,
            .len = (uint32_t)wlen,
            .flags = I2C_MSG_WRITE | I2C_MSG_RESTART,
        },
        {
            .buf = rdata,
            .len = (uint32_t)rlen,
            .flags = I2C_MSG_READ | I2C_MSG_STOP,
        },
    };
#ifdef CONFIG_I2C_CALLBACK
    int rc = i2c_transfer_cb(i2c_dev, msgs, 2, addr, (i2c_callback_t)cb, user);
    if (rc != -ENOSYS) {
        return rc;
    }
#endif
    int res = i2c_transfer(i2c_dev, msgs, 2, addr);
    if (cb)
        cb(res, user);
    return 0;
}

/* Blocking wrappers */
/**
 * @brief Wait for a semaphore with millisecond timeout.
 * @param sem        Semaphore to take.
 * @param timeout_ms Timeout in milliseconds (<=0 for no wait, <0 for forever).
 * @return 0 on success, negative on timeout.
 */
static int wait_sem(struct k_sem *sem, int timeout_ms)
{
    if (timeout_ms < 0)
        timeout_ms = K_FOREVER.ticks;
    k_timeout_t to = (timeout_ms == 0) ? K_NO_WAIT : K_MSEC(timeout_ms);
    return k_sem_take(sem, to);
}

/**
 * @brief Bridge async callback into blocking request context.
 * @param result Result code from async transfer.
 * @param user   Pointer to i2c_req_ctx with semaphore to release.
 */
static void bridge_cb(int result, void *user)
{
    struct i2c_req_ctx *ctx = (struct i2c_req_ctx *)user;
    if (ctx) {
        ctx->result = result;
        k_sem_give(&ctx->done);
    }
}

int grlc_i2c_blocking_write(uint16_t addr,
                            const uint8_t *data,
                            size_t len,
                            int timeout_ms)
{
    struct i2c_req_ctx ctx;
    k_sem_init(&ctx.done, 0, 1);
    ctx.result = -EIO;
    int rc = grlc_i2c_write(addr, data, len, bridge_cb, &ctx);
    if (rc)
        return rc;
    if (wait_sem(&ctx.done, timeout_ms) != 0)
        return -ETIMEDOUT;
    return ctx.result;
}

int grlc_i2c_blocking_read(uint16_t addr,
                           uint8_t *data,
                           size_t len,
                           int timeout_ms)
{
    struct i2c_req_ctx ctx;
    k_sem_init(&ctx.done, 0, 1);
    ctx.result = -EIO;
    int rc = grlc_i2c_read(addr, data, len, bridge_cb, &ctx);
    if (rc)
        return rc;
    if (wait_sem(&ctx.done, timeout_ms) != 0)
        return -ETIMEDOUT;
    return ctx.result;
}

int grlc_i2c_blocking_write_read(uint16_t addr,
                                 const uint8_t *wdata,
                                 size_t wlen,
                                 uint8_t *rdata,
                                 size_t rlen,
                                 int timeout_ms)
{
    struct i2c_req_ctx ctx;
    k_sem_init(&ctx.done, 0, 1);
    ctx.result = -EIO;
    int rc =
        grlc_i2c_write_read(addr, wdata, wlen, rdata, rlen, bridge_cb, &ctx);
    if (rc)
        return rc;
    if (wait_sem(&ctx.done, timeout_ms) != 0)
        return -ETIMEDOUT;
    return ctx.result;
}

int grlc_i2c_bus_recover(void)
{
    if (!i2c_dev) {
        int rc = grlc_i2c_init();
        if (rc)
            return rc;
    }
    return i2c_recover_bus(i2c_dev);
}

int grlc_i2c_ping(uint16_t addr)
{
    if (!i2c_dev) {
        int rc = grlc_i2c_init();
        if (rc)
            return rc;
    }
    uint8_t dummy = 0;
    struct i2c_msg msg = {
        .buf = &dummy,
        .len = 1,
        .flags = I2C_MSG_READ | I2C_MSG_STOP,
    };
    return i2c_transfer(i2c_dev, &msg, 1, addr);
}
