#include <gtest/gtest.h>
extern "C" {
#include "proto/inc/transport.h"
}

static size_t s_last_write_len;
static size_t test_write(const uint8_t* data, size_t len)
{
  (void)data;
  s_last_write_len = len;
  return len;
}

TEST(TransportApi, SendMessageRejectsNulls)
{
  struct transport_ctx t = {};
  struct transport_lower_if lower = {nullptr};
  grlc_transport_init(&t, &lower, nullptr, nullptr);
  EXPECT_FALSE(grlc_transport_send_message(&t, 1, (const uint8_t*)"x", 1, false));
  lower.write = test_write;
  grlc_transport_init(&t, &lower, nullptr, nullptr);
  EXPECT_FALSE(grlc_transport_send_message(&t, 1, nullptr, 1, false));
}

TEST(TransportApi, SendZeroLengthMessage)
{
  struct transport_ctx t = {};
  struct transport_lower_if lower = {test_write};
  grlc_transport_init(&t, &lower, nullptr, nullptr);
  s_last_write_len = 0;
  ASSERT_TRUE(grlc_transport_send_message(&t, 0x1234, nullptr, 0, false));
  // One frame: 2 sync + 10 hdr + 0 payload + 4 crc
  EXPECT_EQ(s_last_write_len, 2u + 10u + 0u + 4u);
}

