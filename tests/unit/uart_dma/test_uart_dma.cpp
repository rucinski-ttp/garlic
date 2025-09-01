// Unit tests for UART DMA driver (host build with test shim)

#include <gtest/gtest.h>
#include <gmock/gmock.h>

extern "C" {
#include "drivers/uart/inc/uart.h"
#include "drivers/uart/inc/uart_test.h"
}

using ::testing::Eq;

namespace {

static int last_rx_buf_rsp_idx = -1;
static int rx_buf_rsp_calls = 0;
static int rx_enable_calls = 0;

static uint8_t *buf0 = nullptr;
static uint8_t *buf1 = nullptr;

// Simple interceptors that detect buffer index by pointer equality. We capture
// the first two distinct pointers observed.
static int hal_rx_buf_rsp_intercept(const device *, unsigned char *buf, unsigned long len)
{
    (void)len;
    rx_buf_rsp_calls++;
    if (!buf0) {
        buf0 = buf;
    } else if (!buf1 && buf != buf0) {
        buf1 = buf;
    }
    if (buf == buf0) last_rx_buf_rsp_idx = 0; else if (buf == buf1) last_rx_buf_rsp_idx = 1; else last_rx_buf_rsp_idx = -1;
    return 0;
}

static int hal_rx_enable_intercept(const device *, unsigned char *buf, unsigned long len, unsigned int timeout)
{
    (void)len; (void)timeout;
    rx_enable_calls++;
    // Capture buf0 on first enable
    if (!buf0) buf0 = buf;
    return 0;
}

} // namespace

TEST(UartDma, AlternatesRxBuffersOnRequest)
{
    uart_dma_test_reset();
    uart_dma_test_set_hal_rx_buf_rsp(&hal_rx_buf_rsp_intercept);
    // Request 1 -> expect buf0
    uart_event e{}; e.type = UART_RX_BUF_REQUEST;
    uart_dma_test_invoke_event(&e);
    ASSERT_EQ(rx_buf_rsp_calls, 1);
    EXPECT_EQ(last_rx_buf_rsp_idx, 0);
    // Request 2 -> expect buf1
    uart_dma_test_invoke_event(&e);
    ASSERT_EQ(rx_buf_rsp_calls, 2);
    EXPECT_EQ(last_rx_buf_rsp_idx, 1);
    // Request 3 -> back to buf0
    uart_dma_test_invoke_event(&e);
    ASSERT_EQ(rx_buf_rsp_calls, 3);
    EXPECT_EQ(last_rx_buf_rsp_idx, 0);
}

TEST(UartDma, RestartSetsIndexAndEnables)
{
    uart_dma_test_reset();
    uart_dma_test_set_hal_rx_enable(&hal_rx_enable_intercept);
    // Simulate RX disabled -> driver should enable(buf0) and set idx=1
    uart_event e{}; e.type = UART_RX_DISABLED;
    uart_dma_test_invoke_event(&e);
    EXPECT_EQ(rx_enable_calls, 1);
    // Now a buffer request should hand out buf1 (since idx was set to 1)
    uart_dma_test_set_hal_rx_buf_rsp(&hal_rx_buf_rsp_intercept);
    uart_event r{}; r.type = UART_RX_BUF_REQUEST;
    uart_dma_test_invoke_event(&r);
    ASSERT_EQ(rx_buf_rsp_calls, 1);
    EXPECT_EQ(last_rx_buf_rsp_idx, 1);
}

TEST(UartDma, RxStoppedUpdatesStatsAndRestart)
{
    uart_dma_test_reset();
    // Install rx_enable stub to observe restarts
    auto rx_enable_stub = [](const device*, unsigned char*, unsigned long, unsigned int) -> int {
        rx_enable_calls++;
        return 0;
    };
    uart_dma_test_set_hal_rx_enable(+rx_enable_stub);

    // Simulate RX stopped with different reasons; expect stats updated and restart attempted
    uart_event e{}; e.type = UART_RX_STOPPED; e.data.rx_stop.reason = UART_ERROR_FRAMING;
    uart_dma_test_invoke_event(&e);
    uart_statistics s{}; grlc_uart_get_statistics(&s);
    EXPECT_EQ(s.framing_errors, 1u);

    e.data.rx_stop.reason = UART_ERROR_PARITY;
    uart_dma_test_invoke_event(&e);
    grlc_uart_get_statistics(&s);
    EXPECT_EQ(s.parity_errors, 1u);

    e.data.rx_stop.reason = UART_ERROR_OVERRUN;
    uart_dma_test_invoke_event(&e);
    grlc_uart_get_statistics(&s);
    EXPECT_EQ(s.rx_overruns, 1u);

    // We expect rx_enable to have been called on each stop when initialized
    EXPECT_GE(rx_enable_calls, 1);
}
