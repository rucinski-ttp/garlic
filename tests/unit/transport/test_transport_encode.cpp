// Transport encode tests: framing and fragmentation

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "proto/inc/transport.h"
#include "proto/inc/crc32.h"
#include <vector>
#include <cstring>

using ::testing::ElementsAreArray;

namespace {

struct LowerMock {
    std::vector<std::vector<uint8_t>> frames;
    static size_t write_thunk(const uint8_t *data, size_t len) {
        auto *self = reinterpret_cast<LowerMock*>(lower_cookie);
        self->frames.emplace_back(data, data + len);
        return len;
    }
    static void *lower_cookie;
};
void *LowerMock::lower_cookie = nullptr;

size_t lower_write_adapter(const uint8_t *data, size_t len) {
    auto *self = reinterpret_cast<LowerMock*>(LowerMock::lower_cookie);
    self->frames.emplace_back(data, data + len);
    return len;
}

} // namespace

TEST(TransportEncode, SingleFrame)
{
    LowerMock lm; LowerMock::lower_cookie = &lm;
    transport_msg_cb cb = nullptr;
    struct transport_lower_if lif{lower_write_adapter};
    transport_ctx t{};
    grlc_transport_init(&t, &lif, cb, nullptr);

    uint8_t payload[] = {0xDE, 0xAD, 0xBE, 0xEF};
    ASSERT_TRUE(grlc_transport_send_message(&t, 0x1234, payload, sizeof(payload), false));
    ASSERT_EQ(lm.frames.size(), 1u);

    auto &f = lm.frames[0];
    ASSERT_GE(f.size(), (size_t) (2 + 10 + 4));
    EXPECT_EQ(f[0], 0xA5);
    EXPECT_EQ(f[1], 0x5A);
    EXPECT_EQ(f[2], (uint8_t)TRANSPORT_VERSION);
    uint8_t flags = f[3];
    EXPECT_TRUE(flags & TRANSPORT_FLAG_START);
    EXPECT_TRUE(flags & TRANSPORT_FLAG_END);
    EXPECT_FALSE(flags & TRANSPORT_FLAG_RESP);
    // session
    EXPECT_EQ((uint16_t) (f[4] | (f[5] << 8)), 0x1234);
    // frag index/count
    EXPECT_EQ((uint16_t) (f[6] | (f[7] << 8)), 0);
    EXPECT_EQ((uint16_t) (f[8] | (f[9] << 8)), 1);
    // payload len
    EXPECT_EQ((uint16_t) (f[10] | (f[11] << 8)), sizeof(payload));
    // payload
    ASSERT_EQ(memcmp(&f[12], payload, sizeof(payload)), 0);
    // crc over [ver..payload]
    uint32_t calc = crc32_ieee(&f[2], (size_t)(10 + sizeof(payload)));
    uint32_t got = (uint32_t)f[12 + sizeof(payload)] | ((uint32_t)f[13 + sizeof(payload)] << 8) |
                   ((uint32_t)f[14 + sizeof(payload)] << 16) | ((uint32_t)f[15 + sizeof(payload)] << 24);
    EXPECT_EQ(calc, got);
}

TEST(TransportEncode, Fragmentation)
{
    LowerMock lm; LowerMock::lower_cookie = &lm;
    struct transport_lower_if lif{lower_write_adapter};
    transport_ctx t{};
    grlc_transport_init(&t, &lif, nullptr, nullptr);

    // Make payload larger than max payload per frame
    const size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
    std::vector<uint8_t> payload(maxp * 2 + 5);
    for (size_t i = 0; i < payload.size(); ++i) payload[i] = (uint8_t)i;
    ASSERT_TRUE(grlc_transport_send_message(&t, 0xBEEF, payload.data(), payload.size(), true));

    // Expect 3 frames
    ASSERT_EQ(lm.frames.size(), 3u);
    for (size_t i = 0; i < lm.frames.size(); ++i) {
        auto &f = lm.frames[i];
        uint8_t flags = f[3];
        if (i == 0) EXPECT_TRUE(flags & TRANSPORT_FLAG_START); else EXPECT_FALSE(flags & TRANSPORT_FLAG_START);
        if (i == lm.frames.size()-1) EXPECT_TRUE(flags & TRANSPORT_FLAG_END); else EXPECT_TRUE(flags & TRANSPORT_FLAG_MIDDLE);
        EXPECT_TRUE(flags & TRANSPORT_FLAG_RESP);
        uint16_t idx = (uint16_t) (f[6] | (f[7] << 8));
        uint16_t cnt = (uint16_t) (f[8] | (f[9] << 8));
        EXPECT_EQ(idx, i);
        EXPECT_EQ(cnt, 3);
    }
}
