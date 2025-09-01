#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

struct transport_ctx;
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#endif
typedef void (*transport_msg_cb)(void *user,
                                 uint16_t session,
                                 const uint8_t *msg,
                                 size_t len,
                                 bool is_response);

/**
 * @brief Per-transport binding for command transport.
 *
 * Associates a transport with a dedicated response buffer and (under Zephyr)
 * a mutex to serialize response construction and sending. Prevents races when
 * multiple links (UART, BLE) are active.
 */
struct cmd_transport_binding {
    struct transport_ctx *t; /**< Destination transport to send responses on */
    uint8_t
        resp_buf[2048]; /**< Response buffer (>= TRANSPORT_REASSEMBLY_MAX) */
#ifdef __ZEPHYR__
    struct k_mutex lock; /**< Serialize response build/send per binding */
#endif
    /* Pending response if transport TX is busy (keeps its own copy) */
    bool pending;
    uint16_t pending_session;
    size_t pending_len;
    uint8_t pending_buf[2048];
};

/** @brief Initialize command registry and built-in handlers (idempotent). */
void grlc_cmd_transport_init(void);

/**
 * @brief Bind a transport to the command callback.
 * @param b Binding object to initialize (user-owned storage)
 * @param t Transport to use for responses
 */
void grlc_cmd_transport_bind(struct cmd_transport_binding *b,
                             struct transport_ctx *t);

/** @brief Get the transport message callback for commands. */
transport_msg_cb grlc_cmd_get_transport_cb(void);

/**
 * @brief Advance any pending response if transport was busy earlier.
 * @param b Binding to service.
 */
void grlc_cmd_transport_tick(struct cmd_transport_binding *b);

#ifdef __cplusplus
}
#endif
