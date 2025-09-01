#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Register all built-in command handlers.
 *
 * Initializes the command registry and adds the core handlers (git version,
 * uptime, flash_read, reboot, echo, I2C, TMP119, BLE control). This is safe
 * to call once after startup.
 */
void grlc_cmd_register_builtin(void);

#ifdef __cplusplus
}
#endif
