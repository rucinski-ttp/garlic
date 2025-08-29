#include <gtest/gtest.h>
#include <cstdint>
#include <vector>

extern "C" {
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "commands/inc/register_all.h"
}

static std::vector<uint8_t> pack_req(uint16_t cmd, const std::vector<uint8_t> &payload)
{
    std::vector<uint8_t> b;
    b.reserve(4 + payload.size());
    b.push_back((uint8_t)(cmd & 0xFF));
    b.push_back((uint8_t)(cmd >> 8));
    b.push_back((uint8_t)(payload.size() & 0xFF));
    b.push_back((uint8_t)(payload.size() >> 8));
    b.insert(b.end(), payload.begin(), payload.end());
    return b;
}

TEST(TMP119Commands, ReadId)
{
    command_registry_init();
    command_register_builtin();
    // op=0x00 READ_ID, addr7=0x48
    auto req = pack_req(CMD_ID_TMP119, {0x00, 0x48});
    uint16_t cmd_id = 0;
    const uint8_t *payload = nullptr;
    uint16_t payload_len = 0;
    ASSERT_TRUE(command_parse_request(req.data(), req.size(), &cmd_id, &payload, &payload_len));
    ASSERT_EQ(cmd_id, CMD_ID_TMP119);
    uint8_t resp[64] = {0};
    size_t out_len = sizeof(resp);
    uint16_t status = 0xFFFF;
    ASSERT_TRUE(command_dispatch(cmd_id, payload, payload_len, resp, &out_len, &status));
    EXPECT_EQ(status, 0);
    ASSERT_EQ(out_len, 2u);
    // Little-endian 0x2117
    uint16_t got = (uint16_t)resp[0] | ((uint16_t)resp[1] << 8);
    EXPECT_EQ(got, 0x2117);
}

TEST(TMP119Commands, ReadTemp_mC)
{
    command_registry_init();
    command_register_builtin();
    // op=0x01 READ_TEMP_mC, addr7=0x48
    auto req = pack_req(CMD_ID_TMP119, {0x01, 0x48});
    uint16_t cmd_id = 0;
    const uint8_t *payload = nullptr;
    uint16_t payload_len = 0;
    ASSERT_TRUE(command_parse_request(req.data(), req.size(), &cmd_id, &payload, &payload_len));
    ASSERT_EQ(cmd_id, CMD_ID_TMP119);
    uint8_t resp[64] = {0};
    size_t out_len = sizeof(resp);
    uint16_t status = 0xFFFF;
    ASSERT_TRUE(command_dispatch(cmd_id, payload, payload_len, resp, &out_len, &status));
    EXPECT_EQ(status, 0);
    ASSERT_EQ(out_len, 4u);
    int32_t mc = (int32_t)resp[0] | ((int32_t)resp[1] << 8) | ((int32_t)resp[2] << 16) |
                 ((int32_t)resp[3] << 24);
    EXPECT_EQ(mc, 25000); // 25.000 C
}

