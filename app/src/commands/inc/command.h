/**
 * @file command.h
 * @brief Command registry, packing and parsing helpers.
 *
 * Public API for registering command handlers and packing/parsing
 * request/response payloads carried by the transport layer.
 */
#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Common status codes returned by command handlers.
 */
typedef enum {
    CMD_STATUS_OK = 0u,
    CMD_STATUS_ERR_INVALID = 1u,
    CMD_STATUS_ERR_UNSUPPORTED = 2u,
    CMD_STATUS_ERR_BOUNDS = 3u,
    CMD_STATUS_ERR_BUSY = 4u,
    CMD_STATUS_ERR_INTERNAL = 5u,
} command_status_t;

/**
 * @brief Command handler function signature.
 * @param req_payload Pointer to request payload bytes (may be NULL if req_len==0).
 * @param req_len Request payload length in bytes.
 * @param resp_buf Output buffer to place response payload.
 * @param resp_len In/out: on entry, capacity of resp_buf; on return, bytes written.
 * @return Status code indicating success or specific failure.
 */
typedef command_status_t (*command_handler_fn)(const uint8_t *req_payload, size_t req_len,
                                               uint8_t *resp_buf, size_t *resp_len);

/**
 * @brief Initialize the global command registry.
 */
void grlc_cmd_registry_init(void);
/**
 * @brief Register a handler for a command ID.
 * @param cmd_id 16-bit command identifier.
 * @param handler Function to invoke on requests.
 * @return true on success, false if already registered or invalid.
 */
bool grlc_cmd_register(uint16_t cmd_id, command_handler_fn handler);
/**
 * @brief Dispatch a request to a registered handler and produce a response.
 * @param cmd_id Command identifier.
 * @param in Request payload pointer (may be NULL if in_len==0).
 * @param in_len Request payload length.
 * @param out Response payload buffer.
 * @param out_len In/out capacity/result length.
 * @param status_out Filled with handler status.
 * @return true if dispatch executed (handler found and packing succeeded).
 */
bool grlc_cmd_dispatch(uint16_t cmd_id, const uint8_t *in, size_t in_len, uint8_t *out,
                       size_t *out_len, uint16_t *status_out);

/**
 * @brief Pack a request message payload for transport.
 * @param cmd_id Command identifier.
 * @param payload Pointer to payload bytes (may be NULL if payload_len==0).
 * @param payload_len Payload length in bytes.
 * @param out_buf Output buffer to receive packed request.
 * @param out_cap Capacity of out_buf in bytes.
 * @param out_len Filled with number of bytes written to out_buf.
 * @return true on success, false on invalid arguments or insufficient capacity.
 */
bool grlc_cmd_pack_request(uint16_t cmd_id, const uint8_t *payload, uint16_t payload_len,
                           uint8_t *out_buf, size_t out_cap, size_t *out_len);
/**
 * @brief Parse a request message payload extracted from transport.
 * @param in Pointer to packed request bytes.
 * @param in_len Length of input buffer in bytes.
 * @param cmd_id_out Filled with parsed command identifier.
 * @param payload On success, set to pointer into @p in for payload start.
 * @param payload_len Filled with payload length.
 * @return true on success, false if the buffer is malformed or too short.
 */
bool grlc_cmd_parse_request(const uint8_t *in, size_t in_len, uint16_t *cmd_id_out,
                            const uint8_t **payload, uint16_t *payload_len);
/**
 * @brief Pack a response message payload for transport.
 * @param cmd_id Command identifier (echoes the request ID).
 * @param status Status code to embed in the response.
 * @param payload Optional payload pointer (may be NULL if payload_len==0).
 * @param payload_len Payload length in bytes.
 * @param out_buf Output buffer to receive packed response.
 * @param out_cap Capacity of out_buf in bytes.
 * @param out_len Filled with number of bytes written to out_buf.
 * @return true on success, false on invalid arguments or insufficient capacity.
 */
bool grlc_cmd_pack_response(uint16_t cmd_id, uint16_t status, const uint8_t *payload,
                            uint16_t payload_len, uint8_t *out_buf, size_t out_cap,
                            size_t *out_len);
/**
 * @brief Parse a response message payload extracted from transport.
 * @param in Pointer to packed response bytes.
 * @param in_len Length of input buffer in bytes.
 * @param cmd_id_out Filled with parsed command identifier.
 * @param status_out Filled with parsed status code.
 * @param payload On success, set to pointer into @p in for payload start.
 * @param payload_len Filled with payload length.
 * @return true on success, false if the buffer is malformed or too short.
 */
bool grlc_cmd_parse_response(const uint8_t *in, size_t in_len, uint16_t *cmd_id_out,
                             uint16_t *status_out, const uint8_t **payload, uint16_t *payload_len);

#ifdef __cplusplus
}
#endif
