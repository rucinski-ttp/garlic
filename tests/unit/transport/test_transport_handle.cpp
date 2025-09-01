#include <gtest/gtest.h>
extern "C" {
#include "proto/inc/transport.h"
#include "proto/inc/crc32.h"
}

#include <vector>
#include <cstring>

struct Deliver {
  uint16_t session{};
  bool is_resp{};
  std::vector<uint8_t> msg;
  int count{};
};

static void on_msg(void* user, uint16_t session, const uint8_t* msg, size_t len, bool is_resp) {
  auto* d = reinterpret_cast<Deliver*>(user);
  d->session = session;
  d->is_resp = is_resp;
  d->msg.assign(msg, msg + len);
  d->count++;
}

TEST(TransportHandle, MultiFragmentDelivery)
{
  // Encode a 256-byte message (forces two fragments)
  std::vector<uint8_t> payload(256);
  for (size_t i = 0; i < payload.size(); ++i) payload[i] = static_cast<uint8_t>(i);

  std::vector<uint8_t> frames;
  size_t maxp = TRANSPORT_FRAME_MAX_PAYLOAD;
  uint16_t frag_count = static_cast<uint16_t>((payload.size() + maxp - 1) / maxp);
  if (frag_count == 0) frag_count = 1;
  uint16_t frag_index = 0;
  size_t offset = 0;
  while (frag_index < frag_count) {
    size_t remaining = payload.size() - offset;
    size_t take = remaining < maxp ? remaining : maxp;
    uint8_t flags = 0;
    if (frag_index == 0) flags |= TRANSPORT_FLAG_START;
    if (frag_index == frag_count - 1) flags |= TRANSPORT_FLAG_END; else flags |= TRANSPORT_FLAG_MIDDLE;
    uint8_t buf[2 + 10 + TRANSPORT_FRAME_MAX_PAYLOAD + 4];
    size_t pos = 0;
    buf[pos++] = 0xA5; buf[pos++] = 0x5A;
    buf[pos++] = TRANSPORT_VERSION; buf[pos++] = flags;
    buf[pos++] = 0x22 & 0xFF; buf[pos++] = (0x22 >> 8) & 0xFF;
    buf[pos++] = frag_index & 0xFF; buf[pos++] = (frag_index >> 8) & 0xFF;
    buf[pos++] = frag_count & 0xFF; buf[pos++] = (frag_count >> 8) & 0xFF;
    buf[pos++] = static_cast<uint16_t>(take) & 0xFF; buf[pos++] = (static_cast<uint16_t>(take) >> 8) & 0xFF;
    if (take) memcpy(&buf[pos], &payload[offset], take);
    pos += take;
    // CRC over [ver..payload]
    uint32_t crc = crc32_ieee(&buf[2], 10 + take);
    buf[pos++] = crc & 0xFF; buf[pos++] = (crc >> 8) & 0xFF; buf[pos++] = (crc >> 16) & 0xFF; buf[pos++] = (crc >> 24) & 0xFF;
    frames.insert(frames.end(), buf, buf + pos);
    offset += take; frag_index++;
  }

  // Feed to RX transport and verify one delivery
  struct transport_ctx rx{};
  Deliver d{};
  struct transport_lower_if lower_rx { nullptr };
  grlc_transport_init(&rx, &lower_rx, on_msg, &d);
  grlc_transport_rx_bytes(&rx, frames.data(), frames.size());
  ASSERT_EQ(d.count, 1);
  EXPECT_EQ(d.session, 0x22);
  EXPECT_FALSE(d.is_resp);
  ASSERT_EQ(d.msg.size(), payload.size());
  EXPECT_EQ(std::memcmp(d.msg.data(), payload.data(), payload.size()), 0);
}

