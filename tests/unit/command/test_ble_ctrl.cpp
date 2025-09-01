#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "commands/inc/command.h"
#include "commands/inc/ids.h"
#include "commands/inc/register_all.h"
}

using ::testing::ElementsAre;

TEST(BLECtrl, StatusAndSetAdv)
{
    command_registry_init();
    command_register_builtin();

    uint8_t req[8];
    uint8_t out[16];
    size_t out_len = sizeof(out);
    uint16_t status = 0xFFFF;

    // Initially false/false
    req[0] = 0x00;
    ASSERT_TRUE(command_dispatch(CMD_ID_BLE_CTRL, req, 1, out, &out_len, &status));
    EXPECT_EQ(status, (uint16_t)CMD_STATUS_OK);
    ASSERT_EQ(out_len, 2u);
    EXPECT_THAT(std::vector<uint8_t>(out, out + 2), ElementsAre(0, 0));

    // Enable advertising
    out_len = sizeof(out);
    req[0] = 0x01; // SET_ADV
    req[1] = 0x01; // enable
    ASSERT_TRUE(command_dispatch(CMD_ID_BLE_CTRL, req, 2, out, &out_len, &status));
    EXPECT_EQ(status, (uint16_t)CMD_STATUS_OK);
    EXPECT_EQ(out_len, 0u);

    // Status should reflect advertising on
    out_len = sizeof(out);
    req[0] = 0x00;
    ASSERT_TRUE(command_dispatch(CMD_ID_BLE_CTRL, req, 1, out, &out_len, &status));
    EXPECT_EQ(status, (uint16_t)CMD_STATUS_OK);
    ASSERT_EQ(out_len, 2u);
    EXPECT_THAT(std::vector<uint8_t>(out, out + 2), ElementsAre(1, 0));
}

