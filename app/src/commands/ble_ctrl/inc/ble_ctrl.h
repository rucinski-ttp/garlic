/**
 * @brief Register BLE control command handler.
 *
 * Provides a command under CMD_ID_BLE_CTRL (0x0200) with the following ops:
 *  - op=0x00 GET_STATUS -> response payload: [adv:1][conn:1][last_disc_reason:1]
 *  - op=0x01 SET_ADV [en:1] -> response payload: empty (status indicates result)
 */

#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Registers the BLE control command.
 */
void grlc_cmd_register_ble_ctrl(void);

#ifdef __cplusplus
}
#endif
