#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "proto/inc/transport.h"
#include "stack/cmd_transport/inc/cmd_transport.h"
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include <vector>
#include <cstring>
#include "proto/inc/crc32.h"

using ::testing::AnyOf;

namespace {

struct LowerCapture {
    std::vector<std::vector<uint8_t>> frames;
};

static LowerCapture *g_cap = nullptr;

size_t lc_write(const uint8_t *data, size_t len) {
    if (g_cap) g_cap->frames.emplace_back(data, data + len);
    return len;
}

} // namespace

static std::vector<uint8_t> pack_req(uint16_t cmd_id, const std::vector<uint8_t>& pl)
{
    std::vector<uint8_t> out(4 + pl.size());
    out[0] = (uint8_t)(cmd_id & 0xFF);
    out[1] = (uint8_t)(cmd_id >> 8);
    out[2] = (uint8_t)(pl.size() & 0xFF);
    out[3] = (uint8_t)(pl.size() >> 8);
    if (!pl.empty()) memcpy(&out[4], pl.data(), pl.size());
    return out;
}

TEST(CommandGlue, RequestProducesResponse)
{
    LowerCapture lc; g_cap = &lc;
    transport_lower_if lif{lc_write};
    transport_ctx t{};
    transport_init(&t, &lif, cmd_get_transport_cb(), &t);
    cmd_transport_init(&t);

    // Build one payload: GET_GIT_VERSION with no payload
    auto req = pack_req(CMD_ID_GET_GIT_VERSION, {});
    // send one transport message to RX
    // Use transport_send_message to build framing, then loop back into rx for minimal harness
    // (In real app, RX would be via UART) Here we directly feed a synthetically encoded frame
    // Construct one frame manually for test
    uint16_t session = 0x55AA;
    std::vector<uint8_t> frame;
    frame.reserve(2+10+req.size()+4);
    frame.push_back(0xA5); frame.push_back(0x5A);
    frame.push_back((uint8_t)TRANSPORT_VERSION);
    frame.push_back(TRANSPORT_FLAG_START | TRANSPORT_FLAG_END);
    frame.push_back((uint8_t)(session & 0xFF)); frame.push_back((uint8_t)(session >> 8));
    frame.push_back(0); frame.push_back(0);
    frame.push_back(1); frame.push_back(0);
    frame.push_back((uint8_t)(req.size() & 0xFF)); frame.push_back((uint8_t)(req.size() >> 8));
    frame.insert(frame.end(), req.begin(), req.end());
    uint32_t crc = crc32_ieee(&frame[2], 10 + req.size());
    frame.push_back((uint8_t)(crc & 0xFF));
    frame.push_back((uint8_t)((crc >> 8) & 0xFF));
    frame.push_back((uint8_t)((crc >> 16) & 0xFF));
    frame.push_back((uint8_t)((crc >> 24) & 0xFF));

    transport_rx_bytes(&t, frame.data(), frame.size());

    // Expect at least one outbound response frame written
    ASSERT_FALSE(lc.frames.empty());
    auto &f = lc.frames[0];
    ASSERT_GE(f.size(), (size_t)(2+10+6+4));
    // Flags include RESP
    EXPECT_TRUE((f[3] & TRANSPORT_FLAG_RESP) != 0);
    // Payload contains cmd_id + status + payload_len
    uint16_t p_len = (uint16_t)f[10] | ((uint16_t)f[11] << 8);
    ASSERT_GE(p_len, 6u);
}
