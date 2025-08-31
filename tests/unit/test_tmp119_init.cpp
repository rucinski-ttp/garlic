#include <gtest/gtest.h>
#include <cstdint>
#include <csignal>

extern "C" {
#include "drivers/tmp119/inc/tmp119.h"
void i2c_mock_set_present_addr(uint8_t a);
void i2c_mock_set_dev_id(uint16_t id);
void i2c_mock_set_temp_raw(int16_t raw);
}

// Death tests require special settings on some platforms.

TEST(TMP119Init, BootInitFindsDevice)
{
    i2c_mock_set_present_addr(0x48);
    i2c_mock_set_dev_id(0x2117);
    int n = tmp119_boot_init();
    EXPECT_GE(n, 1);
    uint16_t id = 0;
    EXPECT_EQ(0, tmp119_read_device_id(0x48, &id));
    EXPECT_EQ(id, 0x2117);
}

TEST(TMP119Init, RequireInitializedAbortsOnMissing)
{
    // Simulate device at 0x48 only; 0x49 missing.
    i2c_mock_set_present_addr(0x48);
    ASSERT_GE(tmp119_boot_init(), 1);

    // Reading from an address that is not present should trigger fatal.
    EXPECT_DEATH({
        uint16_t id = 0;
        (void)tmp119_read_device_id(0x49, &id);
    }, "project_fatal");
}

