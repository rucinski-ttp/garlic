#include <string.h>

#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "drivers/tmp119/inc/tmp119.h"

/*
 * TMP119 command protocol (CMD_ID_TMP119 = 0x0119)
 *
 * Request:
 *   uint8_t op
 *   uint8_t addr7
 *   ... op-specific params ...
 *
 * Ops:
 *   0x00: READ_ID              -> resp: u16 id
 *   0x01: READ_TEMP_mC         -> resp: s32 milli-Celsius
 *   0x02: READ_TEMP_RAW        -> resp: u16 raw
 *   0x03: READ_CONFIG          -> resp: u16 cfg
 *   0x04: WRITE_CONFIG (u16)   -> resp: empty
 *   0x05: READ_HIGH_LIMIT      -> resp: u16
 *   0x06: WRITE_HIGH_LIMIT(u16)-> resp: empty
 *   0x07: READ_LOW_LIMIT       -> resp: u16
 *   0x08: WRITE_LOW_LIMIT(u16) -> resp: empty
 *   0x09: UNLOCK_EEPROM        -> resp: empty
 *   0x0A: READ_EEPROM(idx)     -> resp: u16
 *   0x0B: WRITE_EEPROM(idx,u16)-> resp: empty
 *   0x0C: READ_OFFSET          -> resp: u16
 *   0x0D: WRITE_OFFSET(u16)    -> resp: empty
 */

static command_status_t handle_tmp119(const uint8_t *req, size_t req_len, uint8_t *resp,
                                      size_t *resp_len)
{
    if (req_len < 2) {
        return CMD_STATUS_ERR_INVALID;
    }
    uint8_t op = req[0];
    uint8_t addr7 = req[1] & 0x7F;

    switch (op) {
        case 0x00: { /* READ_ID */
            if (*resp_len < 2) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            uint16_t id = 0;
            if (tmp119_read_device_id(addr7, &id)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(id & 0xFF);
            resp[1] = (uint8_t)((id >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x01: { /* READ_TEMP_mC */
            if (*resp_len < 4) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            int32_t mc = 0;
            if (tmp119_read_temperature_mC(addr7, &mc)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(mc & 0xFF);
            resp[1] = (uint8_t)((mc >> 8) & 0xFF);
            resp[2] = (uint8_t)((mc >> 16) & 0xFF);
            resp[3] = (uint8_t)((mc >> 24) & 0xFF);
            *resp_len = 4;
            return CMD_STATUS_OK;
        }
        case 0x02: { /* READ_TEMP_RAW */
            if (*resp_len < 2) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            uint16_t raw = 0;
            if (tmp119_read_temperature_raw(addr7, &raw)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(raw & 0xFF);
            resp[1] = (uint8_t)((raw >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x03: { /* READ_CONFIG */
            if (*resp_len < 2) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            uint16_t cfg = 0;
            if (tmp119_read_config(addr7, &cfg)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(cfg & 0xFF);
            resp[1] = (uint8_t)((cfg >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x04: { /* WRITE_CONFIG(u16) */
            if (req_len < 4) {
                return CMD_STATUS_ERR_INVALID;
            }
            uint16_t cfg = (uint16_t)req[2] | ((uint16_t)req[3] << 8);
            if (tmp119_write_config(addr7, cfg)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            *resp_len = 0;
            return CMD_STATUS_OK;
        }
        case 0x05: { /* READ_HIGH_LIMIT */
            if (*resp_len < 2) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            uint16_t v = 0;
            if (tmp119_read_high_limit(addr7, &v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(v & 0xFF);
            resp[1] = (uint8_t)((v >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x06: { /* WRITE_HIGH_LIMIT(u16) */
            if (req_len < 4) {
                return CMD_STATUS_ERR_INVALID;
            }
            uint16_t v = (uint16_t)req[2] | ((uint16_t)req[3] << 8);
            if (tmp119_write_high_limit(addr7, v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            *resp_len = 0;
            return CMD_STATUS_OK;
        }
        case 0x07: { /* READ_LOW_LIMIT */
            if (*resp_len < 2) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            uint16_t v = 0;
            if (tmp119_read_low_limit(addr7, &v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(v & 0xFF);
            resp[1] = (uint8_t)((v >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x08: { /* WRITE_LOW_LIMIT(u16) */
            if (req_len < 4) {
                return CMD_STATUS_ERR_INVALID;
            }
            uint16_t v = (uint16_t)req[2] | ((uint16_t)req[3] << 8);
            if (tmp119_write_low_limit(addr7, v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            *resp_len = 0;
            return CMD_STATUS_OK;
        }
        case 0x09: { /* UNLOCK_EEPROM */
            if (tmp119_unlock_eeprom(addr7)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            *resp_len = 0;
            return CMD_STATUS_OK;
        }
        case 0x0A: { /* READ_EEPROM(idx) */
            if (req_len < 3 || *resp_len < 2) {
                return CMD_STATUS_ERR_INVALID;
            }
            uint8_t idx = req[2];
            uint16_t v = 0;
            if (tmp119_read_eeprom(addr7, idx, &v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(v & 0xFF);
            resp[1] = (uint8_t)((v >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x0B: { /* WRITE_EEPROM(idx,u16) */
            if (req_len < 5) {
                return CMD_STATUS_ERR_INVALID;
            }
            uint8_t idx = req[2];
            uint16_t v = (uint16_t)req[3] | ((uint16_t)req[4] << 8);
            if (tmp119_write_eeprom(addr7, idx, v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            *resp_len = 0;
            return CMD_STATUS_OK;
        }
        case 0x0C: { /* READ_OFFSET */
            if (*resp_len < 2) {
                return CMD_STATUS_ERR_BOUNDS;
            }
            uint16_t v = 0;
            if (tmp119_read_offset(addr7, &v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            resp[0] = (uint8_t)(v & 0xFF);
            resp[1] = (uint8_t)((v >> 8) & 0xFF);
            *resp_len = 2;
            return CMD_STATUS_OK;
        }
        case 0x0D: { /* WRITE_OFFSET(u16) */
            if (req_len < 4) {
                return CMD_STATUS_ERR_INVALID;
            }
            uint16_t v = (uint16_t)req[2] | ((uint16_t)req[3] << 8);
            if (tmp119_write_offset(addr7, v)) {
                return CMD_STATUS_ERR_INTERNAL;
            }
            *resp_len = 0;
            return CMD_STATUS_OK;
        }
        default:
            return CMD_STATUS_ERR_INVALID;
    }
}

void command_register_tmp119(void)
{
    command_register(CMD_ID_TMP119, handle_tmp119);
}
