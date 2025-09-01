# I2C Async Helper

Thin wrapper around Zephyr's I2C API providing both asynchronous (callback-based) and blocking
convenience functions. When `CONFIG_I2C_CALLBACK=y` is enabled, the async callbacks are used;
otherwise calls fall back to synchronous `i2c_transfer()`.

- Public API: `drivers/i2c/inc/i2c.h` (`grlc_i2c_*`)
- No dynamic allocation on runtime paths
- Unit-tested via mocks in the test suite
