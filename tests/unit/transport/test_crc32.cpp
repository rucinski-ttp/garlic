// Unit tests for CRC32 implementation

#include <gtest/gtest.h>
#include "proto/inc/crc32.h"
#include <cstdint>
#include <vector>

TEST(CRC32, KnownVectors)
{
    // Empty string
    const uint8_t empty[] = {};
    EXPECT_EQ(crc32_ieee(empty, 0), 0x00000000u);

    // "123456789" => 0xCBF43926
    const uint8_t s[] = {'1','2','3','4','5','6','7','8','9'};
    EXPECT_EQ(crc32_ieee(s, sizeof(s)), 0xCBF43926u);
}

TEST(CRC32, IncrementalEquivalent)
{
    std::vector<uint8_t> data(1024);
    for (size_t i = 0; i < data.size(); ++i) data[i] = static_cast<uint8_t>(i & 0xFF);
    uint32_t one = crc32_ieee(data.data(), data.size());

    // compute as two blocks concatenated with temp
    const size_t mid = 333;
    std::vector<uint8_t> both;
    both.insert(both.end(), data.begin(), data.begin() + mid);
    both.insert(both.end(), data.begin() + mid, data.end());
    uint32_t two = crc32_ieee(both.data(), both.size());

    EXPECT_EQ(one, two);
}
