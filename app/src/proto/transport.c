#include "proto/inc/transport.h"
#ifdef __ZEPHYR__
#include <zephyr/kernel.h>
#include <zephyr/logging/log.h>
LOG_MODULE_REGISTER(transport_tx, LOG_LEVEL_INF);
#endif
#include <string.h>
#include "proto/inc/crc32.h"

#define SYNC0 0xA5
#define SYNC1 0x5A

// Header (excluding sync): ver(1) flags(1) session(2) frag_index(2) frag_count(2) payload_len(2)
#define HDR_LEN 10

static uint16_t rd16(const uint8_t *p)
{
    return (uint16_t)p[0] | ((uint16_t)p[1] << 8);
}
static void wr16(uint8_t *p, uint16_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
}
static void wr32(uint8_t *p, uint32_t v)
{
    p[0] = (uint8_t)(v & 0xFF);
    p[1] = (uint8_t)((v >> 8) & 0xFF);
    p[2] = (uint8_t)((v >> 16) & 0xFF);
    p[3] = (uint8_t)((v >> 24) & 0xFF);
}

static void reset_parse(struct transport_ctx *t)
{
    t->rx_state = RX_SYNC0;
    t->rx_have = 0;
    t->rx_need = 0;
}

void grlc_transport_init(struct transport_ctx *t, const struct transport_lower_if *lower,
                         transport_msg_cb on_msg, void *user)
{
    memset(t, 0, sizeof(*t));
    t->lower = lower;
    t->on_msg = on_msg;
    t->user = user;
    reset_parse(t);
}

void grlc_transport_reset(struct transport_ctx *t)
{
    reset_parse(t);
    t->re_in_progress = false;
    t->re_len = 0;
    t->re_frag_index = 0;
    t->re_frag_count = 0;
    /* reset TX */
    t->tx_in_progress = false;
    t->tx_msg_ptr = NULL;
    t->tx_msg_len = 0;
    t->tx_frame_len = 0;
    t->tx_frame_pos = 0;
}

void grlc_transport_get_stats(const struct transport_ctx *t, struct transport_stats *out)
{
    if (out) {
        *out = t->stats;
    }
}

static void reassembly_reset(struct transport_ctx *t)
{
    t->re_in_progress = false;
    t->re_len = 0;
    t->re_frag_index = 0;
    t->re_frag_count = 0;
}

static void deliver_if_complete(struct transport_ctx *t, uint16_t session, uint8_t flags)
{
    if (!(flags & TRANSPORT_FLAG_END)) {
        return;
    }
    // complete
    t->stats.messages_ok++;
    if (t->on_msg) {
        t->on_msg(t->user, session, t->re_buf, t->re_len, t->re_is_resp);
    }
    reassembly_reset(t);
}

static void handle_frame(struct transport_ctx *t, const uint8_t *hdr, const uint8_t *payload)
{
    uint8_t ver = hdr[0];
    uint8_t flags = hdr[1];
    uint16_t session = rd16(&hdr[2]);
    uint16_t frag_index = rd16(&hdr[4]);
    uint16_t frag_count = rd16(&hdr[6]);
    uint16_t payload_len = rd16(&hdr[8]);

    if (ver != TRANSPORT_VERSION) {
        t->stats.frames_sync_drop++;
        reassembly_reset(t);
        return;
    }
    if (payload_len > TRANSPORT_FRAME_MAX_PAYLOAD) {
        t->stats.frames_sync_drop++;
        reassembly_reset(t);
        return;
    }
    if (frag_count == 0 || frag_count > TRANSPORT_MAX_FRAGMENTS) {
        t->stats.frames_sync_drop++;
        reassembly_reset(t);
        return;
    }

    bool is_resp = (flags & TRANSPORT_FLAG_RESP) != 0;

    if (flags & TRANSPORT_FLAG_START) {
        // start new message
        t->re_in_progress = true;
        t->re_len = 0;
        t->re_session = session;
        t->re_frag_index = 0;
        t->re_frag_count = frag_count;
        t->re_is_resp = is_resp;
    } else {
        // must match in-progress session and ordering
        if (!t->re_in_progress || session != t->re_session || frag_index != t->re_frag_index) {
            t->stats.messages_dropped++;
            reassembly_reset(t);
            return;
        }
    }

    // Append payload
    if ((uint32_t)t->re_len + payload_len > TRANSPORT_REASSEMBLY_MAX) {
        t->stats.messages_dropped++;
        reassembly_reset(t);
        return;
    }
    memcpy(&t->re_buf[t->re_len], payload, payload_len);
    t->re_len += payload_len;
    t->re_frag_index++;

    // If middle, ensure flags consistent
    if (!(flags & TRANSPORT_FLAG_END)) {
        // Continue only if not exceeding declared frag_count
        if (t->re_frag_index > t->re_frag_count) {
            t->stats.messages_dropped++;
            reassembly_reset(t);
            return;
        }
    }

    deliver_if_complete(t, session, flags);
}

void grlc_transport_rx_bytes(struct transport_ctx *t, const uint8_t *data, size_t len)
{
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = data[i];
        switch (t->rx_state) {
            case RX_SYNC0:
                if (b == SYNC0) {
                    t->rx_state = RX_SYNC1;
                }
                break;
            case RX_SYNC1:
                if (b == SYNC1) {
                    t->rx_state = RX_HEADER;
                    t->rx_have = 0;
                    t->rx_need = HDR_LEN;
                } else {
                    t->rx_state = RX_SYNC0;
                }
                break;
            case RX_HEADER:
                t->rx_hdr[t->rx_have++] = b;
                if (t->rx_have == t->rx_need) {
                    uint16_t payload_len = rd16(&t->rx_hdr[8]);
                    if (payload_len > TRANSPORT_FRAME_MAX_PAYLOAD) {
                        // invalid, drop and resync
                        t->stats.frames_sync_drop++;
                        reset_parse(t);
                        break;
                    }
                    t->rx_state = RX_PAYLOAD;
                    t->rx_have = 0;
                    t->rx_need = payload_len;
                }
                break;
            case RX_PAYLOAD:
                if (t->rx_need) {
                    t->rx_payload[t->rx_have++] = b;
                    if (t->rx_have == t->rx_need) {
                        t->rx_state = RX_CRC;
                        t->rx_have = 0;
                        t->rx_need = 4;
                    }
                }
                break;
            case RX_CRC:
                t->rx_crc[t->rx_have++] = b;
                if (t->rx_have == t->rx_need) {
                    // verify CRC over [ver..payload]
                    // Compute in one shot using a temporary buffer
                    uint8_t tmp[HDR_LEN + TRANSPORT_FRAME_MAX_PAYLOAD];
                    memcpy(tmp, t->rx_hdr, HDR_LEN);
                    memcpy(tmp + HDR_LEN, t->rx_payload, rd16(&t->rx_hdr[8]));
                    uint32_t calc = crc32_ieee(tmp, HDR_LEN + rd16(&t->rx_hdr[8]));
                    uint32_t got = (uint32_t)t->rx_crc[0] | ((uint32_t)t->rx_crc[1] << 8) |
                                   ((uint32_t)t->rx_crc[2] << 16) | ((uint32_t)t->rx_crc[3] << 24);
                    if (calc == got) {
                        t->stats.frames_ok++;
                        handle_frame(t, t->rx_hdr, t->rx_payload);
                    } else {
                        t->stats.frames_crc_err++;
                        reassembly_reset(t);
                    }
                    reset_parse(t);
                }
                break;
        }
    }
}

bool grlc_transport_send_message(struct transport_ctx *t, uint16_t session, const uint8_t *msg,
                                 size_t len, bool is_response)
{
    bool accepted = false;
    if (t && t->lower && t->lower->write && (msg || len == 0) && !t->tx_in_progress &&
        len <= TRANSPORT_REASSEMBLY_MAX) {
        size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
        uint16_t frag_count = (uint16_t)((len + maxp - 1) / maxp);
        if (frag_count == 0) {
            frag_count = 1;
        }
        if (len > 0) {
            memcpy(t->tx_msg_copy, msg, len);
        }
        t->tx_msg_copy_len = len;
        /* Initialize TX state */
        t->tx_msg_ptr = (len > 0) ? t->tx_msg_copy : NULL;
        t->tx_msg_len = t->tx_msg_copy_len;
        t->tx_session = session;
        t->tx_is_resp = is_response;
        t->tx_frag_index = 0;
        t->tx_frag_count = frag_count;
        t->tx_in_progress = true;
        t->tx_frame_pos = 0;
        t->tx_frame_len = 0;
        /* Try to pump immediately (non-blocking). */
        grlc_transport_tx_pump(t);
        accepted = true;
    }
    return accepted;
}

static void assemble_next_frame(struct transport_ctx *t)
{
    size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
    size_t remaining = t->tx_msg_len;
    size_t take = remaining < maxp ? remaining : maxp;
    uint8_t flags = 0;
    if (t->tx_frag_index == 0) {
        flags |= TRANSPORT_FLAG_START;
    }
    if (t->tx_frag_index == (uint16_t)(t->tx_frag_count - 1)) {
        flags |= TRANSPORT_FLAG_END;
    } else {
        flags |= TRANSPORT_FLAG_MIDDLE;
    }
    if (t->tx_is_resp) {
        flags |= TRANSPORT_FLAG_RESP;
    }

    size_t pos = 0;
    t->tx_frame_buf[pos++] = SYNC0;
    t->tx_frame_buf[pos++] = SYNC1;
    t->tx_frame_buf[pos++] = (uint8_t)TRANSPORT_VERSION;
    t->tx_frame_buf[pos++] = flags;
    wr16(&t->tx_frame_buf[pos], t->tx_session);
    pos += 2;
    wr16(&t->tx_frame_buf[pos], t->tx_frag_index);
    pos += 2;
    wr16(&t->tx_frame_buf[pos], t->tx_frag_count);
    pos += 2;
    wr16(&t->tx_frame_buf[pos], (uint16_t)take);
    pos += 2;
    if (take > 0 && t->tx_msg_ptr) {
        memcpy(&t->tx_frame_buf[pos], t->tx_msg_ptr, take);
    }
    pos += take;
    uint32_t crc = crc32_ieee(&t->tx_frame_buf[2], HDR_LEN + take);
    wr32(&t->tx_frame_buf[pos], crc);
    pos += 4;
    t->tx_frame_len = pos;
    t->tx_frame_pos = 0;
    t->tx_frame_payload_len = (uint16_t)take;
#ifdef __ZEPHYR__
    LOG_INF("tx asm: sess=%u idx=%u/%u pay=%u bytes", (unsigned)t->tx_session,
            (unsigned)t->tx_frag_index, (unsigned)t->tx_frag_count,
            (unsigned)t->tx_frame_payload_len);
#endif
}

void grlc_transport_tx_pump(struct transport_ctx *t)
{
    if (!t || !t->tx_in_progress || !t->lower || !t->lower->write) {
        return;
    }
    /* Keep assembling and writing frames until lower layer stalls or message completes. */
    while (t->tx_in_progress) {
        /* If no current frame assembled, assemble it */
        if (t->tx_frame_pos == 0 && t->tx_frame_len == 0) {
            assemble_next_frame(t);
        }

        /* Write as much as the lower layer accepts, non-blocking */
        while (t->tx_frame_pos < t->tx_frame_len) {
            size_t w = t->lower->write(&t->tx_frame_buf[t->tx_frame_pos],
                                       t->tx_frame_len - t->tx_frame_pos);
            if (w == 0) {
                /* No space right now; try again on next tick */
#ifdef __ZEPHYR__
                LOG_INF("tx stall: pos=%u len=%u", (unsigned)t->tx_frame_pos,
                        (unsigned)t->tx_frame_len);
#endif
                /* Stall: exit pump; caller will invoke again later. */
                return;
            }
            t->tx_frame_pos += w;
        }

        /* Frame complete */
        t->tx_frag_index++;
        if (t->tx_frame_payload_len > 0) {
            if (t->tx_msg_len >= t->tx_frame_payload_len) {
                t->tx_msg_len -= t->tx_frame_payload_len;
                if (t->tx_msg_ptr) {
                    t->tx_msg_ptr += t->tx_frame_payload_len;
                }
            } else {
                t->tx_msg_len = 0; /* safety */
            }
        }

        if (t->tx_frag_index >= t->tx_frag_count) {
            /* Message complete */
            t->tx_in_progress = false;
            t->tx_frame_len = 0;
            t->tx_frame_pos = 0;
            break;
        }

        /* Prepare to assemble next frame and loop */
        t->tx_frame_len = 0;
        t->tx_frame_pos = 0;
    }
}
