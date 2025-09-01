#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "drivers/ble_nus/inc/ble_nus.h"

/* Payload definitions:
 * op=0x00: GET_STATUS -> resp: [adv:1][conn:1]
 * op=0x01: SET_ADV [en:1] -> resp: empty
 */

static command_status_t ble_ctrl_handler(const uint8_t *req_payload, size_t req_len,
                                         uint8_t *resp_buf, size_t *resp_len)
{
    if (!req_payload || req_len < 1) {
        if (resp_len) {
            *resp_len = 0;
        }
        return CMD_STATUS_ERR_INVALID;
    }
    uint8_t op = req_payload[0];
    switch (op) {
        case 0x00: { /* GET_STATUS */
            bool adv = false, conn = false;
            ble_nus_get_status(&adv, &conn);
            if (!resp_buf || !resp_len || *resp_len < 2) {
                if (resp_len) {
                    *resp_len = 0;
                }
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp_buf[0] = adv ? 1u : 0u;
            resp_buf[1] = conn ? 1u : 0u;
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x01: { /* SET_ADV */
            if (req_len < 2) {
                if (resp_len) {
                    *resp_len = 0;
                }
                return CMD_STATUS_ERR_INVALID;
            }
            bool en = (req_payload[1] != 0);
            int rc = ble_nus_set_advertising(en);
            if (resp_len) {
                *resp_len = 0;
            }
            return (rc == 0) ? CMD_STATUS_OK : CMD_STATUS_ERR_INTERNAL;
        }
        default:
            if (resp_len) {
                *resp_len = 0;
            }
            return CMD_STATUS_ERR_UNSUPPORTED;
    }
}

void command_register_ble_ctrl(void)
{
    (void)command_register(CMD_ID_BLE_CTRL, ble_ctrl_handler);
}
