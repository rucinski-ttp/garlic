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

/** @brief Running statistics from the transport layer. */
struct transport_stats {
    uint32_t frames_ok;        /**< Frames received with valid CRC */
    uint32_t frames_crc_err;   /**< Frames dropped due to CRC mismatch */
    uint32_t frames_sync_drop; /**< Parser resync events (bad header/len) */
    uint32_t messages_ok;      /**< Fully reassembled messages delivered */
    uint32_t messages_dropped; /**< Messages dropped due to protocol errors */
};

/** @brief Lower layer interface for frame writes. */
struct transport_lower_if {
    /**
     * @brief Write raw bytes to the underlying link.
     * @param data Pointer to bytes to write
     * @param len  Number of bytes
     * @return number of bytes consumed (<= len)
     */
    size_t (*write)(const uint8_t *data, size_t len);
};

/** @brief Callback invoked when a full message is reassembled. */
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

    /* Non-blocking TX state (one message in flight) */
    const uint8_t *tx_msg_ptr; /* pointer to whole message payload */
    size_t tx_msg_len;         /* remaining bytes of message payload */
    uint16_t tx_session;       /* session for the message */
    bool tx_is_resp;           /* response flag for frames */
    uint16_t tx_frag_index;    /* current fragment index */
    uint16_t tx_frag_count;    /* total fragments */
    uint8_t tx_frame_buf[2 + 10 + TRANSPORT_FRAME_MAX_PAYLOAD + 4];
    size_t tx_frame_len;           /* total length of current frame */
    size_t tx_frame_pos;           /* bytes already written for current frame */
    uint16_t tx_frame_payload_len; /* payload length of current frame */
    bool tx_in_progress;           /* sending in progress */

    /* Transport-owned copy of the message to send (prevents caller buffer reuse issues) */
    uint8_t tx_msg_copy[TRANSPORT_REASSEMBLY_MAX];
    size_t tx_msg_copy_len;
};

/**
 * @brief Initialize a transport context.
 * @param t      Transport context to initialize
 * @param lower  Lower-layer write interface
 * @param on_msg Callback invoked on complete messages
 * @param user   Opaque pointer passed to @p on_msg on delivery
 */
void grlc_transport_init(struct transport_ctx *t, const struct transport_lower_if *lower,
                         transport_msg_cb on_msg, void *user);

/** @brief Reset the transport parser and reassembly state. */
void grlc_transport_reset(struct transport_ctx *t);

/**
 * @brief Feed received bytes to the transport parser.
 * @param t    Transport context
 * @param data Pointer to bytes
 * @param len  Byte count
 */
void grlc_transport_rx_bytes(struct transport_ctx *t, const uint8_t *data, size_t len);

/**
 * @brief Send a complete message (fragmented into frames as needed).
 * @param t           Transport context
 * @param session     Session/correlation identifier
 * @param msg         Pointer to message payload
 * @param len         Payload length in bytes
 * @param is_response true if this is a response (sets RESP flag)
 * @return true on accepted for write, false on invalid args
 */
bool grlc_transport_send_message(struct transport_ctx *t, uint16_t session, const uint8_t *msg,
                                 size_t len, bool is_response);

/**
 * @brief Attempt to advance any in-progress TX frames (non-blocking).
 *
 * Writes as many bytes as the lower layer accepts without sleeping. Call
 * from the system tick/main loop to keep TX draining.
 */
void grlc_transport_tx_pump(struct transport_ctx *t);

/** @brief Copy current stats into @p out. */
void grlc_transport_get_stats(const struct transport_ctx *t, struct transport_stats *out);

#ifdef __cplusplus
}
#endif
