// Additional tests for command transport glue (unknown command, fragmented echo)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "proto/inc/transport.h"
#include "stack/cmd_transport/inc/cmd_transport.h"
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "proto/inc/crc32.h"
}

#include <vector>
#include <cstring>

namespace {
struct LowerCap {
    std::vector<std::vector<uint8_t>> frames;
};
static LowerCap *g_cap = nullptr;
size_t lc_write(const uint8_t *data, size_t len) { if (g_cap) g_cap->frames.emplace_back(data, data+len); return len; }

static std::vector<uint8_t> make_transport_frame(uint16_t session, uint16_t idx, uint16_t cnt, const std::vector<uint8_t>& payload, uint8_t flags)
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
    f.push_back((uint8_t)(crc & 0xFF)); f.push_back((uint8_t)((crc >> 8) & 0xFF));
    f.push_back((uint8_t)((crc >> 16) & 0xFF)); f.push_back((uint8_t)((crc >> 24) & 0xFF));
    return f;
}

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

static bool unpack_resp(const std::vector<uint8_t>& resp_bytes, uint16_t *cmd_id, uint16_t *status, std::vector<uint8_t> *payload)
{
    if (resp_bytes.size() < 6) return false;
    uint16_t cid = (uint16_t)resp_bytes[0] | ((uint16_t)resp_bytes[1] << 8);
    uint16_t st  = (uint16_t)resp_bytes[2] | ((uint16_t)resp_bytes[3] << 8);
    uint16_t len = (uint16_t)resp_bytes[4] | ((uint16_t)resp_bytes[5] << 8);
    if (resp_bytes.size() < (size_t)(6+len)) return false;
    if (cmd_id) *cmd_id = cid; if (status) *status = st; if (payload) payload->assign(resp_bytes.begin()+6, resp_bytes.begin()+6+len);
    return true;
}

static bool reassemble_and_parse(const LowerCap& lc, std::vector<uint8_t>& out_payload)
{
    // Reassemble one response message from captured frames
    uint16_t re_sess = 0; uint16_t re_idx = 0; uint16_t re_cnt = 0; std::vector<uint8_t> buf;
    bool in_prog = false;
    for (auto &f : lc.frames) {
        if (f.size() < 2+10+4) continue;
        uint8_t flags = f[3];
        uint16_t sess = (uint16_t)f[4] | ((uint16_t)f[5] << 8);
        uint16_t idx  = (uint16_t)f[6] | ((uint16_t)f[7] << 8);
        uint16_t cnt  = (uint16_t)f[8] | ((uint16_t)f[9] << 8);
        uint16_t len  = (uint16_t)f[10] | ((uint16_t)f[11] << 8);
        if (flags & TRANSPORT_FLAG_START) { buf.clear(); re_sess = sess; re_idx = 0; re_cnt = cnt; in_prog = true; }
        if (!in_prog || sess != re_sess || idx != re_idx) continue;
        buf.insert(buf.end(), f.begin()+12, f.begin()+12+len);
        re_idx++;
        if (flags & TRANSPORT_FLAG_END) {
            // buf now contains response message
            out_payload = buf; return true;
        }
    }
    return false;
}

} // namespace

TEST(CommandGlueMore, UnknownCommandRespondsWithError)
{
    LowerCap lc; g_cap = &lc; transport_lower_if lif{lc_write}; transport_ctx t{};
    cmd_transport_binding b{};
    grlc_transport_init(&t, &lif, grlc_cmd_get_transport_cb(), &b);
    grlc_cmd_transport_bind(&b, &t);
    grlc_cmd_transport_init();

    auto req = pack_req(0x7777, {});
    auto f = make_transport_frame(0x0BAD, 0, 1, req, TRANSPORT_FLAG_START | TRANSPORT_FLAG_END);
    grlc_transport_rx_bytes(&t, f.data(), f.size());

    ASSERT_FALSE(lc.frames.empty());
    std::vector<uint8_t> resp_msg;
    ASSERT_TRUE(reassemble_and_parse(lc, resp_msg));
    uint16_t cid=0, st=0; std::vector<uint8_t> pl;
    ASSERT_TRUE(unpack_resp(resp_msg, &cid, &st, &pl));
    EXPECT_EQ(cid, 0x7777);
    EXPECT_NE(st, (uint16_t)CMD_STATUS_OK);
}

TEST(CommandGlueMore, FragmentedEchoRoundTrip)
{
    LowerCap lc; g_cap = &lc; transport_lower_if lif{lc_write}; transport_ctx t{};
    cmd_transport_binding b{};
    grlc_transport_init(&t, &lif, grlc_cmd_get_transport_cb(), &b);
    grlc_cmd_transport_bind(&b, &t);
    grlc_cmd_transport_init();

    // Build large echo payload to force fragmentation in both request and response
    const size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
    std::vector<uint8_t> pl(maxp + 17);
    for (size_t i=0;i<pl.size();++i) pl[i] = (uint8_t)(i & 0xFF);
    auto req = pack_req(CMD_ID_ECHO, pl);
    auto f0 = make_transport_frame(0xCAFE, 0, 2, std::vector<uint8_t>(req.begin(), req.begin()+maxp), TRANSPORT_FLAG_START);
    auto f1 = make_transport_frame(0xCAFE, 1, 2, std::vector<uint8_t>(req.begin()+maxp, req.end()), TRANSPORT_FLAG_END);

    grlc_transport_rx_bytes(&t, f0.data(), f0.size());
    grlc_transport_rx_bytes(&t, f1.data(), f1.size());

    ASSERT_GE(lc.frames.size(), 2u); // response will be fragmented too
    std::vector<uint8_t> resp_msg;
    ASSERT_TRUE(reassemble_and_parse(lc, resp_msg));

    // Parse command response and verify payload matches request payload
    uint16_t cid=0, st=0; std::vector<uint8_t> resp_pl;
    ASSERT_TRUE(unpack_resp(resp_msg, &cid, &st, &resp_pl));
    EXPECT_EQ(cid, (uint16_t)CMD_ID_ECHO);
    EXPECT_EQ(st, (uint16_t)CMD_STATUS_OK);
    ASSERT_EQ(resp_pl.size(), pl.size());
    EXPECT_EQ(0, memcmp(resp_pl.data(), pl.data(), pl.size()));
}
