#pragma once

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef TRANSPORT_VERSION
#define TRANSPORT_VERSION 1
#endif

#ifndef TRANSPORT_FRAME_MAX_PAYLOAD
#define TRANSPORT_FRAME_MAX_PAYLOAD 128u
#endif

#ifndef TRANSPORT_REASSEMBLY_MAX
#define TRANSPORT_REASSEMBLY_MAX 2048u
#endif

#ifndef TRANSPORT_MAX_FRAGMENTS
#define TRANSPORT_MAX_FRAGMENTS 64u
#endif

enum transport_flags {
    TRANSPORT_FLAG_START = 1u << 0,
    TRANSPORT_FLAG_MIDDLE = 1u << 1,
    TRANSPORT_FLAG_END = 1u << 2,
    TRANSPORT_FLAG_RESP = 1u << 4,
};

struct transport_stats {
    uint32_t frames_ok;
    uint32_t frames_crc_err;
    uint32_t frames_sync_drop;
    uint32_t messages_ok;
    uint32_t messages_dropped;
};

struct transport_lower_if {
    size_t (*write)(const uint8_t *data, size_t len);
};

typedef void (*transport_msg_cb)(void *user, uint16_t session, const uint8_t *msg, size_t len,
                                 bool is_response);

struct transport_ctx {
    const struct transport_lower_if *lower;
    transport_msg_cb on_msg;
    void *user;
    uint8_t rx_buf[TRANSPORT_FRAME_MAX_PAYLOAD + 16];
    uint16_t rx_need;
    uint16_t rx_have;
    enum {
        RX_SYNC0,
        RX_SYNC1,
        RX_HEADER,
        RX_PAYLOAD,
        RX_CRC
    } rx_state;
    uint8_t rx_hdr[12];
    uint8_t rx_payload[TRANSPORT_FRAME_MAX_PAYLOAD];
    uint8_t rx_crc[4];
    uint8_t re_buf[TRANSPORT_REASSEMBLY_MAX];
    uint16_t re_len;
    uint16_t re_session;
    uint16_t re_frag_index;
    uint16_t re_frag_count;
    bool re_in_progress;
    bool re_is_resp;
    struct transport_stats stats;
};

void transport_init(struct transport_ctx *t, const struct transport_lower_if *lower,
                    transport_msg_cb on_msg, void *user);
void transport_reset(struct transport_ctx *t);
void transport_rx_bytes(struct transport_ctx *t, const uint8_t *data, size_t len);
bool transport_send_message(struct transport_ctx *t, uint16_t session, const uint8_t *msg,
                            size_t len, bool is_response);
void transport_get_stats(const struct transport_ctx *t, struct transport_stats *out);

#ifdef __cplusplus
}
#endif
