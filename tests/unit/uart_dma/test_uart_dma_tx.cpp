// Unit test for UART DMA TX path using host HAL shims

#include <gtest/gtest.h>

extern "C" {
#include "drivers/uart_dma/inc/uart_dma.h"
#include "drivers/uart_dma/inc/uart_dma_test.h"
}

namespace {

static int stub_callback_set_calls = 0;
static int stub_rx_enable_calls = 0;
static int stub_tx_calls = 0;
static int last_tx_len = 0;

static int stub_callback_set(const device *, void (*)(const device *, uart_event *, void *), void *)
{
    // Accept registration silently
    (void)stub_callback_set_calls;
    stub_callback_set_calls++;
    return 0;
}

static int stub_rx_enable(const device *, unsigned char *, unsigned long, unsigned int)
{
    stub_rx_enable_calls++;
    return 0;
}

static int stub_tx(const device *, const unsigned char *, unsigned long len, unsigned int)
{
    stub_tx_calls++;
    last_tx_len = (int)len;
    return 0;
}

} // namespace

TEST(UartDmaTx, SendAndCompleteUpdatesStats)
{
    // Install HAL shims and initialize driver
    uart_dma_test_set_hal_callback_set(&stub_callback_set);
    uart_dma_test_set_hal_rx_enable(&stub_rx_enable);
    uart_dma_test_set_hal_tx(&stub_tx);

    ASSERT_EQ(uart_dma_init(), UART_DMA_STATUS_OK);

    // Send a small buffer; driver should issue one TX and then we simulate completion
    uint8_t buf[64];
    for (int i = 0; i < (int)sizeof(buf); ++i) buf[i] = (uint8_t)i;
    ASSERT_EQ(uart_dma_send(buf, sizeof(buf)), UART_DMA_STATUS_OK);

    // The driver reads up to 256 bytes; expect one TX call with len==64
    ASSERT_EQ(stub_tx_calls, 1);
    ASSERT_EQ(last_tx_len, 64);

    // Simulate TX done event to release in-progress flag and update stats
    uart_event e{}; e.type = UART_TX_DONE; e.data.tx.len = sizeof(buf);
    uart_dma_test_invoke_event(&e);

    uart_statistics stats{};
    uart_dma_get_statistics(&stats);
    EXPECT_GE(stats.tx_bytes, (uint32_t)sizeof(buf));
    EXPECT_TRUE(uart_dma_tx_complete());

    // Sending a byte API should also work
    ASSERT_EQ(uart_dma_send_byte(0xAA), UART_DMA_STATUS_OK);
}

