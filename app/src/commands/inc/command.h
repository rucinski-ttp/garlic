#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    CMD_STATUS_OK = 0u,
    CMD_STATUS_ERR_INVALID = 1u,
    CMD_STATUS_ERR_UNSUPPORTED = 2u,
    CMD_STATUS_ERR_BOUNDS = 3u,
    CMD_STATUS_ERR_BUSY = 4u,
    CMD_STATUS_ERR_INTERNAL = 5u,
} command_status_t;

typedef command_status_t (*command_handler_fn)(const uint8_t *req_payload, size_t req_len,
                                               uint8_t *resp_buf, size_t *resp_len);

void command_registry_init(void);
bool command_register(uint16_t cmd_id, command_handler_fn handler);
bool command_dispatch(uint16_t cmd_id, const uint8_t *in, size_t in_len, uint8_t *out,
                      size_t *out_len, uint16_t *status_out);

bool command_pack_request(uint16_t cmd_id, const uint8_t *payload, uint16_t payload_len,
                          uint8_t *out_buf, size_t out_cap, size_t *out_len);
bool command_parse_request(const uint8_t *in, size_t in_len, uint16_t *cmd_id_out,
                           const uint8_t **payload, uint16_t *payload_len);
bool command_pack_response(uint16_t cmd_id, uint16_t status, const uint8_t *payload,
                           uint16_t payload_len, uint8_t *out_buf, size_t out_cap, size_t *out_len);
bool command_parse_response(const uint8_t *in, size_t in_len, uint16_t *cmd_id_out,
                            uint16_t *status_out, const uint8_t **payload, uint16_t *payload_len);

#ifdef __cplusplus
}
#endif
