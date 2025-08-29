// Additional transport parser edge and stats tests

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "proto/inc/transport.h"
#include "proto/inc/crc32.h"
#include <vector>
#include <cstring>

namespace {

struct Cap {
    std::vector<uint8_t> msg;
    uint16_t session = 0;
    bool is_resp = false;
};

void on_msg(void *user, uint16_t session, const uint8_t *msg, size_t len, bool is_response)
{
    auto *c = reinterpret_cast<Cap*>(user);
    c->msg.assign(msg, msg + len);
    c->session = session;
    c->is_resp = is_response;
}

size_t devnull_write(const uint8_t *, size_t len) { return len; }

static std::vector<uint8_t> frame(uint8_t ver, uint8_t flags, uint16_t session,
                                  uint16_t idx, uint16_t cnt, const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> f;
    f.reserve(2 + 10 + payload.size() + 4);
    f.push_back(0xA5); f.push_back(0x5A);
    f.push_back(ver);
    f.push_back(flags);
    f.push_back((uint8_t)(session & 0xFF)); f.push_back((uint8_t)(session >> 8));
    f.push_back((uint8_t)(idx & 0xFF)); f.push_back((uint8_t)(idx >> 8));
    f.push_back((uint8_t)(cnt & 0xFF)); f.push_back((uint8_t)(cnt >> 8));
    f.push_back((uint8_t)(payload.size() & 0xFF)); f.push_back((uint8_t)(payload.size() >> 8));
    f.insert(f.end(), payload.begin(), payload.end());
    uint32_t crc = crc32_ieee(&f[2], 10 + payload.size());
    f.push_back((uint8_t)(crc & 0xFF));
    f.push_back((uint8_t)((crc >> 8) & 0xFF));
    f.push_back((uint8_t)((crc >> 16) & 0xFF));
    f.push_back((uint8_t)((crc >> 24) & 0xFF));
    return f;
}

} // namespace

TEST(TransportEdges, ThreeFragmentSequence)
{
    transport_ctx t{}; Cap cap{}; transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);
    const size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
    std::vector<uint8_t> payload(maxp + 50, 0x42);
    auto f0 = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_START, 0x1001, 0, 3,
                    std::vector<uint8_t>(payload.begin(), payload.begin()+maxp));
    auto f1 = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_MIDDLE, 0x1001, 1, 3,
                    std::vector<uint8_t>(payload.begin()+maxp, payload.begin()+maxp+25));
    auto f2 = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_END, 0x1001, 2, 3,
                    std::vector<uint8_t>(payload.begin()+maxp+25, payload.end()));
    transport_rx_bytes(&t, f0.data(), f0.size());
    EXPECT_TRUE(cap.msg.empty());
    transport_rx_bytes(&t, f1.data(), f1.size());
    EXPECT_TRUE(cap.msg.empty());
    transport_rx_bytes(&t, f2.data(), f2.size());
    ASSERT_EQ(cap.msg.size(), payload.size());
}

TEST(TransportEdges, FragIndexMismatchDrops)
{
    transport_ctx t{}; Cap cap{}; transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);
    auto f0 = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_START, 0x2002, 0, 2, {1,2});
    auto f_bad = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_END, 0x2002, 2, 2, {3,4});
    transport_rx_bytes(&t, f0.data(), f0.size());
    transport_rx_bytes(&t, f_bad.data(), f_bad.size());
    EXPECT_TRUE(cap.msg.empty());
    transport_stats s{}; transport_get_stats(&t, &s);
    EXPECT_EQ(s.messages_dropped, 1u);
}

TEST(TransportEdges, FragCountOverflowEndStillDelivers)
{
    transport_ctx t{}; Cap cap{}; transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);
    auto f0 = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_START, 0x3003, 0, 2, {1});
    auto f1 = frame(TRANSPORT_VERSION, 0, 0x3003, 1, 2, {2});
    auto f2 = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_END, 0x3003, 2, 2, {3});
    transport_rx_bytes(&t, f0.data(), f0.size());
    transport_rx_bytes(&t, f1.data(), f1.size());
    transport_rx_bytes(&t, f2.data(), f2.size());
    // Current implementation does not validate idx==cnt-1 on END; it delivers.
    ASSERT_EQ(cap.msg.size(), 3u);
}

TEST(TransportEdges, InvalidPayloadLenHeaderDrop)
{
    transport_ctx t{}; Cap cap{}; transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);
    // Feed sync + header with payload_len beyond max
    uint8_t hdr[2 + 10] = {0xA5, 0x5A,
        TRANSPORT_VERSION, 0,
        0x34, 0x12, 0, 0, 1, 0,
        (uint8_t)((TRANSPORT_FRAME_MAX_PAYLOAD + 1) & 0xFF), (uint8_t)(((TRANSPORT_FRAME_MAX_PAYLOAD + 1) >> 8) & 0xFF)};
    transport_rx_bytes(&t, hdr, sizeof(hdr));
    transport_stats s{}; transport_get_stats(&t, &s);
    EXPECT_EQ(s.frames_sync_drop, 1u);
}

TEST(TransportEdges, WrongVersionDroppedThenRecovers)
{
    transport_ctx t{}; Cap cap{}; transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);
    auto bad = frame(TRANSPORT_VERSION + 1, TRANSPORT_FLAG_START | TRANSPORT_FLAG_END, 0x4444, 0, 1, {9});
    transport_rx_bytes(&t, bad.data(), bad.size());
    EXPECT_TRUE(cap.msg.empty());
    transport_stats s{}; transport_get_stats(&t, &s);
    // CRC verified before version check, so frames_ok increments; but no message delivered
    EXPECT_EQ(s.frames_ok, 1u);
    EXPECT_EQ(s.frames_sync_drop, 1u);
    auto ok = frame(TRANSPORT_VERSION, TRANSPORT_FLAG_START | TRANSPORT_FLAG_END, 0x4444, 0, 1, {7});
    transport_rx_bytes(&t, ok.data(), ok.size());
    EXPECT_EQ(cap.session, 0x4444);
    ASSERT_EQ(cap.msg.size(), 1u);
    EXPECT_EQ(cap.msg[0], 7);
}

TEST(TransportEdges, ReassemblyMaxExceeded)
{
    transport_ctx t{}; Cap cap{}; transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);
    // Build fragments to exceed TRANSPORT_REASSEMBLY_MAX
    const size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
    const size_t too_big = TRANSPORT_REASSEMBLY_MAX + 10;
    std::vector<uint8_t> payload(too_big, 0x55);
    // Feed enough frames until drop occurs
    uint16_t idx = 0; uint16_t cnt = (uint16_t)((too_big + maxp - 1) / maxp);
    size_t off = 0;
    while (off < payload.size()) {
        size_t take = std::min(maxp, payload.size() - off);
        uint8_t flags = (idx == 0 ? TRANSPORT_FLAG_START : 0);
        if (off + take >= payload.size()) flags |= TRANSPORT_FLAG_END; else flags |= TRANSPORT_FLAG_MIDDLE;
        auto f = frame(TRANSPORT_VERSION, flags, 0xABCD, idx, cnt,
                       std::vector<uint8_t>(payload.begin()+off, payload.begin()+off+take));
        transport_rx_bytes(&t, f.data(), f.size());
        off += take; idx++;
    }
    EXPECT_TRUE(cap.msg.empty());
    transport_stats s{}; transport_get_stats(&t, &s);
    EXPECT_EQ(s.messages_dropped, 1u);
}
