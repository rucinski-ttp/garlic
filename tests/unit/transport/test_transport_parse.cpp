// Transport parse tests: end-to-end framing, resync, CRC errors

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "proto/inc/transport.h"
#include "proto/inc/crc32.h"
#include <vector>
#include <cstring>

namespace {

struct Capture {
    std::vector<uint8_t> msg;
    uint16_t session = 0;
    bool is_resp = false;
};

void on_msg(void *user, uint16_t session, const uint8_t *msg, size_t len, bool is_response) {
    auto *cap = reinterpret_cast<Capture*>(user);
    cap->msg.assign(msg, msg + len);
    cap->session = session;
    cap->is_resp = is_response;
}

size_t devnull_write(const uint8_t *, size_t len) { return len; }

} // namespace

static std::vector<uint8_t> make_frame(uint16_t session, uint16_t idx, uint16_t cnt, const std::vector<uint8_t>& payload, uint8_t flags)
{
    std::vector<uint8_t> f;
    f.reserve(2 + 10 + payload.size() + 4);
    f.push_back(0xA5); f.push_back(0x5A);
    f.push_back((uint8_t)TRANSPORT_VERSION);
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

TEST(TransportParse, SingleFrameHappyPath)
{
    transport_ctx t{}; Capture cap{};
    transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);

    std::vector<uint8_t> payload = {1,2,3,4,5};
    auto f = make_frame(0x1111, 0, 1, payload, TRANSPORT_FLAG_START | TRANSPORT_FLAG_END);

    // Feed in arbitrary chunks
    transport_rx_bytes(&t, f.data(), 3);
    transport_rx_bytes(&t, f.data()+3, 7);
    transport_rx_bytes(&t, f.data()+10, f.size()-10);

    EXPECT_EQ(cap.session, 0x1111);
    ASSERT_EQ(cap.msg.size(), payload.size());
    EXPECT_EQ(0, memcmp(cap.msg.data(), payload.data(), payload.size()));
}

TEST(TransportParse, ResyncWithNoiseAndCRCError)
{
    transport_ctx t{}; Capture cap{};
    transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);

    std::vector<uint8_t> payload = {9,8,7};
    auto f = make_frame(0x2222, 0, 1, payload, TRANSPORT_FLAG_START | TRANSPORT_FLAG_END | TRANSPORT_FLAG_RESP);

    std::vector<uint8_t> noise = {0x00, 0xFF, 0xAA};
    transport_rx_bytes(&t, noise.data(), noise.size());

    // Corrupt one byte in CRC
    auto bad = f;
    bad[bad.size()-1] ^= 0x1;
    transport_rx_bytes(&t, bad.data(), bad.size()); // should be dropped
    EXPECT_TRUE(cap.msg.empty());

    // Now feed good frame; parser should resync and deliver
    transport_rx_bytes(&t, f.data(), f.size());
    EXPECT_EQ(cap.session, 0x2222);
    ASSERT_EQ(cap.msg.size(), payload.size());
    EXPECT_TRUE(cap.is_resp);
}

TEST(TransportParse, MultiFragmentReassembly)
{
    transport_ctx t{}; Capture cap{};
    transport_lower_if lif{devnull_write};
    transport_init(&t, &lif, on_msg, &cap);

    const size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
    std::vector<uint8_t> payload(maxp + 7, 0xAB);
    auto f0 = make_frame(0x3333, 0, 2, std::vector<uint8_t>(payload.begin(), payload.begin()+maxp), TRANSPORT_FLAG_START);
    auto f1 = make_frame(0x3333, 1, 2, std::vector<uint8_t>(payload.begin()+maxp, payload.end()), TRANSPORT_FLAG_END);

    transport_rx_bytes(&t, f0.data(), f0.size());
    EXPECT_TRUE(cap.msg.empty());
    transport_rx_bytes(&t, f1.data(), f1.size());
    ASSERT_EQ(cap.msg.size(), payload.size());
}
