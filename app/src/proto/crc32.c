#include "proto/inc/crc32.h"

// Minimal table-based CRC32 implementation (IEEE 802.3), reflected.

static uint32_t crc32_table[256];
static int crc32_table_init = 0;

static void crc32_init_table(void)
{
    if (crc32_table_init) {
        return;
    }
    uint32_t poly = 0xEDB88320u; // reversed 0x04C11DB7
    for (uint32_t i = 0; i < 256; ++i) {
        uint32_t c = i;
        for (int j = 0; j < 8; ++j) {
            if (c & 1) {
                c = (c >> 1) ^ poly;
            } else {
                c >>= 1;
            }
        }
        crc32_table[i] = c;
    }
    crc32_table_init = 1;
}

uint32_t crc32_ieee(const uint8_t *data, size_t len)
{
    crc32_init_table();
    uint32_t crc = 0xFFFFFFFFu;
    for (size_t i = 0; i < len; ++i) {
        uint8_t b = data[i];
        crc = crc32_table[(crc ^ b) & 0xFFu] ^ (crc >> 8);
    }
    return crc ^ 0xFFFFFFFFu;
}
