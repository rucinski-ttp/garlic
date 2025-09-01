#include <gtest/gtest.h>
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include <cstring>

extern "C" void grlc_cmd_registry_init(void);

extern "C" void grlc_cmd_register_builtin(void);

TEST(CommandHandlers, GitVersion)
{
    grlc_cmd_registry_init();
    grlc_cmd_register_builtin();
    // Registration happens via constructor attributes in handlers
    uint8_t out[64]; size_t out_len = sizeof(out);
    uint16_t st = 0;
    ASSERT_TRUE(grlc_cmd_dispatch(CMD_ID_GET_GIT_VERSION, nullptr, 0, out, &out_len, &st));
    EXPECT_EQ(st, CMD_STATUS_OK);
    ASSERT_GT(out_len, 0u);
}

TEST(CommandHandlers, Uptime)
{
    grlc_cmd_registry_init();
    grlc_cmd_register_builtin();
    uint8_t out[16]; size_t out_len = sizeof(out);
    uint16_t st = 0;
    ASSERT_TRUE(grlc_cmd_dispatch(CMD_ID_GET_UPTIME, nullptr, 0, out, &out_len, &st));
    EXPECT_EQ(st, CMD_STATUS_OK);
    ASSERT_EQ(out_len, 8u);
    uint64_t val = 0;
    for (int i = 7; i >= 0; --i) { val <<= 8; val |= out[i]; }
    EXPECT_EQ(val, 123456789ULL);
}

TEST(CommandHandlers, FlashRead)
{
    grlc_cmd_registry_init();
    grlc_cmd_register_builtin();
    uint8_t req[6];
    // addr=0x00000000, len=16
    req[0]=0; req[1]=0; req[2]=0; req[3]=0; req[4]=16; req[5]=0;
    uint8_t out[32]; size_t out_len = sizeof(out);
    uint16_t st = 0;
    ASSERT_TRUE(grlc_cmd_dispatch(CMD_ID_FLASH_READ, req, sizeof(req), out, &out_len, &st));
    EXPECT_EQ(st, CMD_STATUS_OK);
    ASSERT_EQ(out_len, 16u);
    for (size_t i=0;i<16;++i) EXPECT_EQ(out[i], (uint8_t)i);
}

TEST(CommandHandlers, Echo)
{
    grlc_cmd_registry_init();
    grlc_cmd_register_builtin();
    const uint8_t req[] = {0x10, 0x20, 0x30, 0x40, 0x55};
    uint8_t out[16]; size_t out_len = sizeof(out);
    uint16_t st = 0;
    ASSERT_TRUE(grlc_cmd_dispatch(CMD_ID_ECHO, req, sizeof(req), out, &out_len, &st));
    EXPECT_EQ(st, CMD_STATUS_OK);
    ASSERT_EQ(out_len, sizeof(req));
    for (size_t i = 0; i < sizeof(req); ++i) EXPECT_EQ(out[i], req[i]);
}
