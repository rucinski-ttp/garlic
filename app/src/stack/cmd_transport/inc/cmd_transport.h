#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct transport_ctx;
typedef void (*transport_msg_cb)(void *user, uint16_t session, const uint8_t *msg, size_t len,
                                 bool is_response);

void cmd_transport_init(struct transport_ctx *t);
transport_msg_cb cmd_get_transport_cb(void);

#ifdef __cplusplus
}
#endif
