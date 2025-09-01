// Unit tests for command registry, pack/unpack, and dispatch

#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include "commands/inc/command.h"
#include <cstdint>
#include <vector>
#include <cstring>

static command_status_t echo_handler(const uint8_t *in, size_t in_len, uint8_t *out, size_t *out_len)
{
    if (!out || !out_len) return CMD_STATUS_ERR_INVALID;
    if (*out_len < in_len) return CMD_STATUS_ERR_BOUNDS;
    if (in_len && in) memcpy(out, in, in_len);
    *out_len = in_len;
    return CMD_STATUS_OK;
}

TEST(CommandCore, PackParseRequest)
{
    uint8_t out[64]; size_t out_len = 0;
    uint8_t payload[] = {1,2,3};
    ASSERT_TRUE(grlc_cmd_pack_request(0x1001, payload, sizeof(payload), out, sizeof(out), &out_len));

    uint16_t cmd = 0; const uint8_t *pl = nullptr; uint16_t pl_len = 0;
    ASSERT_TRUE(grlc_cmd_parse_request(out, out_len, &cmd, &pl, &pl_len));
    EXPECT_EQ(cmd, 0x1001);
    ASSERT_EQ(pl_len, sizeof(payload));
    EXPECT_EQ(0, memcmp(pl, payload, sizeof(payload)));
}

TEST(CommandCore, PackParseResponse)
{
    uint8_t out[64]; size_t out_len = 0;
    uint8_t payload[] = {4,5};
    ASSERT_TRUE(grlc_cmd_pack_response(0x2002, CMD_STATUS_OK, payload, sizeof(payload), out, sizeof(out), &out_len));

    uint16_t cmd = 0, st = 0; const uint8_t *pl = nullptr; uint16_t pl_len = 0;
    ASSERT_TRUE(grlc_cmd_parse_response(out, out_len, &cmd, &st, &pl, &pl_len));
    EXPECT_EQ(cmd, 0x2002);
    EXPECT_EQ(st, CMD_STATUS_OK);
    ASSERT_EQ(pl_len, sizeof(payload));
    EXPECT_EQ(0, memcmp(pl, payload, sizeof(payload)));
}

TEST(CommandCore, RegistryAndDispatch)
{
    grlc_cmd_registry_init();
    ASSERT_TRUE(grlc_cmd_register(0x3003, echo_handler));
    // duplicate should fail
    ASSERT_FALSE(grlc_cmd_register(0x3003, echo_handler));

    uint8_t in[] = {9,9,9};
    uint8_t out[8]; size_t cap = sizeof(out);
    uint16_t st = 0;
    ASSERT_TRUE(grlc_cmd_dispatch(0x3003, in, sizeof(in), out, &cap, &st));
    EXPECT_EQ(st, CMD_STATUS_OK);
    ASSERT_EQ(cap, sizeof(in));
    EXPECT_EQ(0, memcmp(out, in, sizeof(in)));

    // unknown command
    cap = sizeof(out);
    ASSERT_FALSE(grlc_cmd_dispatch(0x3004, in, sizeof(in), out, &cap, &st));
    EXPECT_EQ(st, CMD_STATUS_ERR_UNSUPPORTED);
}

TEST(CommandCore, PackRequestTooSmall)
{
    uint8_t out[3]; size_t out_len = 0;
    uint8_t payload[] = {1,2};
    EXPECT_FALSE(grlc_cmd_pack_request(0x1234, payload, sizeof(payload), out, sizeof(out), &out_len));
}

TEST(CommandCore, ParseShortBuffers)
{
    uint8_t short_req[] = {0,0,1};
    EXPECT_FALSE(grlc_cmd_parse_request(short_req, sizeof(short_req), nullptr, nullptr, nullptr));
    uint8_t short_resp[] = {0,0,0,0,1};
    EXPECT_FALSE(grlc_cmd_parse_response(short_resp, sizeof(short_resp), nullptr, nullptr, nullptr, nullptr));
}

TEST(CommandCore, DispatchInvokesMockHandler)
{
    using ::testing::_; using ::testing::Return;
    // gmock over a C-style function pointer via bridge
    static ::testing::MockFunction<command_status_t(const uint8_t*, size_t, uint8_t*, size_t*)> mock;
    auto bridge = [](const uint8_t* in, size_t in_len, uint8_t* out, size_t* out_len) -> command_status_t {
        return mock.Call(in, in_len, out, out_len);
    };
    grlc_cmd_registry_init();
    ASSERT_TRUE(grlc_cmd_register(0x4242, +bridge));

    uint8_t in[] = {1,2,3}; uint8_t out[8]; size_t out_len = sizeof(out); uint16_t st = 0;
    EXPECT_CALL(mock, Call(_, sizeof(in), _, _)).WillOnce(Return(CMD_STATUS_OK));
    EXPECT_TRUE(grlc_cmd_dispatch(0x4242, in, sizeof(in), out, &out_len, &st));
    EXPECT_EQ(st, (uint16_t)CMD_STATUS_OK);
}
