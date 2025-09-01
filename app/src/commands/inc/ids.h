#pragma once
#include <stdint.h>

/** @brief Command IDs used in the command layer payloads. */
enum {
    CMD_ID_GET_GIT_VERSION = 0x0001, /**< Return build git hash string */
    CMD_ID_GET_UPTIME = 0x0002,      /**< Return uptime in milliseconds */
    CMD_ID_FLASH_READ = 0x0003,      /**< Read whitelisted flash region */
    CMD_ID_REBOOT = 0x0004,          /**< Reboot the device */
    CMD_ID_ECHO = 0x0005,            /**< Echo request payload */
    CMD_ID_I2C_TRANSFER = 0x0100,    /**< I2C write/read operations */
    CMD_ID_TMP119 = 0x0119,          /**< Texas Instruments TMP119 helpers */
    CMD_ID_BLE_CTRL = 0x0200,        /**< BLE control (advertising, status) */
};
